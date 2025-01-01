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

void UPeriodicHealthEventFunction::SetupBuffFunction()
{
	TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy());
	if (bHasHealthEventCallback)
	{
		Callback.BindUFunction(GetOwningBuff(), HealthEventCallbackName);
	}
}

void UPeriodicHealthEventFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
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
    if (IsValid(TargetComponent))
    {
        const float InitAmount = bUsesSeparateInitialValue ? InitialValue : BaseValue;
        const EElementalSchool InitSchool = bUsesSeparateInitialValue ? InitialEventSchool : EventSchool;
        const bool FromSnapshot = bSnapshots && !bUsesSeparateInitialValue;
        const FHealthEvent TickResult = TargetComponent->ApplyHealthEvent(EventType, InitAmount, GetOwningBuff()->GetAppliedBy(),
                                          GetOwningBuff(), EEventHitStyle::Chronic, InitSchool, bBypassesAbsorbs, bIgnoresModifiers,
                                          bIgnoresRestrictions, false, FromSnapshot, FHealthEventModCondition(), ThreatInfo);
    	if (Callback.IsBound())
    	{
    		Callback.Execute(TickResult);
    	}
    }
}

void UPeriodicHealthEventFunction::TickHealthEvent()
{
    if (IsValid(TargetComponent))
    {
        const float TickAmount = bScalesWithStacks ? BaseValue * GetOwningBuff()->GetCurrentStacks() : BaseValue;
        const FHealthEvent TickResult = TargetComponent->ApplyHealthEvent(EventType, TickAmount, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
                                          EEventHitStyle::Chronic, EventSchool, bBypassesAbsorbs, bIgnoresModifiers,
                                          bIgnoresRestrictions, false, bSnapshots, FHealthEventModCondition(), ThreatInfo);
    	if (Callback.IsBound())
    	{
    		Callback.Execute(TickResult);
    	}
    }
}

void UPeriodicHealthEventFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
    if (IsValid(TargetComponent) && bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && EventInterval > 0.0f)
    {
    	const float TickFraction = GetWorld()->GetTimerManager().GetTimerElapsed(TickHandle) / EventInterval;
    	const float ExpireTickAmount = (bScalesWithStacks ? BaseValue * GetOwningBuff()->GetCurrentStacks() : BaseValue) * TickFraction;
    	if (ExpireTickAmount > 0.0f)
    	{
    		const FHealthEvent TickResult = TargetComponent->ApplyHealthEvent(EventType, ExpireTickAmount, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
			EEventHitStyle::Chronic, EventSchool, bBypassesAbsorbs, bIgnoresModifiers, bIgnoresRestrictions, false,
			bSnapshots, FHealthEventModCondition(), ThreatInfo);
    		if (Callback.IsBound())
    		{
    			Callback.Execute(TickResult);
    		}
    	}
    }
}

void UPeriodicHealthEventFunction::CleanupBuffFunction()
{
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
}

TArray<FName> UPeriodicHealthEventFunction::GetHealthEventCallbackFunctionNames() const
{
	return USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), FindFunction("ExampleHealthEventCallback"));
}

#pragma endregion
#pragma region Health Event Modifiers

void UHealthEventModifierFunction::SetupBuffFunction()
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	Modifier.BindUFunction(GetOwningBuff(), ModifierFunctionName);
}

void UHealthEventModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetHandler) || !Modifier.IsBound())
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
	if (!IsValid(TargetHandler) || !Modifier.IsBound())
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

TArray<FName> UHealthEventModifierFunction::GetHealthEventModifierFunctionNames() const
{
	return USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), FindFunction("ExampleModifierFunction"));
}

#pragma endregion 
#pragma region Health Event Restrictions

void UHealthEventRestrictionFunction::SetupBuffFunction()
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	Restriction.BindUFunction(GetOwningBuff(), RestrictionFunctionName);
}

void UHealthEventRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetHandler) || !Restriction.IsBound())
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
	if (!IsValid(TargetHandler) || !Restriction.IsBound())
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

TArray<FName> UHealthEventRestrictionFunction::GetHealthEventRestrictionFunctionNames() const
{
	return USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), FindFunction("ExampleRestrictionFunction"));
}

#pragma endregion
#pragma region Death Restriction

void UDeathRestrictionFunction::SetupBuffFunction()
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
	Restriction.BindUFunction(GetOwningBuff(), RestrictionFunctionName);
}

void UDeathRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetHandler) || !Restriction.IsBound())
	{
		return;
	}
	TargetHandler->AddDeathRestriction(Restriction);
}

void UDeathRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler) || !Restriction.IsBound())
	{
		return;
	}
	TargetHandler->RemoveDeathRestriction(Restriction);
}

TArray<FName> UDeathRestrictionFunction::GetDeathEventRestrictionFunctionNames() const
{
	return USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), FindFunction("ExampleRestrictionFunction"));
}

#pragma endregion
#pragma region Resurrection

void UPendingResurrectionFunction::OfferResurrection(UBuff* Buff, const FVector& ResurrectLocation,
	const bool bOverrideHealthPercent, const float HealthOverridePercent, const FResDecisionCallback& DecisionCallback)
{
	if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UPendingResurrectionFunction* NewResFunction = Cast<UPendingResurrectionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewResFunction))
	{
		return;
	}
	NewResFunction->SetResurrectionVars(ResurrectLocation, bOverrideHealthPercent, HealthOverridePercent, DecisionCallback);
}

void UPendingResurrectionFunction::SetResurrectionVars(const FVector& ResurrectLocation, const bool bOverrideHealthPercent,
	const float HealthOverridePercent, const FResDecisionCallback& DecisionCallback)
{
	if (GetOwningBuff()->GetAppliedTo()->Implements<USaiyoraCombatInterface>())
	{
		TargetPlayer = Cast<ASaiyoraPlayerCharacter>(GetOwningBuff()->GetAppliedTo());
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(TargetPlayer);
		ResLocation = ResurrectLocation;
		bOverrideHealth = bOverrideHealthPercent;
		HealthOverride = HealthOverridePercent;
		ResDecisionCallback = DecisionCallback;
		InternalResDecisionCallback.BindDynamic(this, &UPendingResurrectionFunction::OnResDecision);
	}
}

void UPendingResurrectionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetPlayer))
	{
		TargetPlayer->OfferResurrection(GetOwningBuff(), ResLocation, InternalResDecisionCallback);
	}
}

void UPendingResurrectionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetPlayer))
	{
		TargetPlayer->RescindResurrection(GetOwningBuff());
	}
}

void UPendingResurrectionFunction::OnResDecision(const bool bAccepted)
{
	if (bAccepted)
	{
		if (IsValid(TargetHandler))
		{
			TargetHandler->RespawnActor(true, ResLocation, bOverrideHealth, HealthOverride);
		}
	}
	if (ResDecisionCallback.IsBound())
	{
		ResDecisionCallback.Execute(bAccepted);
	}
}

#pragma endregion 
