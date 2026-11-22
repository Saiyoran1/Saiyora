#include "DamageBuffFunctions.h"
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
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	const UDamageHandler* GeneratorComponent = IsValid(GetOwningBuff()->GetAppliedBy()) && GetOwningBuff()->GetAppliedBy()->Implements<USaiyoraCombatInterface>()
		? ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy()) : nullptr;
	if (bSnapshots && !bIgnoresModifiers && IsValid(GeneratorComponent))
	{
		FHealthEventInfo SnapshotInfo;
		SnapshotInfo.EventType = EventType;
		SnapshotInfo.Value = BaseValue;
		SnapshotInfo.Source = GetOwningBuff();
		SnapshotInfo.AppliedBy = GetOwningBuff()->GetAppliedBy();
		SnapshotInfo.AppliedTo = GetOwningBuff()->GetAppliedTo();
		const UCombatStatusComponent* AppliedByCombatStatusComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(SnapshotInfo.AppliedBy);
		SnapshotInfo.AppliedByPlane = IsValid(AppliedByCombatStatusComp) ? AppliedByCombatStatusComp->GetCurrentPlane() : ESaiyoraPlane::Both;
		const UCombatStatusComponent* AppliedToCombatStatusComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(SnapshotInfo.AppliedTo);
		SnapshotInfo.AppliedToPlane = IsValid(AppliedToCombatStatusComp) ? AppliedToCombatStatusComp->GetCurrentPlane() : ESaiyoraPlane::Both;
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
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
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
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
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
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
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
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	switch (EventDirection)
	{
	case ECombatEventDirection::Incoming :
		bSuccess = TargetHandler->AddIncomingHealthEventRestriction(Restriction);
		break;
	case ECombatEventDirection::Outgoing :
		bSuccess = TargetHandler->AddOutgoingHealthEventRestriction(Restriction);
		break;
	default :
		break;
	}
}

void UHealthEventRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
	if (!IsValid(TargetHandler) || !bSuccess)
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
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	bSuccess = TargetHandler->AddDeathRestriction(Restriction);
}

void UDeathRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
	if (!IsValid(TargetHandler) || !bSuccess)
	{
		return;
	}
	TargetHandler->RemoveDeathRestriction(Restriction);
}

#pragma endregion
#pragma region Resurrection

UPendingResurrectionFunction::UPendingResurrectionFunction()
{
	SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UPendingResurrectionFunction, Callback));
}

void UPendingResurrectionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	//Get the res location that was passed in as a buff parameter.
	//If one isn't found, then we will just use the location of the actor applying the buff.
	if (!USaiyoraCombatLibrary::GetVectorParam(ApplyEvent.CombatParams, FSaiyoraCombatTags::Get().Param_ResurrectLocation, ResLocation))
	{
		ResLocation = GetOwningBuff()->GetAppliedBy()->GetActorLocation();
	}
	TargetHandler->OnLifeStatusChanged.AddDynamic(this, &UPendingResurrectionFunction::OnLifeStatusChanged);
	bSuccess = TargetHandler->AddPendingResurrection(this);
}

void UPendingResurrectionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!GetOwningBuff()->HasAuthority())
	{
		return;
	}
	if (!IsValid(TargetHandler))
	{
		return;
	}
	if (bSuccess)
	{
		TargetHandler->RemovePendingResurrection(this);
	}
	TargetHandler->OnLifeStatusChanged.RemoveDynamic(this, &UPendingResurrectionFunction::OnLifeStatusChanged);
}

bool UPendingResurrectionFunction::CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const
{
	//Can only be applied to things that are dead.
	const UDamageHandler* DamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(ApplyEvent.AppliedTo);
	if (IsValid(DamageHandler) && DamageHandler->GetLifeStatus() == ELifeStatus::Dead)
	{
		return false;
	}
	return true;
}

void UPendingResurrectionFunction::OnResDecisionMade(const bool bAccepted)
{
	if (Callback.IsBound())
	{
		Callback.Execute(bAccepted, ResLocation);
	}
}

void UPendingResurrectionFunction::OnLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (NewStatus == ELifeStatus::Alive)
	{
		GetOwningBuff()->GetHandler()->RemoveBuff(GetOwningBuff(), EBuffExpireReason::Absolute);
	}
}

#pragma endregion 
