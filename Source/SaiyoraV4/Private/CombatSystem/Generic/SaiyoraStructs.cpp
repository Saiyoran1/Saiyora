// Fill out your copyright notice in the Description page of Project Settings.
#include "SaiyoraStructs.h"

#include "Buff.h"

int32 FCombatModifier::GlobalID = 0;

float FCombatModifier::CombineModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue)
{
    float AddMod = 0.0f;
    float MultMod = 1.0f;
    for (FCombatModifier const& Mod : ModArray)
    {
        if (Mod.bFromBuff && !IsValid(Mod.Source))
        {
            continue;
        }
        bool const bShouldStack = Mod.bFromBuff && Mod.bStackable && IsValid(Mod.Source);
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
    //Again, no negative values.
    return FMath::Max(0.0f, FMath::Max(0.0f, BaseValue + AddMod) * MultMod);
}

FCombatModifier FCombatModifier::CombineAdditiveMods(TArray<FCombatModifier> const& Mods)
{
    FCombatModifier OutMod;
    OutMod.ModType = EModifierType::Additive;
    for (FCombatModifier const& Mod : Mods)
    {
        if (Mod.ModType == EModifierType::Additive)
        {
            bool const bShouldStack = Mod.bFromBuff && Mod.bStackable && IsValid(Mod.Source);
            OutMod.ModValue += bShouldStack ? Mod.ModValue * Mod.Source->GetCurrentStacks() : Mod.ModValue;
        }
    }
    return OutMod;
}

FCombatModifier FCombatModifier::CombineMultiplicativeMods(TArray<FCombatModifier> const& Mods)
{
    FCombatModifier OutMod;
    OutMod.ModType = EModifierType::Multiplicative;
    OutMod.ModValue = 1.0f;
    for (FCombatModifier const& Mod : Mods)
    {
        if (Mod.ModType == EModifierType::Multiplicative)
        {
            bool const bShouldStack = Mod.bFromBuff && Mod.bStackable && IsValid(Mod.Source);
            OutMod.ModValue *= FMath::Max(0.0f, bShouldStack ? Mod.Source->GetCurrentStacks() * (Mod.ModValue - 1.0f) + 1.0f : Mod.ModValue);
        }
    }
    OutMod.ModValue = FMath::Max(0.0f, OutMod.ModValue);
    return OutMod;
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
