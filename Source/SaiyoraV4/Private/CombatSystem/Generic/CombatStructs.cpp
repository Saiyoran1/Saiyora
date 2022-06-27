#include "CombatStructs.h"
#include "Buff.h"

FSaiyoraCombatTags FSaiyoraCombatTags::SaiyoraCombatTags;

FCombatModifier::FCombatModifier(const float BaseValue, const EModifierType ModifierType, UBuff* SourceBuff, const bool Stackable)
{
    Value = BaseValue;
    Type = ModifierType;
    Source = SourceBuff;
    bStackable = IsValid(Source) && Stackable;
}

float FCombatModifier::ApplyModifiers(const TArray<FCombatModifier>& ModArray, const float BaseValue)
{
    float AddMod = 0.0f;
    float MultMod = 1.0f;
    for (const FCombatModifier& Mod : ModArray)
    {
        switch (Mod.Type)
        {
        case EModifierType::Invalid :
            break;
        case EModifierType::Additive :
            AddMod += Mod.Value * ((IsValid(Mod.Source) && Mod.bStackable) ? Mod.Source->GetCurrentStacks() : 1);
            break;
        case EModifierType::Multiplicative :
            //This means no negative multipliers.
            //If you want a 5% reduction as a modifier, you would use .95. At, for example, 2 stacks, this would become:
            //.95 - 1, which is -.05, then -.05 * 2, which is -.1, then -.1 + 1, which is .9, so a 10% reduction.
            MultMod *= FMath::Max(0.0f, ((IsValid(Mod.Source) && Mod.bStackable) ? Mod.Source->GetCurrentStacks() : 1) * (Mod.Value - 1.0f) + 1.0f);
            break;
        default :
            break;
        }
    }
    return FMath::Max(0.0f, FMath::Max(0.0f, BaseValue + AddMod) * MultMod);
}

int32 FCombatModifier::ApplyModifiers(const TArray<FCombatModifier>& ModArray, const int32 BaseValue)
{
    const float ValueAsFloat = static_cast<float>(BaseValue);
    return static_cast<int32>(ApplyModifiers(ModArray, ValueAsFloat));
}