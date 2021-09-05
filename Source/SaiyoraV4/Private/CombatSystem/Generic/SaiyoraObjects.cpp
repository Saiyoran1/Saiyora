// Fill out your copyright notice in the Description page of Project Settings.

#include "SaiyoraObjects.h"
#include "Buff.h"

void UModifiableIntValue::Init(int32 const Base, bool const bModdable, bool const bLowCapped, int32 const Min,
    bool const bHighCapped, int32 const Max)
{
    bModifiable = bModdable;
    bHasMin = bLowCapped;
    Minimum = Min;
    bHasMax = bHighCapped;
    Maximum = FMath::Max(Min, Max);
    BaseValue = Base;
    if (bHasMin)
    {
        BaseValue = FMath::Max(Min, BaseValue);
    }
    if (bHasMax)
    {
        BaseValue = FMath::Min(Max, BaseValue);
    }
    Value = BaseValue;
    BuffStackCallback.BindDynamic(this, &UModifiableIntValue::CheckForModifierSourceStack);
}

void UModifiableIntValue::SetRecalculationFunction(FIntValueRecalculation const& NewCalculation)
{
    if (NewCalculation.IsBound())
    {
        CustomRecalculation.Clear();
        CustomRecalculation = NewCalculation;
        RecalculateValue();
    }
}

void UModifiableIntValue::RecalculateValue()
{
    int32 const Previous = Value;
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

void UModifiableIntValue::SubscribeToValueChanged(FIntValueCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnValueChanged.AddUnique(Callback);
    }
}

void UModifiableIntValue::UnsubscribeFromValueChanged(FIntValueCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnValueChanged.Remove(Callback);
    }
}

int32 UModifiableIntValue::AddModifier(FCombatModifier const& Modifier)
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

void UModifiableIntValue::RemoveModifier(int32 const ModifierID)
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

void UModifiableIntValue::CheckForModifierSourceStack(FBuffApplyEvent const& Event)
{
    if (Event.Result.PreviousStacks != Event.Result.NewStacks && IsValid(Event.Result.AffectedBuff))
    {
        RecalculateValue();
    }
}

void UModifiableFloatValue::Init(float const Base, bool const bModdable, bool const bLowCapped, float const Min,
                                 bool const bHighCapped, float const Max)
{
    bModifiable = bModdable;
    bHasMin = bLowCapped;
    Minimum = Min;
    bHasMax = bHighCapped;
    Maximum = FMath::Max(Min, Max);
    BaseValue = Base;
    if (bHasMin)
    {
        BaseValue = FMath::Max(Min, BaseValue);
    }
    if (bHasMax)
    {
        BaseValue = FMath::Min(Max, BaseValue);
    }
    Value = BaseValue;
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