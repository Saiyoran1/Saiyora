#include "BuffHandler.h"
#include "Buff.h"
#include "CombatAbility.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "AbilityComponent.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"

#pragma region Initialization

UBuffHandler::UBuffHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
	bReplicateUsingRegisteredSubObjectList = true;
}

void UBuffHandler::InitializeComponent()
{
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Buff Handler."));
	CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	ThreatHandlerRef = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwner());
	MovementComponentRef = ISaiyoraCombatInterface::Execute_GetCustomMovementComponent(GetOwner());
	CcHandlerRef = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwner());
	NPCComponentRef = Cast<UNPCAbilityComponent>(ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwner()));
}

void UBuffHandler::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwnerRole() == ROLE_Authority && IsValid(DamageHandlerRef))
	{
		if (IsValid(DamageHandlerRef))
		{
			DamageHandlerRef->OnLifeStatusChanged.AddDynamic(this, &UBuffHandler::RemoveBuffsOnOwnerDeath);
		}
		if (IsValid(NPCComponentRef))
		{
			NPCComponentRef->OnCombatBehaviorChanged.AddDynamic(this, &UBuffHandler::RemoveBuffsOnCombatEnd);
		}
	}
}

void UBuffHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

#pragma endregion 
#pragma region Application

FBuffApplyEvent UBuffHandler::ApplyBuff(const TSubclassOf<UBuff> BuffClass, AActor* AppliedBy,
		UObject* Source, const bool DuplicateOverride, const EBuffApplicationOverrideType StackOverrideType,
		const int32 OverrideStacks, const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration,
		const bool IgnoreRestrictions, const TArray<FInstancedStruct>& BuffParams)
{
	FBuffApplyEvent Event;

	//Do generic/blanket checks first for valid net role and input data.
	if (GetOwnerRole() != ROLE_Authority)
	{
		Event.FailReasons.AddUnique(EBuffApplyFailReason::NetRole);
	}
	if (!bCanEverReceiveBuffs)
	{
		Event.FailReasons.AddUnique(EBuffApplyFailReason::InvalidAppliedTo);
	}
	if (!IsValid(BuffClass))
	{
		Event.FailReasons.AddUnique(EBuffApplyFailReason::InvalidClass);
	}
	if (!IsValid(AppliedBy))
	{
		Event.FailReasons.AddUnique(EBuffApplyFailReason::InvalidAppliedBy);
	}
	if (!IsValid(Source))
	{
		Event.FailReasons.AddUnique(EBuffApplyFailReason::InvalidSource);
	}

	//Fail to apply if any validity/generic checks failed.
	//Not realistic to check any of the other conditions with invalid data or blanket restrictions.
	if (Event.FailReasons.Num() > 0)
	{
		Event.ActionTaken = EBuffApplyAction::Failed;
		return Event;
	}

	//Fill out basic information for the event struct: AppliedBy, AppliedTo, Source, Plane information, custom params, etc.
	Event.BuffClass = BuffClass;
	Event.AppliedBy = AppliedBy;
	UCombatStatusComponent* GeneratorCombatStatus = nullptr;
	UBuffHandler* GeneratorBuff = nullptr;
	if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		GeneratorCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(AppliedBy);
		GeneratorBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(AppliedBy);
	}
	Event.OriginPlane = IsValid(GeneratorCombatStatus) ? GeneratorCombatStatus->GetCurrentPlane() : ESaiyoraPlane::Both;
	Event.AppliedTo = GetOwner();
	Event.TargetPlane = IsValid(CombatStatusComponentRef) ? CombatStatusComponentRef->GetCurrentPlane() : ESaiyoraPlane::Both;
	Event.Source = Source;
	Event.CombatParams = BuffParams;
	
	//Check buff restrictions, and other restrictions applied by combat components.
	//These functions will add their fail reasons if they have any.
    if (!IgnoreRestrictions && (CheckIncomingBuffRestricted(Event, Event.FailReasons) || (IsValid(GeneratorBuff) && GeneratorBuff->CheckOutgoingBuffRestricted(Event, Event.FailReasons))))
    {
    	Event.ActionTaken = EBuffApplyAction::Failed;
    	return Event;
    }

	//If an existing buff of this class from this owner exists, and we aren't trying to duplicate (via normal behavior or an override),
	//then we want to try and stack or refresh the existing buff.
	UBuff* AffectedBuff = FindExistingBuff(BuffClass, true, AppliedBy);
	if (IsValid(AffectedBuff) && !AffectedBuff->IsDuplicable() && !DuplicateOverride)
	{
		//The event can still fail here, if the existing buff can not be stacked or refreshed and there aren't overrides for either.
		AffectedBuff->ApplyEvent(Event, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
	}
	//If we are duplicating or there isn't an existing buff, create a new buff instance.
	else
	{
		//Currently, this can't fail from this point on. We assume the object will be created and initialize okay.
		Event.AffectedBuff = NewObject<UBuff>(GetOwner(), Event.BuffClass);
		Event.AffectedBuff->InitializeBuff(Event, this, IgnoreRestrictions, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
	}
    return Event;
}

void UBuffHandler::NotifyOfNewIncomingBuff(const FBuffApplyEvent& ApplicationEvent)
{
	//This is called on both client and server, so guard against invalid pointer.
	if (!IsValid(ApplicationEvent.AffectedBuff))
	{
		return;
	}
	//Buff will be added to an array based on its type (buff, debuff, or hidden).
	TArray<UBuff*>* BuffArray;
	switch (ApplicationEvent.AffectedBuff->GetBuffType())
	{
	case EBuffType::Buff :
		BuffArray = &Buffs;
		break;
	case EBuffType::Debuff :
		BuffArray = &Debuffs;
		break;
	case EBuffType::HiddenBuff :
		BuffArray = &HiddenBuffs;
		break;
	default :
		return;
	}
	if (BuffArray->Contains(ApplicationEvent.AffectedBuff))
	{
		return;
	}
	BuffArray->Add(ApplicationEvent.AffectedBuff);
	AddReplicatedSubObject(ApplicationEvent.AffectedBuff);
	OnIncomingBuffApplied.Broadcast(ApplicationEvent);
	//Alert the actor who applied this buff that they should keep track of it as well.
	if (IsValid(ApplicationEvent.AffectedBuff->GetAppliedBy()) && ApplicationEvent.AffectedBuff->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UBuffHandler* GeneratorBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(ApplicationEvent.AffectedBuff->GetAppliedBy());
		if (IsValid(GeneratorBuff))
		{
			GeneratorBuff->NotifyOfNewOutgoingBuff(ApplicationEvent);
		}
	}
}

void UBuffHandler::NotifyOfNewOutgoingBuff(const FBuffApplyEvent& ApplicationEvent)
{
	if (!IsValid(ApplicationEvent.AffectedBuff) || OutgoingBuffs.Contains(ApplicationEvent.AffectedBuff))
	{
		return;
	}
	OutgoingBuffs.Add(ApplicationEvent.AffectedBuff);
	OnOutgoingBuffApplied.Broadcast(ApplicationEvent);
}

#pragma endregion
#pragma region Removal

FBuffRemoveEvent UBuffHandler::RemoveBuff(UBuff* Buff, const EBuffExpireReason ExpireReason)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Buff) || Buff->GetAppliedTo() != GetOwner())
	{
		return FBuffRemoveEvent();
	}
	return Buff->TerminateBuff(ExpireReason);
}

void UBuffHandler::NotifyOfIncomingBuffRemoval(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(RemoveEvent.RemovedBuff))
	{
		return;
	}
	//Remove the buff from an array based on buff type (buff, debuff, or hidden).
	TArray<UBuff*>* BuffArray;
	switch (RemoveEvent.RemovedBuff->GetBuffType())
	{
	case EBuffType::Buff :
		BuffArray = &Buffs;
		break;
	case EBuffType::Debuff :
		BuffArray = &Debuffs;
		break;
	case EBuffType::HiddenBuff :
		BuffArray = &HiddenBuffs;
		break;
	default :
		return;
	}
	if (BuffArray->Remove(RemoveEvent.RemovedBuff) > 0)
	{
		OnIncomingBuffRemoved.Broadcast(RemoveEvent);
		//Move the buff to another array that will continue to replicate for a short time.
		//This lets clients get the chance to call any effects for buff removal.
		//After 1 second, we remove the buff from that array so it will stop replicating and be garbage collected.
		RecentlyRemoved.Add(RemoveEvent.RemovedBuff);
		FTimerHandle RemoveTimer;
		const FTimerDelegate RemoveDel = FTimerDelegate::CreateUObject(this, &UBuffHandler::PostRemoveCleanup, RemoveEvent.RemovedBuff);
		GetWorld()->GetTimerManager().SetTimer(RemoveTimer, RemoveDel, 1.0f, false);

		//Alert the actor who applied the buff that it is being removed.
		if (IsValid(RemoveEvent.RemovedBuff->GetAppliedBy()) && RemoveEvent.RemovedBuff->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UBuffHandler* BuffGenerator = ISaiyoraCombatInterface::Execute_GetBuffHandler(RemoveEvent.RemovedBuff->GetAppliedBy());
			if (IsValid(BuffGenerator))
			{
				BuffGenerator->NotifyOfOutgoingBuffRemoval(RemoveEvent);
			}
		}
	}
}

void UBuffHandler::NotifyOfOutgoingBuffRemoval(const FBuffRemoveEvent& RemoveEvent)
{
	//We don't need to worry about moving it to another array for replication, the target will handle that.
	if (OutgoingBuffs.Remove(RemoveEvent.RemovedBuff) > 0)
	{
		OnOutgoingBuffRemoved.Broadcast(RemoveEvent);
	}
}

void UBuffHandler::PostRemoveCleanup(UBuff* Buff)
{
	//After letting a removed buff replicate, we can get rid of it.
	RemoveReplicatedSubObject(Buff);
	RecentlyRemoved.Remove(Buff);	
}

void UBuffHandler::RemoveBuffsOnOwnerDeath(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (NewStatus != ELifeStatus::Alive)
	{
		TArray<UBuff*> BuffsToRemove;
		for (UBuff* Buff : Buffs)
		{
			if (!Buff->CanBeAppliedWhileDead())
			{
				BuffsToRemove.Add(Buff);
			}
		}
		for (UBuff* Debuff : Debuffs)
		{
			if (!Debuff->CanBeAppliedWhileDead())
			{
				BuffsToRemove.Add(Debuff);
			}
		}
		for (UBuff* HiddenBuff : HiddenBuffs)
		{
			if (!HiddenBuff->CanBeAppliedWhileDead())
			{
				BuffsToRemove.Add(HiddenBuff);
			}
		}
		for (UBuff* BuffToRemove : BuffsToRemove)
		{
			RemoveBuff(BuffToRemove, EBuffExpireReason::Death);
		}
	}
}

void UBuffHandler::RemoveBuffsOnCombatEnd(const ENPCCombatBehavior PreviousBehavior,
	const ENPCCombatBehavior NewBehavior)
{
	if (PreviousBehavior == ENPCCombatBehavior::Combat && NewBehavior != ENPCCombatBehavior::Combat)
	{
		TArray<UBuff*> BuffsToRemove;
		for (UBuff* Buff : Buffs)
		{
			if (Buff->IsRemovedOnCombatEnd())
			{
				BuffsToRemove.Add(Buff);
			}
		}
		for (UBuff* Buff : Debuffs)
		{
			if (Buff->IsRemovedOnCombatEnd())
			{
				BuffsToRemove.Add(Buff);
			}
		}
		for (UBuff* Buff : HiddenBuffs)
		{
			if (Buff->IsRemovedOnCombatEnd())
			{
				BuffsToRemove.Add(Buff);
			}
		}
		for (UBuff* Buff : BuffsToRemove)
		{
			RemoveBuff(Buff, EBuffExpireReason::Combat);
		}
	}
}

#pragma endregion 
#pragma region Get Buffs

void UBuffHandler::GetBuffsOfClass(const TSubclassOf<UBuff> BuffClass, TArray<UBuff*>& OutBuffs) const
{
	OutBuffs.Empty();
	if (!IsValid(BuffClass))
	{
		return;
	}
	TArray<UBuff*> const* BuffArray;
	switch (BuffClass.GetDefaultObject()->GetBuffType())
	{
		case EBuffType::Buff :
			BuffArray = &Buffs;
			break;
		case EBuffType::Debuff :
			BuffArray = &Debuffs;
			break;
		case EBuffType::HiddenBuff :
			BuffArray = &HiddenBuffs;
			break;
		default :
			return;
	}
	for (UBuff* Buff : *BuffArray)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass)
		{
			OutBuffs.Add(Buff);
		}
	}
}

void UBuffHandler::GetBuffsAppliedByActor(const AActor* Actor, TArray<UBuff*>& OutBuffs) const
{
	OutBuffs.Empty();
	if (!IsValid(Actor))
	{
		return;
	}
	for (UBuff* Buff : Buffs)
	{
		if (IsValid(Buff) && Buff->GetAppliedBy() == Actor)
		{
			OutBuffs.Add(Buff);
		}
	}
	for (UBuff* Debuff : Debuffs)
	{
		if (IsValid(Debuff) && Debuff->GetAppliedBy() == Actor)
		{
			OutBuffs.Add(Debuff);
		}
	}
	for (UBuff* HiddenBuff : HiddenBuffs)
	{
		if (IsValid(HiddenBuff) && HiddenBuff->GetAppliedBy() == Actor)
		{
			OutBuffs.Add(HiddenBuff);
		}
	}
}

void UBuffHandler::GetOutgoingBuffsOfClass(const TSubclassOf<UBuff> BuffClass, TArray<UBuff*>& OutBuffs) const
{
	OutBuffs.Empty();
	if (!IsValid(BuffClass))
	{
		return;
	}
	for (UBuff* Buff : OutgoingBuffs)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass)
		{
			OutBuffs.Add(Buff);
		}
	}
}

void UBuffHandler::GetBuffsAppliedToActor(const AActor* Target, TArray<UBuff*>& OutBuffs) const
{
	OutBuffs.Empty();
	if (!IsValid(Target))
	{
		return;
	}
	for (UBuff* Buff : OutgoingBuffs)
	{
		if (IsValid(Buff) && Buff->GetAppliedTo() == Target)
		{
			OutBuffs.Add(Buff);
		}
	}
}

UBuff* UBuffHandler::FindExistingOutgoingBuff(const TSubclassOf<UBuff> BuffClass, const bool bSpecificTarget,
	const AActor* BuffTarget) const
{
	if (!IsValid(BuffClass) || (bSpecificTarget && !IsValid(BuffTarget)))
	{
		return nullptr;
	}
	for (UBuff* Buff : OutgoingBuffs)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass && (!bSpecificTarget || Buff->GetAppliedTo() == BuffTarget))
		{
			return Buff;
		}
	}
	return nullptr;
}

UBuff* UBuffHandler::FindExistingBuff(const TSubclassOf<UBuff> BuffClass, const bool bSpecificOwner, const AActor* BuffOwner) const
{
	if (!IsValid(BuffClass) || (bSpecificOwner && !IsValid(BuffOwner)))
	{
		return nullptr;
	}
	TArray<UBuff*> const* BuffArray;
	switch (BuffClass.GetDefaultObject()->GetBuffType())
	{
		case EBuffType::Buff :
			BuffArray = &Buffs;
			break;
		case EBuffType::Debuff :
			BuffArray = &Debuffs;
			break;
		case EBuffType::HiddenBuff :
			BuffArray = &HiddenBuffs;
			break;
		default :
			return nullptr;
	}
	for (UBuff* Buff : *BuffArray)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass && (!bSpecificOwner || Buff->GetAppliedBy() == BuffOwner))
		{
			return Buff;
		}
	}
	return nullptr;
}

#pragma endregion 
#pragma region Restrictions

bool UBuffHandler::CheckIncomingBuffRestricted(const FBuffApplyEvent& BuffEvent, TArray<EBuffApplyFailReason>& OutFailReasons)
{
	//Don't clear the OutFailReasons array, it might have fail reasons from before this point in the application process.
	bool bRestricted = false;
	//Check any custom restrictions first.
	if (IncomingBuffRestrictions.IsRestricted(BuffEvent))
	{
		OutFailReasons.AddUnique(EBuffApplyFailReason::IncomingRestriction);
		bRestricted = true;
	}
	//Check if the buff can be applied to dead targets.
	if (!BuffEvent.BuffClass.GetDefaultObject()->CanBeAppliedWhileDead() && IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		OutFailReasons.AddUnique(EBuffApplyFailReason::Dead);
		bRestricted = true;
	}
	//Check the buff's tags for any restrictions inherent to other combat components.
	//This would mean things like trying to do damage over time when a target is immune to damage, trying to move a target with no movement component, etc.
	//I'm not 100% convinced this is the best way to do these kinds of restrictions, but it does make outputting fail reasons more clear.
	FGameplayTagContainer BuffTags;
	BuffEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	for (const FGameplayTag Tag : BuffTags)
	{
		if (Tag.MatchesTagExact(FSaiyoraCombatTags::Get().Damage) && (!IsValid(DamageHandlerRef) || !DamageHandlerRef->CanEverReceiveDamage()))
		{
			OutFailReasons.AddUnique(EBuffApplyFailReason::ImmuneToDamage);
			bRestricted = true;
		}
		if (Tag.MatchesTagExact(FSaiyoraCombatTags::Get().Healing) && (!IsValid(DamageHandlerRef) || !DamageHandlerRef->CanEverReceiveHealing()))
		{
			OutFailReasons.AddUnique(EBuffApplyFailReason::ImmuneToHealing);
			bRestricted = true;
		}
		if (Tag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !Tag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat) && (!IsValid(StatHandlerRef) || !StatHandlerRef->IsStatModifiable(Tag)))
		{
			OutFailReasons.AddUnique(EBuffApplyFailReason::StatNotModifiable);
			bRestricted = true;
		}
		if (Tag.MatchesTag(FSaiyoraCombatTags::Get().CrowdControl) && !Tag.MatchesTagExact(FSaiyoraCombatTags::Get().CrowdControl) && (!IsValid(CcHandlerRef) || CcHandlerRef->IsImmuneToCrowdControl(Tag)))
		{
			OutFailReasons.AddUnique(EBuffApplyFailReason::ImmuneToCc);
			bRestricted = true;
		}
		if (Tag.MatchesTag(FSaiyoraCombatTags::Get().Threat) && (!IsValid(ThreatHandlerRef) || !ThreatHandlerRef->HasThreatTable()))
		{
			OutFailReasons.AddUnique(EBuffApplyFailReason::NoThreatTable);
			bRestricted = true;
		}
		if (Tag.MatchesTagExact(FSaiyoraCombatTags::Get().ExternalMovement) && !IsValid(MovementComponentRef))
		{
			OutFailReasons.AddUnique(EBuffApplyFailReason::NoMovement);
			bRestricted = true;
		}
	}
	return bRestricted;
}

bool UBuffHandler::CheckOutgoingBuffRestricted(const FBuffApplyEvent& BuffEvent, TArray<EBuffApplyFailReason>& OutFailReasons)
{
	//Don't clear the OutFailReasons array, it might have fail reasons from before this point in the application process.
	bool bRestricted = false;
	//Check custom restrictions first.
	if (OutgoingBuffRestrictions.IsRestricted(BuffEvent))
	{
		OutFailReasons.AddUnique(EBuffApplyFailReason::OutgoingRestriction);
		bRestricted = true;
	}
	//Check buff tags for restrictions inherent to other combat components.
	//In this case, its just whether the instigator can be in a threat table, if it's trying to apply a buff that generates threat.
	FGameplayTagContainer BuffTags;
	BuffEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	for (const FGameplayTag Tag : BuffTags)
	{
		if (Tag.MatchesTag(FSaiyoraCombatTags::Get().Threat) && (!IsValid(ThreatHandlerRef) || !ThreatHandlerRef->CanBeInThreatTable()))
		{
			OutFailReasons.AddUnique(EBuffApplyFailReason::InvalidThreatTarget);
			bRestricted = true;
			break;
		}
	}
	return bRestricted;
}

#pragma endregion