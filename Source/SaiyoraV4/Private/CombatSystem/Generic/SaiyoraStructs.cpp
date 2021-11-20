// Fill out your copyright notice in the Description page of Project Settings.
#include "SaiyoraStructs.h"
#include "Buff.h"

int32 FCombatModifier::GlobalID = 0;
const FCombatModifier FCombatModifier::InvalidMod(0.0f, EModifierType::Invalid);

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