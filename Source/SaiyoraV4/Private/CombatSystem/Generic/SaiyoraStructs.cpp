// Fill out your copyright notice in the Description page of Project Settings.
#include "SaiyoraStructs.h"
#include "Buff.h"

int32 FModifierCollection::GlobalID = 0;

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

void FCombatModifier::Reset()
{
    if (IsValid(Source))
    {
        Source->ModifierUnsubFromRemove(RemoveHandle);
        if (bStackable)
        {
            Source->ModifierUnsubFromStack(StackHandle);
        }
    }
    Source = nullptr;
    ModType = EModifierType::Invalid;
    ModValue = 0.0f;
    bStackable = false;
    OnModifierChanged.Clear();
}

void FCombatModifier::Activate(FModifierCallback const& OnChangedCallback)
{
    if (IsValid(Source))
    {
        FModifierCallback RemoveCallback;
        RemoveCallback.BindRaw(this, &FCombatModifier::OnBuffRemoved);
        Source->ModifierSubToRemove(RemoveCallback, RemoveHandle);
        if (bStackable)
        {
            FModifierCallback StackCallback;
            StackCallback.BindRaw(this, &FCombatModifier::OnBuffStacked);
            Source->ModifierSubToStack(StackCallback, StackHandle);
        }        
        OnModifierChanged.Add(OnChangedCallback);
    }
}

void FCombatModifier::SetValue(float const NewValue)
{
    if (NewValue != ModValue)
    {
        ModValue = NewValue;
        OnModifierChanged.Broadcast();
    }
}

void FCombatModifier::SetModType(EModifierType const NewModType)
{
    if (NewModType != ModType)
    {
        ModType = NewModType;
        OnModifierChanged.Broadcast();
    }
}

void FCombatModifier::SetStackable(bool const NewStackable)
{
    if (NewStackable != bStackable)
    {
        bStackable = NewStackable;
        if (IsValid(Source))
        {
            FModifierCallback StackCallback;
            StackCallback.BindRaw(this, &FCombatModifier::OnBuffStacked);
            Source->ModifierSubToStack(StackCallback, StackHandle);
        }
    }
}

void FCombatModifier::SetSource(UBuff* NewSource)
{
    int32 PreviousSourceStacks = 1;
    if (IsValid(Source))
    {
        if (IsValid(NewSource) && NewSource == Source)
        {
            return;
        }
        Source->ModifierUnsubFromRemove(RemoveHandle);
        if (bStackable)
        {
            PreviousSourceStacks = Source->GetCurrentStacks();
            Source->ModifierUnsubFromStack(StackHandle);
        }
        Source = nullptr;
    }
    if (IsValid(NewSource))
    {
        Source = NewSource;
        FModifierCallback RemoveCallback;
        RemoveCallback.BindRaw(this, &FCombatModifier::OnBuffRemoved);
        Source->ModifierSubToRemove(RemoveCallback, RemoveHandle);
        if (bStackable)
        {
            FModifierCallback StackCallback;
            StackCallback.BindRaw(this, &FCombatModifier::OnBuffStacked);
            Source->ModifierSubToStack(StackCallback, StackHandle);
        }
    }
    if (bStackable && PreviousSourceStacks != (IsValid(Source) ? Source->GetCurrentStacks() : 1))
    {
        OnModifierChanged.Broadcast();
    }
}

void FCombatModifier::OnBuffStacked()
{
    if (bStackable)
    {
        OnModifierChanged.Broadcast();
    }
}

void FCombatModifier::OnBuffRemoved()
{
    ModType = EModifierType::Invalid;
    OnModifierChanged.Broadcast();
}

float FCombatModifier::ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue)
{
    FCombatModifier AddMod;
    FCombatModifier MultMod;
    CombineModifiers(ModArray, AddMod, MultMod);
    return FMath::Max(0.0f, FMath::Max(0.0f, BaseValue + AddMod.ModValue) * MultMod.ModValue);
}

int32 FCombatModifier::ApplyModifiers(TArray<FCombatModifier> const& ModArray, int32 const BaseValue)
{
    float const ValueAsFloat = static_cast<float>(BaseValue);
    return static_cast<int32>(ApplyModifiers(ModArray, ValueAsFloat));
}

void FCombatModifier::CombineModifiers(TArray<FCombatModifier> const& ModArray, FCombatModifier& OutAddMod,
                                       FCombatModifier& OutMultMod)
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
    OutAddMod.Reset();
    OutAddMod.ModType = EModifierType::Additive;
    OutAddMod.ModValue = AddMod;
    OutMultMod.Reset();
    OutMultMod.ModType = EModifierType::Multiplicative;
    OutMultMod.ModValue = MultMod;
}

int32 FModifierCollection::GetID()
{
    GlobalID++;
    if (GlobalID == -1)
    {
        GlobalID++;
    }
    return GlobalID;
}

int32 FModifierCollection::AddModifier(FCombatModifier const& Modifier)
{
    if (Modifier.GetModType() == EModifierType::Invalid)
    {
        return -1;
    }
    int32 const ModID = GetID();
    FCombatModifier& NewMod = IndividualModifiers.Add(ModID, Modifier);
    FModifierCallback OnChangedCallback;
    OnChangedCallback.BindRaw(this, &FModifierCollection::RecalculateMods);
    NewMod.Activate(OnChangedCallback);
    RecalculateMods();
    return ModID;
}

void FModifierCollection::RemoveModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }    
    if (IndividualModifiers.Remove(ModifierID) > 0)
    {
        RecalculateMods();
    }
}

FDelegateHandle FModifierCollection::BindToModsChanged(FModifierCallback const& Callback)
{
    if (Callback.IsBound())
    {
        return OnModifiersChanged.Add(Callback); 
    }
    FDelegateHandle const Fail;
    return Fail;
}

void FModifierCollection::UnbindFromModsChanged(FDelegateHandle const& Handle)
{
    if (Handle.IsValid())
    {
        OnModifiersChanged.Remove(Handle);
    }
}

void FModifierCollection::RecalculateMods()
{
    float const PreviousAddMod = SummedAddMod.GetValue();
    float const PreviousMultMod = SummedMultMod.GetValue();
    PurgeInvalidMods();
    TArray<FCombatModifier> Mods;
    IndividualModifiers.GenerateValueArray(Mods);
    FCombatModifier::CombineModifiers(Mods, SummedAddMod, SummedMultMod);
    if (PreviousAddMod != SummedAddMod.GetValue() || PreviousMultMod != SummedMultMod.GetValue())
    {
        OnModifiersChanged.Broadcast();
    }
}

void FModifierCollection::PurgeInvalidMods()
{
    TArray<int32> InvalidIDs;
    for (TTuple<int32, FCombatModifier> const& Mod : IndividualModifiers)
    {
        if (Mod.Value.GetModType() == EModifierType::Invalid)
        {
            InvalidIDs.Add(Mod.Key);
        }
    }
    for (int32 const ID : InvalidIDs)
    {
        IndividualModifiers.Remove(ID);
    }
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
