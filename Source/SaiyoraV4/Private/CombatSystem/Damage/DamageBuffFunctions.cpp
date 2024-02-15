#include "DamageBuffFunctions.h"
#include "AbilityBuffFunctions.h"
#include "AbilityFunctionLibrary.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "DamageHandler.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"

#pragma region Periodic Health Event

void UPeriodicHealthEventFunction::PeriodicHealthEvent(UBuff* Buff, const EHealthEventType HealthEventType, const float Amount, const EElementalSchool School, const float Interval,
	const bool bBypassAbsorbs, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const bool bSnapshots, const bool bScaleWithStacks,
	const bool bPartialTickOnExpire, const bool bInitialTick, const bool bUseSeparateInitialAmount,
	const float InitialAmount, const EElementalSchool InitialSchool, const FThreatFromDamage& ThreatParams, const FHealthEventCallback& TickCallback)
{
	if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UPeriodicHealthEventFunction* NewDotFunction = Cast<UPeriodicHealthEventFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDotFunction))
	{
		return;
	}
	NewDotFunction->SetEventVars(HealthEventType, Amount, School, Interval, bBypassAbsorbs, bIgnoreRestrictions, bIgnoreModifiers, bSnapshots,
		bScaleWithStacks, bPartialTickOnExpire, bInitialTick, bUseSeparateInitialAmount, InitialAmount, InitialSchool, ThreatParams, TickCallback);
}

void UPeriodicHealthEventFunction::SetEventVars(const EHealthEventType HealthEventType, const float Amount, const EElementalSchool School, const float Interval,
	const bool bBypassAbsorbs, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const bool bSnapshot, const bool bScaleWithStacks,
	const bool bPartialTickOnExpire, const bool bInitialTick, const bool bUseSeparateInitialAmount,
	const float InitialAmount, const EElementalSchool InitialSchool, const FThreatFromDamage& ThreatParams, const FHealthEventCallback& TickCallback)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		if (IsValid(GetOwningBuff()->GetAppliedBy()) && GetOwningBuff()->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy());
		}
		EventType = HealthEventType;
		BaseValue = Amount;
		EventSchool = School;
		EventInterval = Interval;
		bBypassesAbsorbs = bBypassAbsorbs;
		bIgnoresRestrictions = bIgnoreRestrictions;
		bIgnoresModifiers = bIgnoreModifiers;
		bSnapshots = bSnapshot;
		bScalesWithStacks = bScaleWithStacks;
		bTicksOnExpire = bPartialTickOnExpire;
		bHasInitialTick = bInitialTick;
		bUsesSeparateInitialValue = bUseSeparateInitialAmount;
		InitialValue = InitialAmount;
		InitialEventSchool = InitialSchool;
		ThreatInfo = ThreatParams;
		Callback = TickCallback;
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

#pragma endregion
#pragma region Health Event Modifiers

void UHealthEventModifierFunction::HealthEventModifier(UBuff* Buff, const ECombatEventDirection HealthEventDirection, const FHealthEventModCondition& Modifier)
{
	if (!IsValid(Buff) || !Modifier.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UHealthEventModifierFunction* NewDamageModifierFunction = Cast<UHealthEventModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDamageModifierFunction))
	{
		return;
	}
	NewDamageModifierFunction->SetModifierVars(HealthEventDirection, Modifier);
}

void UHealthEventModifierFunction::SetModifierVars(const ECombatEventDirection HealthEventDirection, const FHealthEventModCondition& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		EventDirection = HealthEventDirection;
		Mod = Modifier;
	}
}

void UHealthEventModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (EventDirection)
		{
		case ECombatEventDirection::Incoming :
			TargetHandler->AddIncomingHealthEventModifier(Mod);
			break;
		case ECombatEventDirection::Outgoing :
			TargetHandler->AddOutgoingHealthEventModifier(Mod);
			break;
		default :
			break;
		}
	}
}

void UHealthEventModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (EventDirection)
		{
		case ECombatEventDirection::Incoming :
			TargetHandler->RemoveIncomingHealthEventModifier(Mod);
			break;
		case ECombatEventDirection::Outgoing :
			TargetHandler->RemoveOutgoingHealthEventModifier(Mod);
			break;
		default :
			break;
		}
	}
}

#pragma endregion 
#pragma region Health Event Restrictions

void UHealthEventRestrictionFunction::HealthEventRestriction(UBuff* Buff, const ECombatEventDirection HealthEventDirection, const FHealthEventRestriction& Restriction)
{
	if (!IsValid(Buff) || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UHealthEventRestrictionFunction* NewDamageRestrictionFunction = Cast<UHealthEventRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDamageRestrictionFunction))
	{
		return;
	}
	NewDamageRestrictionFunction->SetRestrictionVars(HealthEventDirection, Restriction);
}

void UHealthEventRestrictionFunction::SetRestrictionVars(const ECombatEventDirection HealthEventDirection, const FHealthEventRestriction& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		EventDirection = HealthEventDirection;
		Restrict = Restriction;
	}
}

void UHealthEventRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (EventDirection)
		{
		case ECombatEventDirection::Incoming :
			TargetHandler->AddIncomingHealthEventRestriction(Restrict);
			break;
		case ECombatEventDirection::Outgoing :
			TargetHandler->AddOutgoingHealthEventRestriction(Restrict);
			break;
		default :
			break;
		}
	}
}

void UHealthEventRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (EventDirection)
		{
		case ECombatEventDirection::Incoming :
			TargetHandler->RemoveIncomingHealthEventRestriction(Restrict);
			break;
		case ECombatEventDirection::Outgoing :
			TargetHandler->RemoveOutgoingHealthEventRestriction(Restrict);
			break;
		default :
			break;
		}
	}
}

#pragma endregion
#pragma region Death Restriction

void UDeathRestrictionFunction::DeathRestriction(UBuff* Buff, const FDeathRestriction& Restriction)
{
	if (!IsValid(Buff) || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UDeathRestrictionFunction* NewDeathRestrictionFunction = Cast<UDeathRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDeathRestrictionFunction))
	{
		return;
	}
	NewDeathRestrictionFunction->SetRestrictionVars(Restriction);
}

void UDeathRestrictionFunction::SetRestrictionVars(const FDeathRestriction& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		Restrict = Restriction;
	}
}

void UDeathRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->AddDeathRestriction(Restrict);
	}
}

void UDeathRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->RemoveDeathRestriction(Restrict);
	}
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
