#include "SaiyoraStructs.h"
#include "Buff.h"

FCombatModifier::FCombatModifier()
{
    //Needs to exist to prevent compile errors.
}

FCombatModifier::FCombatModifier(float const BaseValue, EModifierType const ModifierType, UBuff* SourceBuff, bool const Stackable)
{
    Value = BaseValue;
    Type = ModifierType;
    Source = SourceBuff;
    bStackable = IsValid(Source) && Stackable;
}

float FCombatModifier::ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue)
{
    float AddMod = 0.0f;
    float MultMod = 1.0f;
    for (FCombatModifier const& Mod : ModArray)
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

int32 FCombatModifier::ApplyModifiers(TArray<FCombatModifier> const& ModArray, int32 const BaseValue)
{
    float const ValueAsFloat = static_cast<float>(BaseValue);
    return static_cast<int32>(ApplyModifiers(ModArray, ValueAsFloat));
}