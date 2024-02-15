#include "BuffHandler.h"
#include "Buff.h"
#include "CombatAbility.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "AbilityComponent.h"
#include "AbilityFunctionLibrary.h"
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

void UBuffHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	return;
}

#pragma endregion 
#pragma region Application

FBuffApplyEvent UBuffHandler::ApplyBuff(const TSubclassOf<UBuff> BuffClass, AActor* AppliedBy,
		UObject* Source, const bool DuplicateOverride, const EBuffApplicationOverrideType StackOverrideType,
		const int32 OverrideStacks, const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration,
		const bool IgnoreRestrictions, const TArray<FInstancedStruct>& BuffParams)
{
    FBuffApplyEvent Event;

    if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveBuffs || !IsValid(AppliedBy) || !IsValid(Source) || !IsValid(BuffClass))
    {
        Event.ActionTaken = EBuffApplyAction::Failed;
        return Event;
    }

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
	Event.AppliedXPlane = UAbilityFunctionLibrary::IsXPlane(Event.OriginPlane, Event.TargetPlane);
    Event.Source = Source;
    Event.BuffClass = BuffClass;
	Event.CombatParams = BuffParams;
	UCombatAbility* AbilitySource = Cast<UCombatAbility>(Source);
	if (IsValid(AbilitySource) && AbilitySource->GetHandler()->GetOwner() == Event.AppliedBy && AbilitySource->GetHandler()->GetOwner() == Event.AppliedTo)
	{
		Event.PredictionID = AbilitySource->GetPredictionID();
	}

    if (!IgnoreRestrictions && (CheckIncomingBuffRestricted(Event) || (IsValid(GeneratorBuff) && GeneratorBuff->CheckOutgoingBuffRestricted(Event))))
    {
    	Event.ActionTaken = EBuffApplyAction::Failed;
    	return Event;
    }

	UBuff* AffectedBuff = FindExistingBuff(BuffClass, true, AppliedBy);
	if (IsValid(AffectedBuff) && !AffectedBuff->IsDuplicable() && !DuplicateOverride)
	{
		AffectedBuff->ApplyEvent(Event, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
	}
	else
	{
		Event.AffectedBuff = NewObject<UBuff>(GetOwner(), Event.BuffClass);
		Event.AffectedBuff->InitializeBuff(Event, this, IgnoreRestrictions, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
	}
    return Event;
}

void UBuffHandler::NotifyOfNewIncomingBuff(const FBuffApplyEvent& ApplicationEvent)
{
	if (!IsValid(ApplicationEvent.AffectedBuff))
	{
		return;
	}
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
		//This is to allow replication one last time (replicating the remove event).
		RecentlyRemoved.Add(RemoveEvent.RemovedBuff);
		FTimerHandle RemoveTimer;
		const FTimerDelegate RemoveDel = FTimerDelegate::CreateUObject(this, &UBuffHandler::PostRemoveCleanup, RemoveEvent.RemovedBuff);
		GetWorld()->GetTimerManager().SetTimer(RemoveTimer, RemoveDel, 1.0f, false);
		
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
	if (OutgoingBuffs.Remove(RemoveEvent.RemovedBuff) > 0)
	{
		OnOutgoingBuffRemoved.Broadcast(RemoveEvent);
	}
}

void UBuffHandler::PostRemoveCleanup(UBuff* Buff)
{
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

bool UBuffHandler::CheckIncomingBuffRestricted(const FBuffApplyEvent& BuffEvent)
{
	if (IncomingBuffRestrictions.IsRestricted(BuffEvent))
	{
		return true;
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	if (!BuffEvent.BuffClass.GetDefaultObject()->CanBeAppliedWhileDead() && IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		return true;
	}
	for (const FGameplayTag Tag : BuffTags)
	{
		if (Tag.MatchesTagExact(FSaiyoraCombatTags::Get().Damage) && (!IsValid(DamageHandlerRef) || !DamageHandlerRef->CanEverReceiveDamage()))
		{
			return true;
		}
		if (Tag.MatchesTagExact(FSaiyoraCombatTags::Get().Healing) && (!IsValid(DamageHandlerRef) || !DamageHandlerRef->CanEverReceiveHealing()))
		{
			return true;
		}
		if (Tag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !Tag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat) && (!IsValid(StatHandlerRef) || !StatHandlerRef->IsStatModifiable(Tag)))
		{
			return true;
		}
		if (Tag.MatchesTag(FSaiyoraCombatTags::Get().CrowdControl) && !Tag.MatchesTagExact(FSaiyoraCombatTags::Get().CrowdControl) && (!IsValid(CcHandlerRef) || CcHandlerRef->IsImmuneToCrowdControl(Tag)))
		{
			return true;
		}
		if (Tag.MatchesTag(FSaiyoraCombatTags::Get().Threat) && (!IsValid(ThreatHandlerRef) || !ThreatHandlerRef->HasThreatTable()))
		{
			return true;
		}
		if (Tag.MatchesTagExact(FSaiyoraCombatTags::Get().ExternalMovement) && !IsValid(MovementComponentRef))
		{
			return true;
		}
	}
	return false;
}

bool UBuffHandler::CheckOutgoingBuffRestricted(const FBuffApplyEvent& BuffEvent)
{
	if (OutgoingBuffRestrictions.IsRestricted(BuffEvent))
	{
		return true;
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	for (const FGameplayTag Tag : BuffTags)
	{
		if (Tag.MatchesTag(FSaiyoraCombatTags::Get().Threat) && (!IsValid(ThreatHandlerRef) || !ThreatHandlerRef->CanBeInThreatTable()))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion