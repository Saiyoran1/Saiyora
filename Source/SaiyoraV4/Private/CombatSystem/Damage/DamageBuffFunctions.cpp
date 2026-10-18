#include "DamageBuffFunctions.h"
#include "AbilityBuffFunctions.h"
#include "AbilityFunctionLibrary.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "DamageHandler.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

#pragma region Periodic Health Event

UPeriodicHealthEventFunction::UPeriodicHealthEventFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UPeriodicHealthEventFunction, Callback));
}

void UPeriodicHealthEventFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	const UDamageHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy());
	if (bSnapshots && !bIgnoresModifiers && IsValid(GeneratorComponent))
	{
		FHealthEventInfo SnapshotInfo;
		SnapshotInfo.EventType = EventType;
		SnapshotInfo.Value = BaseValue;
		SnapshotInfo.Source = GetOwningBuff();
		SnapshotInfo.AppliedBy = GetOwningBuff()->GetAppliedBy();
		SnapshotInfo.AppliedTo = GetOwningBuff()->GetAppliedTo();
		if (SnapshotInfo.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			const UCombatStatusComponent* AppliedByCombatStatusComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(SnapshotInfo.AppliedBy);
			SnapshotInfo.AppliedByPlane = IsValid(AppliedByCombatStatusComp) ? AppliedByCombatStatusComp->GetCurrentPlane() : ESaiyoraPlane::Both;
		}
		else
		{
			SnapshotInfo.AppliedByPlane = ESaiyoraPlane::Both;
		}
		if (SnapshotInfo.AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			const UCombatStatusComponent* AppliedToCombatStatusComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(SnapshotInfo.AppliedTo);
			SnapshotInfo.AppliedToPlane = IsValid(AppliedToCombatStatusComp) ? AppliedToCombatStatusComp->GetCurrentPlane() : ESaiyoraPlane::Both;
		}
		else
		{
			SnapshotInfo.AppliedToPlane = ESaiyoraPlane::Both;
		}
		SnapshotInfo.AppliedXPlane = UAbilityFunctionLibrary::IsXPlane(SnapshotInfo.AppliedByPlane, SnapshotInfo.AppliedToPlane);
		SnapshotInfo.HitStyle = EEventHitStyle::Chronic;
		SnapshotInfo.School = EventSchool;
		BaseValue = GeneratorComponent->GetModifiedOutgoingHealthEventValue(SnapshotInfo, FHealthEventModCondition());
	}
	if (bHasInitialTick)
	{
		InitialTick();
	}
	if (EventInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UPeriodicHealthEventFunction::TickHealthEvent, EventInterval, true);
	}
}

void UPeriodicHealthEventFunction::InitialTick()
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
    const float InitAmount = bUsesSeparateInitialValue ? InitialValue : BaseValue;
    const EElementalSchool InitSchool = bUsesSeparateInitialValue ? InitialEventSchool : EventSchool;
    const bool FromSnapshot = bSnapshots && !bUsesSeparateInitialValue;
    const FHealthEvent TickResult = TargetHandler->ApplyHealthEvent(EventType, InitAmount, GetOwningBuff()->GetAppliedBy(),
    GetOwningBuff(), EEventHitStyle::Chronic, InitSchool, bBypassesAbsorbs, bIgnoresModifiers,
    bIgnoresRestrictions, false, FromSnapshot, FHealthEventModCondition(), ThreatInfo);
	//This returns a bool and Rider is mad if I don't save the return value.
    const bool bExecuted = Callback.ExecuteIfBound(TickResult);
}

void UPeriodicHealthEventFunction::TickHealthEvent()
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
    const float TickAmount = bScalesWithStacks ? BaseValue * GetOwningBuff()->GetCurrentStacks() : BaseValue;
    const FHealthEvent TickResult = TargetHandler->ApplyHealthEvent(EventType, TickAmount, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
		EEventHitStyle::Chronic, EventSchool, bBypassesAbsorbs, bIgnoresModifiers,
		bIgnoresRestrictions, false, bSnapshots, FHealthEventModCondition(), ThreatInfo);
	//This returns a bool and Rider is mad if I don't save the return value.
    const bool bExecuted = Callback.ExecuteIfBound(TickResult);
}

void UPeriodicHealthEventFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		//Clear the handle here in case the target became null during the buff's lifetime.
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		return;
	}
    if (bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && EventInterval > 0.0f)
    {
    	const float TickFraction = GetWorld()->GetTimerManager().GetTimerElapsed(TickHandle) / EventInterval;
    	const float ExpireTickAmount = (bScalesWithStacks ? BaseValue * GetOwningBuff()->GetCurrentStacks() : BaseValue) * TickFraction;
    	if (ExpireTickAmount > 0.0f)
    	{
    		const FHealthEvent TickResult = TargetHandler->ApplyHealthEvent(EventType, ExpireTickAmount, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
				EEventHitStyle::Chronic, EventSchool, bBypassesAbsorbs, bIgnoresModifiers, bIgnoresRestrictions, false,
				bSnapshots, FHealthEventModCondition(), ThreatInfo);
    		//This returns a bool and Rider is mad if I don't save the return value.
    		const bool bExecuted = Callback.ExecuteIfBound(TickResult);
    	}
    }
	GetWorld()->GetTimerManager().ClearTimer(TickHandle);
}

bool UPeriodicHealthEventFunction::CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const
{
	const UDamageHandler* DamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(ApplyEvent.AppliedTo);
	if (!IsValid(DamageHandler)
		|| (EventType == EHealthEventType::Damage && !DamageHandler->CanEverReceiveDamage())
		|| ((EventType == EHealthEventType::Absorb || EventType == EHealthEventType::Healing) && !DamageHandler->CanEverReceiveHealing()))
	{
		OutFailReason = EBuffApplyFailReason::ImmuneToHealthEvent;
		return true;
	}
	return false;
}

#pragma endregion
#pragma region Health Event Modifiers

UHealthEventModifierFunction::UHealthEventModifierFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UHealthEventModifierFunction, Modifier));
}

void UHealthEventModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		TargetHandler->AddIncomingHealthEventModifier(Modifier);
		break;
	case ECombatEventDirection::Outgoing :
		TargetHandler->AddOutgoingHealthEventModifier(Modifier);
		break;
	default :
		break;
	}
}

void UHealthEventModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		TargetHandler->RemoveIncomingHealthEventModifier(Modifier);
		break;
	case ECombatEventDirection::Outgoing :
		TargetHandler->RemoveOutgoingHealthEventModifier(Modifier);
		break;
	default :
		break;
	}
}

#pragma endregion 
#pragma region Health Event Restrictions

UHealthEventRestrictionFunction::UHealthEventRestrictionFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UHealthEventRestrictionFunction, Restriction));
}

void UHealthEventRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		TargetHandler->AddIncomingHealthEventRestriction(Restriction);
		break;
	case ECombatEventDirection::Outgoing :
		TargetHandler->AddOutgoingHealthEventRestriction(Restriction);
		break;
	default :
		break;
	}
}

void UHealthEventRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		TargetHandler->RemoveIncomingHealthEventRestriction(Restriction);
			break;
	case ECombatEventDirection::Outgoing :
		TargetHandler->RemoveOutgoingHealthEventRestriction(Restriction);
		break;
	default :
		break;
	}
}

#pragma endregion
#pragma region Death Restriction

UDeathRestrictionFunction::UDeathRestrictionFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UDeathRestrictionFunction, Restriction));
}

void UDeathRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->AddDeathRestriction(Restriction);
}

void UDeathRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->RemoveDeathRestriction(Restriction);
}

#pragma endregion
#pragma region Resurrection

void UPendingResurrectionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	//Get the res location that was passed in as a buff parameter.
	//If one isn't found, then we will just use the location of the actor applying the buff.
	if (!USaiyoraCombatLibrary::GetVectorParam(ApplyEvent.CombatParams, "ResLocation", ResLocation))
	{
		ResLocation = GetOwningBuff()->GetAppliedBy()->GetActorLocation();
	}
	TargetHandler->OnLifeStatusChanged.AddDynamic(this, &UPendingResurrectionFunction::OnLifeStatusChanged);
	ResID = TargetHandler->AddPendingResurrection(this);
}

void UPendingResurrectionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	TargetHandler->RemovePendingResurrection(ResID);
	TargetHandler->OnLifeStatusChanged.RemoveDynamic(this, &UPendingResurrectionFunction::OnLifeStatusChanged);
}

void UPendingResurrectionFunction::OnLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (NewStatus == ELifeStatus::Alive)
	{
		GetOwningBuff()->GetHandler()->RemoveBuff(GetOwningBuff(), EBuffExpireReason::Dispel);
	}
}

#pragma endregion 
