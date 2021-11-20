// Fill out your copyright notice in the Description page of Project Settings.
#include "BuffFunction.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatComponent.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"

UBuffFunction* UBuffFunction::InstantiateBuffFunction(UBuff* Buff, TSubclassOf<UBuffFunction> const FunctionClass)
{
    if (!IsValid(Buff) || !IsValid(FunctionClass))
    {
        return nullptr;
    }
    UBuffFunction* NewFunction = NewObject<UBuffFunction>(Buff, FunctionClass);
    NewFunction->InitializeBuffFunction(Buff);
    return NewFunction;
}

UWorld* UBuffFunction::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UBuffFunction::InitializeBuffFunction(UBuff* BuffRef)
{
    if (!bBuffFunctionInitialized)
    {
        if (!IsValid(BuffRef))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid owning buff provided to buff function."));
            return;
        }
        OwningBuff = BuffRef;
        OwningBuff->RegisterBuffFunction(this);
        bBuffFunctionInitialized = true;
        SetupBuffFunction();
    }
}

void UCustomBuffFunction::SetupCustomBuffFunction(UBuff* Buff, TSubclassOf<UCustomBuffFunction> const FunctionClass)
{
    InstantiateBuffFunction(Buff, FunctionClass);
}

void UCustomBuffFunction::SetupCustomBuffFunctions(UBuff* Buff,
    TArray<TSubclassOf<UCustomBuffFunction>> const& FunctionClasses)
{
    for (TSubclassOf<UCustomBuffFunction> const& FunctionClass : FunctionClasses)
    {
        InstantiateBuffFunction(Buff, FunctionClass);
    }
}

//Damage Over Time

void UDamageOverTimeFunction::InitialTick()
{
    if (IsValid(TargetComponent))
    {
        float const InitDamage = bUsesSeparateInitialDamage ? InitialDamageAmount : BaseDamage;
        EDamageSchool const InitSchool = bUsesSeparateInitialDamage ? InitialDamageSchool : DamageSchool;
        bool const FromSnapshot = bSnapshots && !bUsesSeparateInitialDamage;
        TargetComponent->ApplyDamage(InitDamage, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
            EDamageHitStyle::Chronic, InitSchool, bIgnoresRestrictions, bIgnoresModifiers,
            FromSnapshot, FDamageModCondition(), ThreatInfo);
    }
}

void UDamageOverTimeFunction::TickDamage()
{
    if (IsValid(TargetComponent))
    {
        float const TickDamage = bScalesWithStacks ? BaseDamage * GetOwningBuff()->GetCurrentStacks() : BaseDamage;
        TargetComponent->ApplyDamage(TickDamage, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(), EDamageHitStyle::Chronic,
            DamageSchool, bIgnoresRestrictions, bIgnoresModifiers, bSnapshots, FDamageModCondition(), ThreatInfo);
    }
}

void UDamageOverTimeFunction::SetDamageVars(float const Damage, EDamageSchool const School, float const Interval,
    bool const bIgnoreRestrictions, bool const bIgnoreModifiers, bool const bSnapshot, bool const bScaleWithStacks,
    bool const bPartialTickOnExpire, bool const bInitialTick, bool const bUseSeparateInitialDamage,
    float const InitialDamage, EDamageSchool const InitialSchool, FThreatFromDamage const& ThreatParams)
{
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
    if (IsValid(GetOwningBuff()->GetAppliedBy()) && GetOwningBuff()->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy());
    }
    if (IsValid(GetOwningBuff()->GetAppliedTo()) && GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
    }
}

void UDamageOverTimeFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
    if (bSnapshots && !bIgnoresModifiers && IsValid(GeneratorComponent))
    {
        FDamageInfo SnapshotInfo;
        SnapshotInfo.Value = BaseDamage;
        SnapshotInfo.Source = GetOwningBuff();
        SnapshotInfo.AppliedBy = GetOwningBuff()->GetAppliedBy();
        SnapshotInfo.AppliedTo = GetOwningBuff()->GetAppliedTo();
        SnapshotInfo.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(SnapshotInfo.AppliedBy);
        SnapshotInfo.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(SnapshotInfo.AppliedTo);
        SnapshotInfo.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(SnapshotInfo.AppliedByPlane, SnapshotInfo.AppliedToPlane);
        SnapshotInfo.HitStyle = EDamageHitStyle::Chronic;
        SnapshotInfo.School = DamageSchool;
        GeneratorComponent->ModifyOutgoingDamage(SnapshotInfo, FDamageModCondition());
        BaseDamage = SnapshotInfo.Value;
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

void UDamageOverTimeFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
    if (IsValid(TargetComponent) && bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && DamageInterval > 0.0f)
    {
        float const TickFraction = GetWorld()->GetTimerManager().GetTimerRemaining(TickHandle) / DamageInterval;
        if (TickFraction > 0.0f)
        {
            float const ExpireTickDamage = (bScalesWithStacks ? BaseDamage * GetOwningBuff()->GetCurrentStacks() : BaseDamage) * TickFraction;
            TargetComponent->ApplyDamage(ExpireTickDamage, GetOwningBuff()->GetAppliedBy(),GetOwningBuff(), EDamageHitStyle::Chronic,
                DamageSchool, bIgnoresRestrictions, bIgnoresModifiers, bSnapshots, FDamageModCondition(), ThreatInfo);
        }
    }
}

void UDamageOverTimeFunction::CleanupBuffFunction()
{
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
}

void UDamageOverTimeFunction::DamageOverTime(UBuff* Buff, float const Damage, EDamageSchool const School, float const Interval,
    bool const bIgnoreRestrictions, bool const bIgnoreModifiers, bool const bSnapshots, bool const bScalesWithStacks,
    bool const bPartialTickOnExpire, bool const bHasInitialTick, bool const bUseSeparateInitialDamage,
    float const InitialDamage, EDamageSchool const InitialSchool, FThreatFromDamage const& ThreatParams)
{
    if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
    {
        return;
    }
    UDamageOverTimeFunction* NewDotFunction = Cast<UDamageOverTimeFunction>(InstantiateBuffFunction(Buff, UDamageOverTimeFunction::StaticClass()));
    if (!IsValid(NewDotFunction))
    {
        return;
    }
    NewDotFunction->SetDamageVars(Damage, School, Interval, bIgnoreRestrictions, bIgnoreModifiers, bSnapshots,
        bScalesWithStacks, bPartialTickOnExpire, bHasInitialTick, bUseSeparateInitialDamage, InitialDamage, InitialSchool, ThreatParams);
}

//Healing Over Time

void UHealingOverTimeFunction::InitialTick()
{
    if (IsValid(TargetComponent))
    {
        float const InitHealing = bUsesSeparateInitialHealing ? InitialHealingAmount : BaseHealing;
        EDamageSchool const InitSchool = bUsesSeparateInitialHealing ? InitialHealingSchool : HealingSchool;
        bool const FromSnapshot = bSnapshots && !bUsesSeparateInitialHealing;
        TargetComponent->ApplyHealing(InitHealing, GetOwningBuff()->GetAppliedBy(), GetOwningBuff(),
            EDamageHitStyle::Chronic, InitSchool, bIgnoresRestrictions, bIgnoresModifiers, FromSnapshot, FDamageModCondition(), ThreatInfo);
    }
}

void UHealingOverTimeFunction::TickHealing()
{
    if (IsValid(TargetComponent))
    {
        float const TickHealing = bScalesWithStacks ? BaseHealing * GetOwningBuff()->GetCurrentStacks() : BaseHealing;
        TargetComponent->ApplyHealing(TickHealing, GetOwningBuff()->GetAppliedBy(),GetOwningBuff(), EDamageHitStyle::Chronic,
            HealingSchool, bIgnoresRestrictions,bIgnoresModifiers, bSnapshots, FDamageModCondition(), ThreatInfo);
    }
}

void UHealingOverTimeFunction::SetHealingVars(float const Healing, EDamageSchool const School, float const Interval,
    bool const bIgnoreRestrictions, bool const bIgnoreModifiers, bool const bSnapshot, bool const bScaleWithStacks,
    bool const bPartialTickOnExpire, bool const bInitialTick, bool const bUseSeparateInitialHealing,
    float const InitialHealing, EDamageSchool const InitialSchool, FThreatFromDamage const& ThreatParams)
{
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
    if (IsValid(GetOwningBuff()->GetAppliedBy()) && GetOwningBuff()->GetAppliedBy()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedBy());
    }
    if (IsValid(GetOwningBuff()->GetAppliedTo()) && GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwningBuff()->GetAppliedTo());
    }
}

void UHealingOverTimeFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
    if (bSnapshots && !bIgnoresModifiers && IsValid(GeneratorComponent))
    {
        FDamageInfo SnapshotInfo;
        SnapshotInfo.Value = BaseHealing;
        SnapshotInfo.Source = GetOwningBuff();
        SnapshotInfo.AppliedBy = GetOwningBuff()->GetAppliedBy();
        SnapshotInfo.AppliedTo = GetOwningBuff()->GetAppliedTo();
        SnapshotInfo.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(SnapshotInfo.AppliedBy);
        SnapshotInfo.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(SnapshotInfo.AppliedTo);
        SnapshotInfo.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(SnapshotInfo.AppliedByPlane, SnapshotInfo.AppliedToPlane);
        SnapshotInfo.HitStyle = EDamageHitStyle::Chronic;
        SnapshotInfo.School = HealingSchool;
        GeneratorComponent->ModifyOutgoingHealing(SnapshotInfo, FDamageModCondition());
        BaseHealing = SnapshotInfo.Value;
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

void UHealingOverTimeFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
    if (IsValid(TargetComponent) && bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && HealingInterval > 0.0f)
    {
        float const TickFraction = GetWorld()->GetTimerManager().GetTimerRemaining(TickHandle) / HealingInterval;
        if (TickFraction > 0.0f)
        {
            float const ExpireTickHealing = (bScalesWithStacks ? BaseHealing * GetOwningBuff()->GetCurrentStacks() : BaseHealing) * TickFraction;
            TargetComponent->ApplyHealing(ExpireTickHealing, GetOwningBuff()->GetAppliedBy(),GetOwningBuff(), EDamageHitStyle::Chronic,
                HealingSchool, bIgnoresRestrictions,bIgnoresModifiers, bSnapshots, FDamageModCondition(), ThreatInfo);
        }
    }
}

void UHealingOverTimeFunction::CleanupBuffFunction()
{
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
}

void UHealingOverTimeFunction::HealingOverTime(UBuff* Buff, float const Healing, EDamageSchool const School, float const Interval,
    bool const bIgnoreRestrictions, bool const bIgnoreModifiers, bool const bSnapshots, bool const bScalesWithStacks,
    bool const bPartialTickOnExpire, bool const bHasInitialTick, bool const bUseSeparateInitialHealing,
    float const InitialHealing, EDamageSchool const InitialSchool, FThreatFromDamage const& ThreatParams)
{
    if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
    {
        return;
    }
    UHealingOverTimeFunction* NewHotFunction = Cast<UHealingOverTimeFunction>(InstantiateBuffFunction(Buff, UHealingOverTimeFunction::StaticClass()));
    if (!IsValid(NewHotFunction))
    {
        return;
    }
    NewHotFunction->SetHealingVars(Healing, School, Interval, bIgnoreRestrictions, bIgnoreModifiers, bSnapshots,
        bScalesWithStacks, bPartialTickOnExpire, bHasInitialTick, bUseSeparateInitialHealing, InitialHealing, InitialSchool, ThreatParams);
}

//Stat Modifiers

void UStatModifierFunction::SetModifierVars(TMap<FGameplayTag, FCombatModifier> const& Modifiers)
{
    StatMods = Modifiers;
}

void UStatModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
    if (!GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return;
    }
    TargetHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwningBuff()->GetAppliedTo());
    if (!IsValid(TargetHandler))
    {
        return;
    }
    ModIDs.Empty();
    for (TTuple<FGameplayTag, FCombatModifier> const& Mod : StatMods)
    {
        int32 const NewID = TargetHandler->AddStatModifier(Mod.Key, Mod.Value);
        if (NewID != -1)
        {
            ModIDs.Add(Mod.Key, TargetHandler->AddStatModifier(Mod.Key, Mod.Value));
        }
    }
}
void UStatModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
    if (IsValid(TargetHandler))
    {
        for (TTuple<FGameplayTag, int32> const& ModID : ModIDs)
        {
            TargetHandler->RemoveStatModifier(ModID.Key, ModID.Value);
        }
    }
}

void UStatModifierFunction::StatModifiers(UBuff* Buff, TMap<FGameplayTag, FCombatModifier> const& Modifiers)
{
    if (!IsValid(Buff))
    {
        return;
    }
    UStatModifierFunction* NewStatModFunction = Cast<UStatModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
    if (!IsValid(NewStatModFunction))
    {
        return;
    }
    NewStatModFunction->SetModifierVars(Modifiers);
}
