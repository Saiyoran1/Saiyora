// Fill out your copyright notice in the Description page of Project Settings.
#include "SaiyoraStructs.h"
#include "Buff.h"

int32 FCombatModifier::GlobalID = 0;

FCombatModifier::FCombatModifier()
{
    Source = nullptr;
    ModType = EModifierType::Invalid;
    ModValue = 0.0f;
    bStackable = false;
}

FCombatModifier::FCombatModifier(float const Value, EModifierType const ModType, bool const bStackable, UBuff* Source)
{
    ModValue = Value;
    this->ModType = ModType;
    if (IsValid(Source))
    {
        this->Source = Source;
        this->bStackable = bStackable;
    }
    else
    {
        this->Source = nullptr;
        this->bStackable = false;
    }
}

float FCombatModifier::ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue)
{
    float AddMod = 0.0f;
    float MultMod = 1.0f;
    for (FCombatModifier const& Mod : ModArray)
    {
        bool const bShouldStack = Mod.bStackable && IsValid(Mod.Source);
        switch (Mod.ModType)
        {
        case EModifierType::Invalid :
            break;
        case EModifierType::Additive :
            AddMod += bShouldStack ? Mod.ModValue * Mod.Source->GetCurrentStacks() : Mod.ModValue;
            break;
        case EModifierType::Multiplicative :
            //This means no negative multipliers.
            //If you want a 5% reduction as a modifier, you would use .95. At, for example, 2 stacks, this would become:
            //.95 - 1, which is -.05, then -.05 * 2, which is -.1, then -.1 + 1, which is .9, so a 10% reduction.
            MultMod *= FMath::Max(0.0f, bShouldStack ? Mod.Source->GetCurrentStacks() * (Mod.ModValue - 1.0f) + 1.0f : Mod.ModValue);
            break;
        default :
            break;
        }
    }
    return FMath::Max(0.0f, FMath::Max(0.0f, BaseValue + AddMod) * MultMod);
}

int32 FCombatModifier::ApplyModifiers(TArray<FCombatModifier> const& ModArray, int32 const BaseValue)
{
    float const ValueAsFloat = static_cast<float>(BaseValue);
    return static_cast<int32>(ApplyModifiers(ModArray, ValueAsFloat));
}

int32 FCombatModifier::GetID()
{
    GlobalID++;
    if (GlobalID == -1)
    {
        GlobalID++;
    }
    return GlobalID;
}

FCombatFloatValue::FCombatFloatValue()
{
    bModifiable = false;
    bHasMin = false;
    Minimum = 0.0f;
    bHasMax = false;
    Maximum = 0.0f;
    BaseValue = 0.0f;
    Value = 0.0f;
}

FCombatFloatValue::FCombatFloatValue(float const BaseValue, bool const bModifiable, bool const bHasMin, float const Min, bool const bHasMax,
                                     float const Max)
{
    this->bModifiable = bModifiable;
    this->bHasMin = bHasMin;
    Minimum = Min;
    this->bHasMax = bHasMax;
    Maximum = FMath::Max(Minimum, Max);
    this->BaseValue = BaseValue;
    if (this->bHasMin)
    {
        this->BaseValue = FMath::Max(Minimum, this->BaseValue);
    }
    if (this->bHasMax)
    {
        this->BaseValue = FMath::Min(Maximum, this->BaseValue);
    }
    Value = this->BaseValue;
    FModifierCallback RecalculationCallback;
    RecalculationCallback.BindRaw(this, &FCombatFloatValue::RecalculateValue);
    Modifiers.BindToModsChanged(RecalculationCallback);
}

void FCombatFloatValue::SetRecalculationFunction(FCombatValueRecalculation const& NewCalculation)
{
    if (NewCalculation.IsBound())
    {
        CustomCalculation = NewCalculation;
    }
}

float FCombatFloatValue::ForceRecalculation()
{
    RecalculateValue();
    return Value;
}

FDelegateHandle FCombatFloatValue::BindToValueChanged(FFloatValueCallback const& Callback)
{
    if (Callback.IsBound())
    {
        return OnValueChanged.Add(Callback);
    }
    FDelegateHandle const Fail;
    return Fail;
}

void FCombatFloatValue::UnbindFromValueChanged(FDelegateHandle const& Handle)
{
    if (Handle.IsValid())
    {
        OnValueChanged.Remove(Handle);
    }
}

int32 FCombatFloatValue::AddModifier(FCombatModifier const& Modifier)
{
    return Modifiers.AddModifier(Modifier);
}

void FCombatFloatValue::RemoveModifier(int32 const ModifierID)
{
    Modifiers.RemoveModifier(ModifierID);
}

void FCombatFloatValue::RecalculateValue()
{
    float const OldValue = Value;
    if (bModifiable)
    {
        if (CustomCalculation.IsBound())
        {
            TArray<FCombatModifier> DependencyMods;
            Modifiers.GetSummedModifiers(DependencyMods);
            float CustomValue = CustomCalculation.Execute(DependencyMods, BaseValue);
            if (bHasMin)
            {
                CustomValue = FMath::Max(Minimum, CustomValue);
            }
            if (bHasMax)
            {
                CustomValue = FMath::Min(Maximum, CustomValue);
            }
            Value = CustomValue;
        }
        else
        {
            DefaultRecalculation();
        }
    }
    else
    {
        Value = BaseValue;
    }
    if (Value != OldValue)
    {
        OnValueChanged.Broadcast(OldValue, Value);
    }
}

void FCombatFloatValue::DefaultRecalculation()
{
    TArray<FCombatModifier> DependencyMods;
    Modifiers.GetSummedModifiers(DependencyMods);
    float NewValue = FCombatModifier::ApplyModifiers(DependencyMods, BaseValue);
    if (bHasMin)
    {
        NewValue = FMath::Max(Minimum, NewValue);
    }
    if (bHasMax)
    {
        NewValue = FMath::Min(Maximum, NewValue);
    }
    Value = NewValue;
}

FCombatIntValue::FCombatIntValue()
{
    bModifiable = false;
    bHasMin = false;
    Minimum = 0.0f;
    bHasMax = false;
    Maximum = 0.0f;
    BaseValue = 0.0f;
    Value = 0.0f;
}

FCombatIntValue::FCombatIntValue(int32 const BaseValue, bool const bModifiable, bool const bHasMin, int32 const Min,
    bool const bHasMax, int32 const Max)
{
    this->bModifiable = bModifiable;
    this->bHasMin = bHasMin;
    Minimum = Min;
    this->bHasMax = bHasMax;
    Maximum = FMath::Max(Minimum, Max);
    this->BaseValue = BaseValue;
    if (this->bHasMin)
    {
        this->BaseValue = FMath::Max(Minimum, this->BaseValue);
    }
    if (this->bHasMax)
    {
        this->BaseValue = FMath::Min(Maximum, this->BaseValue);
    }
    Value = this->BaseValue;
    FModifierCallback RecalculationCallback;
    RecalculationCallback.BindRaw(this, &FCombatIntValue::RecalculateValue);
    Modifiers.BindToModsChanged(RecalculationCallback);
}

void FCombatIntValue::SetRecalculationFunction(FCombatIntValueRecalculation const& NewCalculation)
{
    if (NewCalculation.IsBound())
    {
        CustomCalculation = NewCalculation;
    }
}

int32 FCombatIntValue::ForceRecalculation()
{
    RecalculateValue();
    return Value;
}

FDelegateHandle FCombatIntValue::BindToValueChanged(FIntValueCallback const& Callback)
{
    if (Callback.IsBound())
    {
        return OnValueChanged.Add(Callback);
    }
    FDelegateHandle const Fail;
    return Fail;
}

void FCombatIntValue::UnbindFromValueChanged(FDelegateHandle const& Handle)
{
    if (Handle.IsValid())
    {
        OnValueChanged.Remove(Handle);
    }
}

int32 FCombatIntValue::AddModifier(FCombatModifier const& Modifier)
{
    return Modifiers.AddModifier(Modifier);
}

void FCombatIntValue::RemoveModifier(int32 const ModifierID)
{
    Modifiers.RemoveModifier(ModifierID);
}

void FCombatIntValue::RecalculateValue()
{
    int32 const OldValue = Value;
    if (bModifiable)
    {
        if (CustomCalculation.IsBound())
        {
            TArray<FCombatModifier> DependencyMods;
            Modifiers.GetSummedModifiers(DependencyMods);
            int32 CustomValue = CustomCalculation.Execute(DependencyMods, BaseValue);
            if (bHasMin)
            {
                CustomValue = FMath::Max(Minimum, CustomValue);
            }
            if (bHasMax)
            {
                CustomValue = FMath::Min(Maximum, CustomValue);
            }
            Value = CustomValue;
        }
        else
        {
            DefaultRecalculation();
        }
    }
    else
    {
        Value = BaseValue;
    }
    if (Value != OldValue)
    {
        OnValueChanged.Broadcast(OldValue, Value);
    }
}

void FCombatIntValue::DefaultRecalculation()
{
    TArray<FCombatModifier> DependencyMods;
    Modifiers.GetSummedModifiers(DependencyMods);
    int32 NewValue = FCombatModifier::ApplyModifiers(DependencyMods, BaseValue);
    if (bHasMin)
    {
        NewValue = FMath::Max(Minimum, NewValue);
    }
    if (bHasMax)
    {
        NewValue = FMath::Min(Maximum, NewValue);
    }
    Value = NewValue;
}

void UModifiableFloatValue::Init(float const BaseValue, bool const bModifiable, bool const bHasMin, float const Minimum,
    bool const bHasMax, float const Maximum)
{
    this->bModifiable = bModifiable;
    this->bHasMin = bHasMin;
    this->Minimum = Minimum;
    this->bHasMax = bHasMax;
    this->Maximum = FMath::Max(Minimum, Maximum);
    this->BaseValue = BaseValue;
    if (this->bHasMin)
    {
        this->BaseValue = FMath::Max(Minimum, this->BaseValue);
    }
    if (this->bHasMax)
    {
        this->BaseValue = FMath::Min(Maximum, this->BaseValue);
    }
    Value = this->BaseValue;
    BuffStackCallback.BindDynamic(this, &UModifiableFloatValue::CheckForModifierSourceStack);
}

void UModifiableFloatValue::SetRecalculationFunction(FFloatValueRecalculation const& NewCalculation)
{
    if (NewCalculation.IsBound())
    {
        CustomRecalculation.Clear();
        CustomRecalculation = NewCalculation;
        RecalculateValue();
    }
}

void UModifiableFloatValue::RecalculateValue()
{
    float const Previous = Value;
    if (bModifiable)
    {
        TArray<FCombatModifier> Mods;
        Modifiers.GenerateValueArray(Mods);
        if (CustomRecalculation.IsBound())
        {
            Value = CustomRecalculation.Execute(Mods, BaseValue);
        }
        else
        {
            Value = FCombatModifier::ApplyModifiers(Mods, BaseValue);
        }
        if (bHasMin)
        {
            Value = FMath::Max(Minimum, Value);
        }
        if (bHasMax)
        {
            Value = FMath::Min(Maximum, Value);
        }
    }
    else
    {
        Value = BaseValue;   
    }
    if (Value != Previous)
    {
        OnValueChanged.Broadcast(Previous, Value);
    }
}

void UModifiableFloatValue::SubscribeToValueChanged(FFloatValueCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnValueChanged.AddUnique(Callback);
    }
}

void UModifiableFloatValue::UnsubscribeFromValueChanged(FFloatValueCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnValueChanged.Remove(Callback);
    }
}

int32 UModifiableFloatValue::AddModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid)
    {
        return -1;
    }
    int32 const ModID = FCombatModifier::GetID();
    Modifiers.Add(ModID, Modifier);
    if (Modifier.GetStackable() && IsValid(Modifier.GetSource()))
    {
        Modifier.GetSource()->SubscribeToBuffUpdated(BuffStackCallback);
    }
    RecalculateValue();
    return ModID;
}

void UModifiableFloatValue::RemoveModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    FCombatModifier* Mod = Modifiers.Find(ModifierID);
    if (Mod && Mod->GetStackable() && IsValid(Mod->GetSource()))
    {
        Mod->GetSource()->UnsubscribeFromBuffUpdated(BuffStackCallback);
    }
    if (Modifiers.Remove(ModifierID) > 0)
    {
        RecalculateValue();
    }
}

void UModifiableFloatValue::CheckForModifierSourceStack(FBuffApplyEvent const& Event)
{
    if (Event.Result.PreviousStacks != Event.Result.NewStacks && IsValid(Event.Result.AffectedBuff))
    {
        RecalculateValue();
    }
}

