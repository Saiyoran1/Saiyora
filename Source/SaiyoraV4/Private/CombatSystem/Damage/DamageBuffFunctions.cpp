#include "DamageBuffFunctions.h"
#include "AbilityBuffFunctions.h"
#include "Buff.h"
#include "DamageHandler.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"

#pragma region Periodic Health Event

void UPeriodicHealthEventFunction::PeriodicHealthEvent(UBuff* Buff, const float Amount, const EHealthEventSchool School, const float Interval,
	const bool bBypassAbsorbs, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const bool bSnapshots, const bool bScaleWithStacks,
	const bool bPartialTickOnExpire, const bool bInitialTick, const bool bUseSeparateInitialAmount,
	const float InitialAmount, const EHealthEventSchool InitialSchool, const FThreatFromDamage& ThreatParams)
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
	NewDotFunction->SetEventVars(Amount, School, Interval, bBypassAbsorbs, bIgnoreRestrictions, bIgnoreModifiers, bSnapshots,
		bScaleWithStacks, bPartialTickOnExpire, bInitialTick, bUseSeparateInitialAmount, InitialAmount, InitialSchool, ThreatParams);
}

void UPeriodicHealthEventFunction::SetEventVars(const float Amount, const EHealthEventSchool School, const float Interval,
	const bool bBypassAbsorbs, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const bool bSnapshot, const bool bScaleWithStacks,
	const bool bPartialTickOnExpire, const bool bInitialTick, const bool bUseSeparateInitialAmount,
	const float InitialAmount, const EHealthEventSchool InitialSchool, const FThreatFromDamage& ThreatParams)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		if (IsValid(GetOwningBuff()->GetAppliedBy()) && GetOwningBuff()->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy());
		}
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
			const UPlaneComponent* AppliedByPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(SnapshotInfo.AppliedBy);
			SnapshotInfo.AppliedByPlane = IsValid(AppliedByPlaneComp) ? AppliedByPlaneComp->GetCurrentPlane() : ESaiyoraPlane::None;
		}
		else
		{
			SnapshotInfo.AppliedByPlane = ESaiyoraPlane::None;
		}
		if (SnapshotInfo.AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			const UPlaneComponent* AppliedToPlaneComp = ISaiyoraCombatInterface::Execute_GetPlaneComponent(SnapshotInfo.AppliedTo);
			SnapshotInfo.AppliedToPlane = IsValid(AppliedToPlaneComp) ? AppliedToPlaneComp->GetCurrentPlane() : ESaiyoraPlane::None;
		}
		else
		{
			SnapshotInfo.AppliedToPlane = ESaiyoraPlane::None;
		}
		SnapshotInfo.AppliedXPlane = UPlaneComponent::CheckForXPlane(SnapshotInfo.AppliedByPlane, SnapshotInfo.AppliedToPlane);
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
        const EHealthEventSchool InitSchool = bUsesSeparateInitialValue ? InitialEventSchool : EventSchool;
        const bool FromSnapshot = bSnapshots && !bUsesSeparateInitialValue;
        TargetComponent->ApplyHealthEvent(EventType, InitAmount, GetOwningBuff()->GetAppliedBy(),
                                          GetOwningBuff(), EEventHitStyle::Chronic, InitSchool, bBypassesAbsorbs, bIgnoresModifiers,
                                          bIgnoresRestrictions, false, FromSnapshot, FHealthEventModCondition(), ThreatInfo);
    }
}

void UPeriodicHealthEventFunction::TickHealthEvent()
{
    if (IsValid(TargetComponent))
    {
        const float TickAmount = bScalesWithStacks ? BaseValue * GetOwningBuff()->GetCurrentStacks() : BaseValue;
        TargetComponent->ApplyHealthEvent(EventType, TickAmount, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
                                          EEventHitStyle::Chronic, EventSchool, bBypassesAbsorbs, bIgnoresModifiers,
                                          bIgnoresRestrictions, false, bSnapshots, FHealthEventModCondition(), ThreatInfo);
    }
}

void UPeriodicHealthEventFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
    if (IsValid(TargetComponent) && bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && EventInterval > 0.0f)
    {
        if (const float TickFraction = GetWorld()->GetTimerManager().GetTimerRemaining(TickHandle) / EventInterval > 0.0f)
        {
            const float ExpireTickAmount = (bScalesWithStacks ? BaseValue * GetOwningBuff()->GetCurrentStacks() : BaseValue) * TickFraction;
            TargetComponent->ApplyHealthEvent(EventType, ExpireTickAmount,GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
				EEventHitStyle::Chronic, EventSchool, bBypassesAbsorbs, bIgnoresModifiers, bIgnoresRestrictions, false,
				bSnapshots, FHealthEventModCondition(), ThreatInfo);
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
	if (!IsValid(Buff) || HealthEventDirection == ECombatEventDirection::None || !Modifier.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
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
			TargetHandler->AddIncomingHealthEventModifier(GetOwningBuff(), Mod);
			break;
		case ECombatEventDirection::Outgoing :
			TargetHandler->AddOutgoingHealthEventModifier(GetOwningBuff(), Mod);
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
			TargetHandler->RemoveIncomingHealthEventModifier(GetOwningBuff());
			break;
		case ECombatEventDirection::Outgoing :
			TargetHandler->RemoveOutgoingHealthEventModifier(GetOwningBuff());
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
	if (!IsValid(Buff) || HealthEventDirection == ECombatEventDirection::None || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
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
			TargetHandler->AddIncomingHealthEventRestriction(GetOwningBuff(), Restrict);
			break;
		case ECombatEventDirection::Outgoing :
			TargetHandler->AddOutgoingHealthEventRestriction(GetOwningBuff(), Restrict);
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
			TargetHandler->RemoveIncomingHealthEventRestriction(GetOwningBuff());
			break;
		case ECombatEventDirection::Outgoing :
			TargetHandler->RemoveOutgoingHealthEventRestriction(GetOwningBuff());
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
		TargetHandler->AddDeathRestriction(GetOwningBuff(), Restrict);
	}
}

void UDeathRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->RemoveDeathRestriction(GetOwningBuff());
	}
}

#pragma endregion