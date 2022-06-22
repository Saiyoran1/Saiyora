#include "DamageBuffFunctions.h"
#include "AbilityBuffFunctions.h"
#include "Buff.h"
#include "DamageHandler.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"

#pragma region Damage Over Time

void UDamageOverTimeFunction::DamageOverTime(UBuff* Buff, const float Damage, const EDamageSchool School, const float Interval,
	const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const bool bSnapshots, const bool bScalesWithStacks,
	const bool bPartialTickOnExpire, const bool bHasInitialTick, const bool bUseSeparateInitialDamage,
	const float InitialDamage, const EDamageSchool InitialSchool, const FThreatFromDamage& ThreatParams)
{
	if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UDamageOverTimeFunction* NewDotFunction = Cast<UDamageOverTimeFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDotFunction))
	{
		return;
	}
	NewDotFunction->SetDamageVars(Damage, School, Interval, bIgnoreRestrictions, bIgnoreModifiers, bSnapshots,
		bScalesWithStacks, bPartialTickOnExpire, bHasInitialTick, bUseSeparateInitialDamage, InitialDamage, InitialSchool, ThreatParams);
}

void UDamageOverTimeFunction::SetDamageVars(const float Damage, const EDamageSchool School, const float Interval,
	const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const bool bSnapshot, const bool bScaleWithStacks,
	const bool bPartialTickOnExpire, const bool bInitialTick, const bool bUseSeparateInitialDamage,
	const float InitialDamage, const EDamageSchool InitialSchool, const FThreatFromDamage& ThreatParams)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		if (IsValid(GetOwningBuff()->GetAppliedBy()) && GetOwningBuff()->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy());
		}
		BaseDamage = Damage;
		DamageSchool = School;
		DamageInterval = Interval;
		bIgnoresRestrictions = bIgnoreRestrictions;
		bIgnoresModifiers = bIgnoreModifiers;
		bSnapshots = bSnapshot;
		bScalesWithStacks = bScaleWithStacks;
		bTicksOnExpire = bPartialTickOnExpire;
		bHasInitialTick = bInitialTick;
		bUsesSeparateInitialDamage = bUseSeparateInitialDamage;
		InitialDamageAmount = InitialDamage;
		InitialDamageSchool = InitialSchool;
		ThreatInfo = ThreatParams;
	}
}

void UDamageOverTimeFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (bSnapshots && !bIgnoresModifiers && IsValid(GeneratorComponent))
	{
		FDamageInfo SnapshotInfo;
		SnapshotInfo.Value = BaseDamage;
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
		SnapshotInfo.HitStyle = EDamageHitStyle::Chronic;
		SnapshotInfo.School = DamageSchool;
		BaseDamage = GeneratorComponent->GetModifiedOutgoingDamage(SnapshotInfo, FDamageModCondition());
	}
	if (bHasInitialTick)
	{
		InitialTick();
	}
	if (DamageInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UDamageOverTimeFunction::TickDamage, DamageInterval, true);
	}
}

void UDamageOverTimeFunction::InitialTick()
{
    if (IsValid(TargetComponent))
    {
        const float InitDamage = bUsesSeparateInitialDamage ? InitialDamageAmount : BaseDamage;
        const EDamageSchool InitSchool = bUsesSeparateInitialDamage ? InitialDamageSchool : DamageSchool;
        const bool FromSnapshot = bSnapshots && !bUsesSeparateInitialDamage;
        TargetComponent->ApplyDamage(InitDamage, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
			EDamageHitStyle::Chronic, InitSchool, bIgnoresModifiers, bIgnoresRestrictions, false,
			FromSnapshot, FDamageModCondition(), ThreatInfo);
    }
}

void UDamageOverTimeFunction::TickDamage()
{
    if (IsValid(TargetComponent))
    {
        const float TickDamage = bScalesWithStacks ? BaseDamage * GetOwningBuff()->GetCurrentStacks() : BaseDamage;
        TargetComponent->ApplyDamage(TickDamage, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(), EDamageHitStyle::Chronic,
                                     DamageSchool, bIgnoresModifiers, bIgnoresRestrictions, false,
                                     bSnapshots, FDamageModCondition(), ThreatInfo);
    }
}

void UDamageOverTimeFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
    if (IsValid(TargetComponent) && bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && DamageInterval > 0.0f)
    {
    	const float TickFraction = GetWorld()->GetTimerManager().GetTimerRemaining(TickHandle) / DamageInterval;
        if (TickFraction > 0.0f)
        {
            const float ExpireTickDamage = (bScalesWithStacks ? BaseDamage * GetOwningBuff()->GetCurrentStacks() : BaseDamage) * TickFraction;
            TargetComponent->ApplyDamage(ExpireTickDamage, GetOwningBuff()->GetAppliedBy(),GetOwningBuff(), EDamageHitStyle::Chronic,
                                         DamageSchool, bIgnoresModifiers, bIgnoresRestrictions, false, bSnapshots, FDamageModCondition(), ThreatInfo);
        }
    }
}

void UDamageOverTimeFunction::CleanupBuffFunction()
{
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
}

#pragma endregion
#pragma region Healing Over Time

void UHealingOverTimeFunction::HealingOverTime(UBuff* Buff, const float Healing, const EDamageSchool School, const float Interval,
	const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const bool bSnapshots, const bool bScalesWithStacks,
	const bool bPartialTickOnExpire, const bool bHasInitialTick, const bool bUseSeparateInitialHealing,
	const float InitialHealing, const EDamageSchool InitialSchool, const FThreatFromDamage& ThreatParams)
{
	if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UHealingOverTimeFunction* NewHotFunction = Cast<UHealingOverTimeFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewHotFunction))
	{
		return;
	}
	NewHotFunction->SetHealingVars(Healing, School, Interval, bIgnoreRestrictions, bIgnoreModifiers, bSnapshots,
		bScalesWithStacks, bPartialTickOnExpire, bHasInitialTick, bUseSeparateInitialHealing, InitialHealing, InitialSchool, ThreatParams);
}

void UHealingOverTimeFunction::SetHealingVars(const float Healing, const EDamageSchool School, const float Interval,
	const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const bool bSnapshot, const bool bScaleWithStacks,
	const bool bPartialTickOnExpire, const bool bInitialTick, const bool bUseSeparateInitialHealing,
	const float InitialHealing, const EDamageSchool InitialSchool, const FThreatFromDamage& ThreatParams)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		if (IsValid(GetOwningBuff()->GetAppliedBy()) && GetOwningBuff()->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy());
		}
		BaseHealing = Healing;
		HealingSchool = School;
		HealingInterval = Interval;
		bIgnoresRestrictions = bIgnoreRestrictions;
		bIgnoresModifiers = bIgnoreModifiers;
		bSnapshots = bSnapshot;
		bScalesWithStacks = bScaleWithStacks;
		bTicksOnExpire = bPartialTickOnExpire;
		bHasInitialTick = bInitialTick;
		bUsesSeparateInitialHealing = bUseSeparateInitialHealing;
		InitialHealingAmount = InitialHealing;
		InitialHealingSchool = InitialSchool;
		ThreatInfo = ThreatParams;
	}
}

void UHealingOverTimeFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (bSnapshots && !bIgnoresModifiers && IsValid(GeneratorComponent))
	{
		FDamageInfo SnapshotInfo;
		SnapshotInfo.Value = BaseHealing;
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
		SnapshotInfo.HitStyle = EDamageHitStyle::Chronic;
		SnapshotInfo.School = HealingSchool;
		BaseHealing = GeneratorComponent->GetModifiedOutgoingHealing(SnapshotInfo, FDamageModCondition());
	}
	if (bHasInitialTick)
	{
		InitialTick();
	}
	if (HealingInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UHealingOverTimeFunction::TickHealing, HealingInterval, true);
	}
}

void UHealingOverTimeFunction::InitialTick()
{
    if (IsValid(TargetComponent))
    {
        const float InitHealing = bUsesSeparateInitialHealing ? InitialHealingAmount : BaseHealing;
        const EDamageSchool InitSchool = bUsesSeparateInitialHealing ? InitialHealingSchool : HealingSchool;
        const bool FromSnapshot = bSnapshots && !bUsesSeparateInitialHealing;
        TargetComponent->ApplyHealing(InitHealing, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
            EDamageHitStyle::Chronic, InitSchool, bIgnoresRestrictions, bIgnoresModifiers, FromSnapshot, FDamageModCondition(), ThreatInfo);
    }
}

void UHealingOverTimeFunction::TickHealing()
{
    if (IsValid(TargetComponent))
    {
        const float TickHealing = bScalesWithStacks ? BaseHealing * GetOwningBuff()->GetCurrentStacks() : BaseHealing;
        TargetComponent->ApplyHealing(TickHealing, GetOwningBuff()->GetAppliedBy(),GetOwningBuff(), EDamageHitStyle::Chronic,
            HealingSchool, bIgnoresRestrictions,bIgnoresModifiers, bSnapshots, FDamageModCondition(), ThreatInfo);
    }
}

void UHealingOverTimeFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
    if (IsValid(TargetComponent) && bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && HealingInterval > 0.0f)
    {
    	const float TickFraction = GetWorld()->GetTimerManager().GetTimerRemaining(TickHandle) / HealingInterval;
        if (TickFraction > 0.0f)
        {
            const float ExpireTickHealing = (bScalesWithStacks ? BaseHealing * GetOwningBuff()->GetCurrentStacks() : BaseHealing) * TickFraction;
            TargetComponent->ApplyHealing(ExpireTickHealing, GetOwningBuff()->GetAppliedBy(),GetOwningBuff(), EDamageHitStyle::Chronic,
                HealingSchool, bIgnoresRestrictions,bIgnoresModifiers, bSnapshots, FDamageModCondition(), ThreatInfo);
        }
    }
}

void UHealingOverTimeFunction::CleanupBuffFunction()
{
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
}

#pragma endregion 
#pragma region Damage Modifiers

void UDamageModifierFunction::DamageModifier(UBuff* Buff, const EDamageModifierType ModifierType,
                                             const FDamageModCondition& Modifier)
{
	if (!IsValid(Buff) || ModifierType == EDamageModifierType::None || !Modifier.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UDamageModifierFunction* NewDamageModifierFunction = Cast<UDamageModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDamageModifierFunction))
	{
		return;
	}
	NewDamageModifierFunction->SetModifierVars(ModifierType, Modifier);
}

void UDamageModifierFunction::SetModifierVars(const EDamageModifierType ModifierType,
                                              const FDamageModCondition& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		ModType = ModifierType;
		Mod = Modifier;
	}
}

void UDamageModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
		case EDamageModifierType::IncomingDamage :
			TargetHandler->AddIncomingDamageModifier(GetOwningBuff(), Mod);
			break;
		case EDamageModifierType::OutgoingDamage :
			TargetHandler->AddOutgoingDamageModifier(GetOwningBuff(), Mod);
			break;
		case EDamageModifierType::IncomingHealing :
			TargetHandler->AddIncomingHealingModifier(GetOwningBuff(), Mod);
			break;
		case EDamageModifierType::OutgoingHealing :
			TargetHandler->AddOutgoingHealingModifier(GetOwningBuff(), Mod);
			break;
		default :
			break;
		}
	}
}

void UDamageModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
		case EDamageModifierType::IncomingDamage :
			TargetHandler->RemoveIncomingDamageModifier(GetOwningBuff());
			break;
		case EDamageModifierType::OutgoingDamage :
			TargetHandler->RemoveOutgoingDamageModifier(GetOwningBuff());
			break;
		case EDamageModifierType::IncomingHealing :
			TargetHandler->RemoveIncomingHealingModifier(GetOwningBuff());
			break;
		case EDamageModifierType::OutgoingHealing :
			TargetHandler->RemoveOutgoingHealingModifier(GetOwningBuff());
			break;
		default :
			break;
		}
	}
}

#pragma endregion 
#pragma region Damage Restrictions

void UDamageRestrictionFunction::DamageRestriction(UBuff* Buff, const EDamageModifierType RestrictionType,
                                                   const FDamageRestriction& Restriction)
{
	if (!IsValid(Buff) || RestrictionType == EDamageModifierType::None || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UDamageRestrictionFunction* NewDamageRestrictionFunction = Cast<UDamageRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewDamageRestrictionFunction))
	{
		return;
	}
	NewDamageRestrictionFunction->SetRestrictionVars(RestrictionType, Restriction);
}

void UDamageRestrictionFunction::SetRestrictionVars(const EDamageModifierType RestrictionType,
                                                    const FDamageRestriction& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
		RestrictType = RestrictionType;
		Restrict = Restriction;
	}
}

void UDamageRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (RestrictType)
		{
		case EDamageModifierType::IncomingDamage :
			TargetHandler->AddIncomingDamageRestriction(GetOwningBuff(), Restrict);
			break;
		case EDamageModifierType::OutgoingDamage :
			TargetHandler->AddOutgoingDamageRestriction(GetOwningBuff(), Restrict);
			break;
		case EDamageModifierType::IncomingHealing :
			TargetHandler->AddIncomingHealingRestriction(GetOwningBuff(), Restrict);
			break;
		case EDamageModifierType::OutgoingHealing :
			TargetHandler->AddOutgoingHealingRestriction(GetOwningBuff(), Restrict);
			break;
		default :
			break;
		}
	}
}

void UDamageRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (RestrictType)
		{
		case EDamageModifierType::IncomingDamage :
			TargetHandler->RemoveIncomingDamageRestriction(GetOwningBuff());
			break;
		case EDamageModifierType::OutgoingDamage :
			TargetHandler->RemoveOutgoingDamageRestriction(GetOwningBuff());
			break;
		case EDamageModifierType::IncomingHealing :
			TargetHandler->RemoveIncomingHealingRestriction(GetOwningBuff());
			break;
		case EDamageModifierType::OutgoingHealing :
			TargetHandler->RemoveOutgoingHealingRestriction(GetOwningBuff());
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