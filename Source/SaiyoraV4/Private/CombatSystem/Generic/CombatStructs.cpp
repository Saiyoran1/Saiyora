#include "CombatStructs.h"
#include "Buff.h"

const ECollisionChannel FSaiyoraCollision::O_WorldAncient = ECC_GameTraceChannel12; 
const ECollisionChannel FSaiyoraCollision::O_WorldModern = ECC_GameTraceChannel13;
const ECollisionChannel FSaiyoraCollision::O_PlayerHitbox = ECC_GameTraceChannel10; 
const ECollisionChannel FSaiyoraCollision::O_NPCHitbox = ECC_GameTraceChannel11;
const ECollisionChannel FSaiyoraCollision::O_ProjectileHitbox = ECC_GameTraceChannel9;
const ECollisionChannel FSaiyoraCollision::T_Combat = ECC_GameTraceChannel1;

const FName FSaiyoraCollision::P_NoCollision = FName("NoCollision");
const FName FSaiyoraCollision::P_OverlapAll = FName("OverlapAll");
const FName FSaiyoraCollision::P_BlockAll = FName("BlockAllDynamic");
const FName FSaiyoraCollision::P_Pawn = FName("Pawn");
const FName FSaiyoraCollision::P_AncientPawn = FName("AncientPawn");
const FName FSaiyoraCollision::P_ModernPawn = FName("ModernPawn");
const FName FSaiyoraCollision::P_PlayerHitbox = FName("PlayerHitbox");
const FName FSaiyoraCollision::P_NPCHitbox = FName("NPCHitbox");
const FName FSaiyoraCollision::P_ProjectileHitboxAll = FName("NeutralProjectile");
const FName FSaiyoraCollision::P_ProjectileHitboxPlayers = FName("ProjectileHitsPlayers");
const FName FSaiyoraCollision::P_ProjectileHitboxNPCs = FName("ProjectileHitsNPCs");
const FName FSaiyoraCollision::P_ProjectileCollisionAll = FName("ProjectileCollision");
const FName FSaiyoraCollision::P_ProjectileCollisionAncient = FName("ProjectileCollisionAncient");
const FName FSaiyoraCollision::P_ProjectileCollisionModern = FName("ProjectileCollisionModern");

const FName FSaiyoraCollision::CT_All = FName("CombatTrace");
const FName FSaiyoraCollision::CT_Players = FName("CombatTracePlayers");
const FName FSaiyoraCollision::CT_NPCs = FName("CombatTraceNPCs");
const FName FSaiyoraCollision::CT_OverlapAll = FName("CombatTraceOverlap");
const FName FSaiyoraCollision::CT_OverlapPlayers = FName("CombatTraceOverlapPlayers");
const FName FSaiyoraCollision::CT_OverlapNPCs = FName("CombatTraceOverlapNPCs");
const FName FSaiyoraCollision::CT_AncientAll = FName("CombatTraceAncient");
const FName FSaiyoraCollision::CT_AncientPlayers = FName("CombatTraceAncientPlayers");
const FName FSaiyoraCollision::CT_AncientNPCs = FName("CombatTraceAncientNPCs");
const FName FSaiyoraCollision::CT_AncientOverlapAll = FName("CombatTraceAncientOverlap");
const FName FSaiyoraCollision::CT_AncientOverlapPlayers = FName("CombatTraceAncientOverlapPlayers");
const FName FSaiyoraCollision::CT_AncientOverlapNPCs = FName("CombatTraceAncientOverlapNPCs");
const FName FSaiyoraCollision::CT_ModernAll = FName("CombatTraceModern");
const FName FSaiyoraCollision::CT_ModernPlayers = FName("CombatTraceModernPlayers");
const FName FSaiyoraCollision::CT_ModernNPCs = FName("CombatTraceModernNPCs");
const FName FSaiyoraCollision::CT_ModernOverlapAll = FName("CombatTraceModernOverlap");
const FName FSaiyoraCollision::CT_ModernOverlapPlayers = FName("CombatTraceModernOverlapPlayers");
const FName FSaiyoraCollision::CT_ModernOverlapNPCs = FName("CombatTraceModernOverlapNPCs");

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