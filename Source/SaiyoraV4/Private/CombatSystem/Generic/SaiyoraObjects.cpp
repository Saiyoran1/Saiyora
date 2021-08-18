// Fill out your copyright notice in the Description page of Project Settings.

#include "SaiyoraObjects.h"
#include "Buff.h"

void UModifiableIntValue::Init(int32 const BaseValue, bool const bModifiable, bool const bHasMin, int32 const Minimum,
    bool const bHasMax, int32 const Maximum)
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