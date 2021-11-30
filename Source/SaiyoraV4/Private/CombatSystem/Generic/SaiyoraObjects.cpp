#include "SaiyoraObjects.h"

/*void UModifiableIntValue::Init(int32 const Base, bool const bModdable, bool const bLowCapped, int32 const Min,
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

void UModifiableIntValue::RecalculateValue(bool const bChangedPredictionID)
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
    if (Previous != Value || bChangedPredictionID)
    {
        OnUpdated(Previous, LastPredictionID);
    }
}

void UModifiableIntValue::AddModifier(FCombatModifier const& Modifier, UBuff* Source)
{
    if (Modifier.Type == EModifierType::Invalid || (Modifier.bStackable && Modifier.Stacks < 1) || !IsValid(Source))
    {
        return;
    }
    bool bShouldRecalculate = false;
    FCombatModifier* Found = Modifiers.Find(Source);
    if (Found)
    {
        if (Found->bStackable && Found->Stacks != Modifier.Stacks)
        {
            Found->Stacks = Modifier.Stacks;
            bShouldRecalculate = true;
        }
    }
    else
    {
        Modifiers.Add(Source, Modifier);
        bShouldRecalculate = true;
    }
    bool bChangedPredictionID = false;
    if (Source->GetLastPredictionID() > LastPredictionID)
    {
        LastPredictionID = Source->GetLastPredictionID();
        bChangedPredictionID = true;
    }
    if (bShouldRecalculate || bChangedPredictionID)
    {
        RecalculateValue(bChangedPredictionID);
    }
}

void UModifiableIntValue::RemoveModifier(UBuff* Source)
{
    if (!IsValid(Source) || Modifiers.Remove(Source) < 1)
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

void UModifiableFloatValue::RecalculateValue(bool const bChangedPredictionID)
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
    if (Value != Previous || bChangedPredictionID)
    {
        OnValueChanged(Previous, LastPredictionID);
    }
}

void UModifiableFloatValue::AddModifier(FCombatModifier const& Modifier, UBuff* Source)
{
    if (Modifier.Type == EModifierType::Invalid || (Modifier.bStackable && Modifier.Stacks < 1) || !IsValid(Source))
    {
        return;
    }
    bool bShouldRecalculate = false;
    FCombatModifier* Found = Modifiers.Find(Source);
    if (Found)
    {
        if (Found->bStackable && Found->Stacks != Modifier.Stacks)
        {
            Found->Stacks = Modifier.Stacks;
            bShouldRecalculate = true;
        }
    }
    else
    {
        Modifiers.Add(Source, Modifier);
        bShouldRecalculate = true;
    }
    bool bChangedPrediction = false;
    if (Source->GetLastPredictionID() > LastPredictionID)
    {
        LastPredictionID = Source->GetLastPredictionID();
        bChangedPrediction = true;
    }
    if (bShouldRecalculate || bChangedPrediction)
    {
        RecalculateValue(bChangedPrediction);
    }
}

void UModifiableFloatValue::RemoveModifier(UBuff* Source)
{
    if (!IsValid(Source) || Modifiers.Remove(Source) < 1)
    {
        RecalculateValue();
    }
}*/