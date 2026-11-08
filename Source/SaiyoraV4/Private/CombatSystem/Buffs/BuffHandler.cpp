#include "BuffHandler.h"
#include "Buff.h"
#include "BuffFunctionality.h"
#include "CombatDebugOptions.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "DamageHandler.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraDevSettings.h"

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
	Super::InitializeComponent();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	checkf(GetOwner()->Implements<USaiyoraCombatInterface>(), TEXT("Owner does not implement combat interface, but has Buff Handler."));
	CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
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
	Event.EventID = USaiyoraCombatLibrary::GetUniqueCombatEventID();
	//Fill out basic information for the event struct: AppliedBy, AppliedTo, Source, Plane information, custom params, etc.
	//We fill this out early mostly for debug logging to have this data if we fail early.
	Event.BuffClass = BuffClass;
	Event.AppliedBy = AppliedBy;
	UCombatStatusComponent* GeneratorCombatStatus = nullptr;
	UBuffHandler* GeneratorBuff = nullptr;
	if (IsValid(AppliedBy) && AppliedBy->Implements<USaiyoraCombatInterface>())
	{
		GeneratorCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(AppliedBy);
		GeneratorBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(AppliedBy);
	}
	Event.OriginPlane = IsValid(GeneratorCombatStatus) ? GeneratorCombatStatus->GetCurrentPlane() : ESaiyoraPlane::Both;
	Event.AppliedTo = GetOwner();
	Event.TargetPlane = IsValid(CombatStatusComponentRef) ? CombatStatusComponentRef->GetCurrentPlane() : ESaiyoraPlane::Both;
	Event.Source = Source;
	Event.CombatParams = BuffParams;

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
		DEBUG_LOG_BUFF_APPLY_FAIL(this, Event);
		return Event;
	}

	//Check here if we'll be working on an existing buff or if we are creating a new buff instance.
	//This can be relevant to restrictions, so we set AffectedBuff to valid before doing restrictions, if we aren't creating a new buff instance.
	UBuff* AffectedBuff = FindExistingBuff(BuffClass, true, AppliedBy);
	if (IsValid(AffectedBuff) && !AffectedBuff->IsDuplicable() && !DuplicateOverride)
	{
		Event.AffectedBuff = AffectedBuff;
	}
	
	//Check buff restrictions, and other restrictions applied by combat components and the buff's own functions.
	//These functions will add their fail reasons if they have any.
    if (!IgnoreRestrictions)
    {
    	FBuffRestriction IncomingRestriction;
    	const bool bIncomingRestriction = CheckBuffRestricted(ECombatEventDirection::Incoming, Event, Event.FailReasons, IncomingRestriction);
    	FBuffRestriction OutgoingRestriction;
    	const bool bOutgoingRestriction = IsValid(GeneratorBuff) && GeneratorBuff->CheckBuffRestricted(ECombatEventDirection::Outgoing, Event, Event.FailReasons, OutgoingRestriction);
    	if (bIncomingRestriction || bOutgoingRestriction)
    	{
    		Event.ActionTaken = EBuffApplyAction::Failed;
    		//Due to some C++ circular dependency fuckery, we can't use FBuffRestriction as a member of FBuffApplyEvent because FBuffRestriction TAKES an FBuffApplyEvent in its signature.
    		//Instead, we can use a proxy struct that can be held by the FBuffApplyEvent to track which restrictions were applied.
    		Event.IncomingRestriction = IncomingRestriction;
    		Event.OutgoingRestriction = OutgoingRestriction;
    		DEBUG_LOG_BUFF_APPLY_FAIL(this, Event);
    		return Event;
    	}
    }

	//If we're acting on an existing buff (stacking/refreshing), do that.
	if (IsValid(Event.AffectedBuff))
	{
		//The event can still fail here, if the existing buff can not be stacked or refreshed and there aren't overrides for either.
		AffectedBuff->ApplyEvent(Event, StackOverrideType, OverrideStacks, RefreshOverrideType, OverrideDuration);
		if (Event.ActionTaken == EBuffApplyAction::Failed)
		{
			DEBUG_LOG_BUFF_APPLY_FAIL(this, Event);
		}
	}
	//Otherwise, we'll create and initialize a brand new buff instance.
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
	if (ActiveBuffs.Contains(ApplicationEvent.AffectedBuff))
	{
		return;
	}
	ActiveBuffs.Add(ApplicationEvent.AffectedBuff);
	AddReplicatedSubObject(ApplicationEvent.AffectedBuff);
	OnIncomingBuffApplied.Broadcast(ApplicationEvent);
	//Alert the actor who applied this buff that they should keep track of it as well.
	if (IsValid(ApplicationEvent.AffectedBuff->GetAppliedBy()) && ApplicationEvent.AffectedBuff->GetAppliedBy()->Implements<USaiyoraCombatInterface>())
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
	FBuffRemoveEvent Event;
	Event.ExpireReason = ExpireReason;
	Event.RemovedBuff = Buff;
	Event.RemovedFrom = GetOwner();
	Event.AppliedBy = IsValid(Buff) ? Buff->GetAppliedBy() : nullptr;
	if (GetOwnerRole() != ROLE_Authority)
	{
		Event.FailReasons.Add(EBuffRemoveFailReason::NetRole);
	}
	if (IsValid(Buff))
	{
		if (Buff->GetAppliedTo() != GetOwner())
		{
			Event.FailReasons.Add(EBuffRemoveFailReason::WrongTarget);
		}
		if (Buff->GetBuffStatus() == EBuffStatus::Removed)
		{
			Event.FailReasons.Add(EBuffRemoveFailReason::AlreadyRemoved);
		}
		if (Buff->IsUndispellable() && ExpireReason == EBuffExpireReason::Dispel)
		{
			Event.FailReasons.Add(EBuffRemoveFailReason::Undispellable);
		}
	}
	else
	{
		Event.FailReasons.Add(EBuffRemoveFailReason::InvalidBuff);
	}
	if (Event.FailReasons.Num() > 0)
	{
		DEBUG_LOG_BUFF_REMOVE_FAIL(this, Event);
		return Event;
	}
	Event.bResult = true;
	Buff->TerminateBuff(Event);
	return Event;
}

void UBuffHandler::NotifyOfIncomingBuffRemoval(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(RemoveEvent.RemovedBuff))
	{
		return;
	}
	if (ActiveBuffs.Remove(RemoveEvent.RemovedBuff) > 0)
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
		if (IsValid(RemoveEvent.RemovedBuff->GetAppliedBy()) && RemoveEvent.RemovedBuff->GetAppliedBy()->Implements<USaiyoraCombatInterface>())
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
		for (UBuff* Buff : ActiveBuffs)
		{
			if (!Buff->CanBeAppliedWhileDead())
			{
				BuffsToRemove.Add(Buff);
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
		for (UBuff* Buff : ActiveBuffs)
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

void UBuffHandler::GetBuffsOfType(const EBuffType BuffType, TArray<UBuff*>& OutBuffs) const
{
	OutBuffs.Empty();
	for (UBuff* Buff : ActiveBuffs)
	{
		if (IsValid(Buff) && Buff->GetBuffType() == BuffType)
		{
			OutBuffs.Add(Buff);
		}
	}
}

void UBuffHandler::GetBuffsOfClass(const TSubclassOf<UBuff> BuffClass, TArray<UBuff*>& OutBuffs) const
{
	OutBuffs.Empty();
	if (!IsValid(BuffClass))
	{
		return;
	}
	for (UBuff* Buff : ActiveBuffs)
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
	for (UBuff* Buff : ActiveBuffs)
	{
		if (IsValid(Buff) && Buff->GetAppliedBy() == Actor)
		{
			OutBuffs.Add(Buff);
		}
	}
}

UBuff* UBuffHandler::FindExistingBuff(const TSubclassOf<UBuff> BuffClass, const bool bSpecificOwner, const AActor* BuffOwner) const
{
	if (!IsValid(BuffClass) || (bSpecificOwner && !IsValid(BuffOwner)))
	{
		return nullptr;
	}
	for (UBuff* Buff : ActiveBuffs)
	{
		if (IsValid(Buff) && Buff->GetClass() == BuffClass && (!bSpecificOwner || Buff->GetAppliedBy() == BuffOwner))
		{
			return Buff;
		}
	}
	return nullptr;
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

#pragma endregion 
#pragma region Restrictions

bool UBuffHandler::AddBuffRestriction(const ECombatEventDirection Direction, const FBuffRestriction& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}
	if (!Restriction.IsBound() || !Restriction.GetUObject()->IsA(UBuff::StaticClass()))
	{
		DEBUG_LOG_BUFF_RESTRICTION_ADD_FAILED(this, Direction, Restriction, TEXT("Restriction not bound to UBuff function."));
		return false;
	}
	const bool bAdded = Direction == ECombatEventDirection::Incoming
		? IncomingBuffRestrictions.Add(Restriction) : OutgoingBuffRestrictions.Add(Restriction);
	if (bAdded)
	{
		DEBUG_LOG_BUFF_RESTRICTION_ADDED(this, Direction, Restriction);
	}
	else
	{
		DEBUG_LOG_BUFF_RESTRICTION_ADD_FAILED(this, Direction, Restriction, TEXT("Duplicate restriction in list."));
	}
	return bAdded;
}

void UBuffHandler::RemoveBuffRestriction(const ECombatEventDirection Direction, const FBuffRestriction& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const int NumRemoved = Direction == ECombatEventDirection::Incoming ? IncomingBuffRestrictions.Remove(Restriction) : OutgoingBuffRestrictions.Remove(Restriction);
	if (NumRemoved > 0)
	{
		DEBUG_LOG_BUFF_RESTRICTION_REMOVED(this, Direction, Restriction);
	}
	else
	{
		DEBUG_LOG_BUFF_RESTRICTION_REMOVE_FAILED(this, Direction, Restriction, TEXT("Restriction not found in list."));
	}
}

bool UBuffHandler::CheckBuffRestricted(const ECombatEventDirection Direction, const FBuffApplyEvent& Event, TArray<EBuffApplyFailReason>& OutFailReasons, FBuffRestriction& OutRestriction)
{
	//Don't clear the OutFailReasons array, it might have fail reasons from before this point in the application process.
	bool bRestricted = false;
	//Check any custom restrictions first.
	if (Direction == ECombatEventDirection::Incoming
		? IncomingBuffRestrictions.IsRestricted(OutRestriction, Event)
		: OutgoingBuffRestrictions.IsRestricted(OutRestriction, Event))
	{
		OutFailReasons.AddUnique(Direction == ECombatEventDirection::Incoming ? EBuffApplyFailReason::IncomingRestriction : EBuffApplyFailReason::OutgoingRestriction);
		bRestricted = true;
	}
	if (Direction == ECombatEventDirection::Incoming)
	{
		//Check if the buff can be applied to dead targets.
		if (!Event.BuffClass.GetDefaultObject()->CanBeAppliedWhileDead() && IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
		{
			OutFailReasons.AddUnique(EBuffApplyFailReason::Dead);
			bRestricted = true;
		}
		//Check if any of the buff's functions would prevent it from being applied.
		for (const UBuffFunction* Function : Event.BuffClass.GetDefaultObject()->GetAllBuffFunctions())
		{
			EBuffApplyFailReason FailReason = EBuffApplyFailReason::None;
			if (IsValid(Function) && Function->CheckBuffFunctionRestricted(Event, FailReason))
			{
				//We might forget to set a fail reason, in which case we just add the generic "BuffFunction" fail reason.
				if (FailReason == EBuffApplyFailReason::None)
				{
					OutFailReasons.AddUnique(EBuffApplyFailReason::BuffFunction);
				}
				else
				{
					OutFailReasons.AddUnique(FailReason);
				}
				bRestricted = true;
			}
		}
	}
	return bRestricted;
}

#pragma endregion
#pragma region DEBUG
#if !UE_BUILD_SHIPPING

void UBuffHandler::DEBUG_DumpState(const ECombatEventDirection Direction, TArray<FString>& OutStrings) const
{
	const FString DirectionString = UEnum::GetDisplayValueAsText(Direction).ToString().ToUpper();
	OutStrings.Add(FString::Printf(L"---BUFF HANDLER %s STATE DUMP START---", *DirectionString));
	OutStrings.Add(FString::Printf(L"Handler for actor %s has %i buffs applied.", *GetOwner()->GetName(), Direction == ECombatEventDirection::Incoming ? ActiveBuffs.Num() : OutgoingBuffs.Num()));
	switch (Direction)
	{
	case ECombatEventDirection::Incoming :
		{
			OutStrings.Add("Active Buffs: ");
			for (UBuff* Buff : ActiveBuffs)
			{
				FString& BuffString = OutStrings.Emplace_GetRef();
				Buff->DEBUG_DumpBuffState(BuffString);
			}
			OutStrings.Add("Recently Removed Buffs: ");
			for (UBuff* Buff : RecentlyRemoved)
			{
				FString& BuffString = OutStrings.Emplace_GetRef();
				Buff->DEBUG_DumpBuffState(BuffString);
			}
		}
		break;
	case ECombatEventDirection::Outgoing :
		{
			OutStrings.Add("Outgoing Buffs: ");
			for (UBuff* Buff : OutgoingBuffs)
			{
				FString& BuffString = OutStrings.Emplace_GetRef();
				Buff->DEBUG_DumpBuffState(BuffString);
			}
		}
		break;
	default :
		break;
	}
	OutStrings.Add(FString::Printf(L"---BUFF HANDLER %s STATE DUMP END---", *DirectionString));
}

void UBuffHandler::DEBUG_DumpRestrictions(const ECombatEventDirection Direction, TArray<FString>& OutStrings) const
{
	const FString DirectionString = UEnum::GetDisplayValueAsText(Direction).ToString().ToUpper();
	OutStrings.Add(FString::Printf(L"---BUFF HANDLER %s RESTRICTION DUMP START---", *DirectionString));
	OutStrings.Add(FString::Printf(L"Handler for actor %s has %i restrictions applied.", *GetOwner()->GetName(),
		Direction == ECombatEventDirection::Incoming ? IncomingBuffRestrictions.Num() : OutgoingBuffRestrictions.Num()));
	const TArray<FBuffRestriction>& Restrictions = Direction == ECombatEventDirection::Incoming
		? IncomingBuffRestrictions.GetRestrictionsList() : OutgoingBuffRestrictions.GetRestrictionsList();
	for (const FBuffRestriction& Restriction : Restrictions)
	{
		OutStrings.Add(USaiyoraCombatLibrary::DelegateToLogString(Restriction));
	}
	OutStrings.Add(FString::Printf(L"---BUFF HANDLER %s RESTRICTION DUMP END---", *DirectionString));
}

#endif
#pragma endregion 