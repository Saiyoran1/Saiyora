// Fill out your copyright notice in the Description page of Project Settings.
#include "BuffFunction.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraDamageFunctions.h"
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
    float const InitDamage = bUsesSeparateInitialDamage ? InitialDamageAmount : BaseDamage;
    EDamageSchool const InitSchool = bUsesSeparateInitialDamage ? InitialDamageSchool : DamageSchool;
        
    USaiyoraDamageFunctions::ApplyDamage(InitDamage, GetOwningBuff()->GetAppliedBy(),
        GetOwningBuff()->GetAppliedTo(), GetOwningBuff(), EDamageHitStyle::Chronic, InitSchool, bIgnoresRestrictions,
        bIgnoresModifiers, false);
}

void UDamageOverTimeFunction::TickDamage()
{
    float const TickDamage = bScalesWithStacks ? BaseDamage * GetOwningBuff()->GetCurrentStacks() : BaseDamage;

    USaiyoraDamageFunctions::ApplyDamage(TickDamage, GetOwningBuff()->GetAppliedBy(),
        GetOwningBuff()->GetAppliedTo(), GetOwningBuff(), EDamageHitStyle::Chronic, DamageSchool, bIgnoresRestrictions,
        bIgnoresModifiers, bSnapshots);
}

void UDamageOverTimeFunction::SetDamageVars(float const Damage, EDamageSchool const School, float const Interval,
    bool const bIgnoreRestrictions, bool const bIgnoreModifiers, bool const bSnapshot, bool const bScaleWithStacks,
    bool const bPartialTickOnExpire, bool const bInitialTick, bool const bUseSeparateInitialDamage,
    float const InitialDamage, EDamageSchool const InitialSchool)
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
}

void UDamageOverTimeFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
    if (bHasInitialTick)
    {
        InitialTick();
    }
    if (bSnapshots)
    {
        BaseDamage = USaiyoraDamageFunctions::GetSnapshotDamage(BaseDamage, GetOwningBuff()->GetAppliedBy(),
            GetOwningBuff()->GetAppliedTo(), GetOwningBuff(), EDamageHitStyle::Chronic, DamageSchool, bIgnoresModifiers);
    }
    if (DamageInterval > 0.0f)
    {
        FTimerDelegate TickDelegate;
        TickDelegate.BindUFunction(this, FName(TEXT("TickDamage")));
        GetWorld()->GetTimerManager().SetTimer(TickHandle, TickDelegate, DamageInterval, true);
    }
}

void UDamageOverTimeFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
    if (bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && DamageInterval > 0.0f)
    {
        float const TickFraction = GetWorld()->GetTimerManager().GetTimerRemaining(TickHandle) / DamageInterval;
        if (TickFraction > 0.0f)
        {
            float const ExpireTickDamage = (bScalesWithStacks ? BaseDamage * GetOwningBuff()->GetCurrentStacks() : BaseDamage) * TickFraction;
            USaiyoraDamageFunctions::ApplyDamage(ExpireTickDamage, GetOwningBuff()->GetAppliedBy(),
                GetOwningBuff()->GetAppliedTo(), GetOwningBuff(), EDamageHitStyle::Chronic, DamageSchool, bIgnoresRestrictions,
                bIgnoresModifiers, bSnapshots);
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
    float const InitialDamage, EDamageSchool const InitialSchool)
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
        bScalesWithStacks, bPartialTickOnExpire, bHasInitialTick, bUseSeparateInitialDamage, InitialDamage, InitialSchool);
}

//Healing Over Time

void UHealingOverTimeFunction::InitialTick()
{
    float const InitHealing = bUsesSeparateInitialHealing ? InitialHealingAmount : BaseHealing;
    EDamageSchool const InitSchool = bUsesSeparateInitialHealing ? InitialHealingSchool : HealingSchool;
        
    USaiyoraDamageFunctions::ApplyHealing(InitHealing, GetOwningBuff()->GetAppliedBy(),
        GetOwningBuff()->GetAppliedTo(), GetOwningBuff(), EDamageHitStyle::Chronic, InitSchool, bIgnoresRestrictions,
        bIgnoresModifiers, false);
}

void UHealingOverTimeFunction::TickHealing()
{
    float const TickHealing = bScalesWithStacks ? BaseHealing * GetOwningBuff()->GetCurrentStacks() : BaseHealing;

    USaiyoraDamageFunctions::ApplyHealing(TickHealing, GetOwningBuff()->GetAppliedBy(),
        GetOwningBuff()->GetAppliedTo(), GetOwningBuff(), EDamageHitStyle::Chronic, HealingSchool, bIgnoresRestrictions,
        bIgnoresModifiers, bSnapshots);
}

void UHealingOverTimeFunction::SetHealingVars(float const Healing, EDamageSchool const School, float const Interval,
    bool const bIgnoreRestrictions, bool const bIgnoreModifiers, bool const bSnapshot, bool const bScaleWithStacks,
    bool const bPartialTickOnExpire, bool const bInitialTick, bool const bUseSeparateInitialHealing,
    float const InitialHealing, EDamageSchool const InitialSchool)
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
}

void UHealingOverTimeFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
    if (bHasInitialTick)
    {
        InitialTick();
    }
    if (bSnapshots)
    {
        BaseHealing = USaiyoraDamageFunctions::GetSnapshotHealing(BaseHealing, GetOwningBuff()->GetAppliedBy(),
            GetOwningBuff()->GetAppliedTo(), GetOwningBuff(), EDamageHitStyle::Chronic, HealingSchool, bIgnoresModifiers);
    }
    if (HealingInterval > 0.0f)
    {
        FTimerDelegate TickDelegate;
        TickDelegate.BindUFunction(this, FName(TEXT("TickHealing")));
        GetWorld()->GetTimerManager().SetTimer(TickHandle, TickDelegate, HealingInterval, true);
    }
}

void UHealingOverTimeFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
    if (bTicksOnExpire && RemoveEvent.ExpireReason == EBuffExpireReason::TimedOut && HealingInterval > 0.0f)
    {
        float const TickFraction = GetWorld()->GetTimerManager().GetTimerRemaining(TickHandle) / HealingInterval;
        if (TickFraction > 0.0f)
        {
            float const ExpireTickHealing = (bScalesWithStacks ? BaseHealing * GetOwningBuff()->GetCurrentStacks() : BaseHealing) * TickFraction;
            USaiyoraDamageFunctions::ApplyHealing(ExpireTickHealing, GetOwningBuff()->GetAppliedBy(),
                GetOwningBuff()->GetAppliedTo(), GetOwningBuff(), EDamageHitStyle::Chronic, HealingSchool, bIgnoresRestrictions,
                bIgnoresModifiers, bSnapshots);
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
    float const InitialHealing, EDamageSchool const InitialSchool)
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
        bScalesWithStacks, bPartialTickOnExpire, bHasInitialTick, bUseSeparateInitialHealing, InitialHealing, InitialSchool);
}

//Stat Modifiers

void UStatModifierFunction::SetModifierVars(TArray<FStatModifier> const& Modifiers)
{
    StatMods = Modifiers;
    for (FStatModifier& StatMod : StatMods)
    {
        StatMod.Source = GetOwningBuff();
    }
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
    TargetHandler->AddStatModifiers(StatMods);
}

void UStatModifierFunction::OnStack(FBuffApplyEvent const& ApplyEvent)
{
    if (!IsValid(TargetHandler))
    {
        return;
    }
    TSet<FGameplayTag> StatTags;
    for (FStatModifier const& StatMod : StatMods)
    {
        if (StatMod.bStackable && StatMod.StatTag.MatchesTag(UStatHandler::GenericStatTag) && StatMod.ModType != EModifierType::Invalid)
        {
            StatTags.Add(StatMod.StatTag);
        }
    }
    TargetHandler->UpdateStackingModifiers(GetOwningBuff(), StatTags);
}

void UStatModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
    if (IsValid(TargetHandler))
    {
        TSet<FGameplayTag> StatTags;
        for (FStatModifier const& StatMod : StatMods)
        {
            if (StatMod.StatTag.MatchesTag(UStatHandler::GenericStatTag) && StatMod.ModType != EModifierType::Invalid)
            {
                StatTags.Add(StatMod.StatTag);
            }
        }
        TargetHandler->RemoveStatModifiers(GetOwningBuff(), StatTags);
    }
}

void UStatModifierFunction::StatModifiers(UBuff* Buff, TArray<FStatModifier> const& Modifiers)
{
    if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
    {
        return;
    }
    UStatModifierFunction* NewStatModFunction = Cast<UStatModifierFunction>(InstantiateBuffFunction(Buff, UStatModifierFunction::StaticClass()));
    if (!IsValid(NewStatModFunction))
    {
        return;
    }
    NewStatModFunction->SetModifierVars(Modifiers);
}
