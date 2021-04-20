// Fill out your copyright notice in the Description page of Project Settings.
#include "SaiyoraStructs.h"

float FCombatModifier::CombineModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue)
{
    float AddMod = 0.0f;
    float MultMod = 1.0f;
    for (FCombatModifier const& Mod : ModArray)
    {
        switch (Mod.ModifierType)
        {
            case EModifierType::Invalid :
                break;
            case EModifierType::Additive :
                AddMod += Mod.ModifierValue * Mod.Stacks;
                break;
            case EModifierType::Multiplicative :
                //This means no negative multipliers.
                //If you want a 5% reduction as a modifier, you would use .95. At, for example, 2 stacks, this would become:
                //.95 - 1, which is -.05, then -.05 * 2, which is -.1, then -.1 + 1, which is .9, so a 10% reduction.
                MultMod *= FMath::Max(0.0f, Mod.Stacks * (Mod.ModifierValue - 1.0f) + 1.0f);
                break;
            default :
                break;
        }
    }
    //Again, no negative values.
    return FMath::Max(0.0f, FMath::Max(0.0f, BaseValue + AddMod) * MultMod);
}