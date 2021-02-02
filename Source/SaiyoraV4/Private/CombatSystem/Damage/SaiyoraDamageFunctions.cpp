// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraDamageFunctions.h"
#include "DamageHandler.h"
#include "SaiyoraCombatLibrary.h"

UDamageHandler* USaiyoraDamageFunctions::GetDamageHandler(AActor* Target)
{
    if (!Target)
    {
        return nullptr;
    }
    return Target->FindComponentByClass<UDamageHandler>();
}

FDamagingEvent USaiyoraDamageFunctions::ApplyDamage(float Amount, AActor* AppliedBy, AActor* AppliedTo, UObject* Source,
    EDamageHitStyle HitStyle, EDamageSchool School, bool IgnoreRestrictions, bool IgnoreModifiers)
{
     //Initialize the event.
    FDamagingEvent DamageEvent;

    //Null checks.
    if (!AppliedBy || !AppliedTo || !Source)
    {
        return DamageEvent;
    }

    //Check for valid target component.
    UDamageHandler* TargetComponent = GetDamageHandler(AppliedTo);
    if (!TargetComponent || !TargetComponent->CanEverReceiveDamage() || TargetComponent->GetLifeStatus() != ELifeStatus::Alive)
    {
        return DamageEvent;
    }

    //Fill the struct.
    DamageEvent.DamageInfo.Damage = Amount;
    DamageEvent.DamageInfo.AppliedBy = AppliedBy;
    DamageEvent.DamageInfo.AppliedTo = AppliedTo;
    DamageEvent.DamageInfo.Source = Source;
    DamageEvent.DamageInfo.HitStyle = HitStyle;
    DamageEvent.DamageInfo.School = School;
    DamageEvent.DamageInfo.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    DamageEvent.DamageInfo.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
    DamageEvent.DamageInfo.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
        DamageEvent.DamageInfo.AppliedByPlane, DamageEvent.DamageInfo.AppliedToPlane);
    
    //Check for generator. Not required.
    UDamageHandler* GeneratorComponent = GetDamageHandler(AppliedBy);

    //Modify the damage, if ignore modifiers is false.
    if (!IgnoreModifiers)
    {
        //Add relevant incoming mods.
        TArray<FCombatModifier> Mods = TargetComponent->GetRelevantIncomingDamageMods(DamageEvent.DamageInfo);
        if (GeneratorComponent && GeneratorComponent->CanEverDealDamage())
        {
            //Add relevant outgoing mods.
            Mods.Append(GeneratorComponent->GetRelevantOutgoingDamageMods(DamageEvent.DamageInfo));
        }

        float AdditiveMod = 0.0f;
        float MultMod = 1.0f;
        //Combine mods into additive and multiplicative values.
        for (FCombatModifier const& Mod : Mods)
        {
            switch (Mod.ModifierType)
            {
                case EModifierType::Invalid :
                    break;
                case EModifierType::Additive :
                    AdditiveMod += Mod.ModifierValue;
                    break;
                case EModifierType::Multiplicative :
                    //Negative multiplicative modifiers are not allowed.
                    MultMod *= FMath::Max(0.0f, Mod.ModifierValue);
                    break;
            }
        }
        //Apply additive mod first. Clamp at 0.
        DamageEvent.DamageInfo.Damage = FMath::Max(0.0f, DamageEvent.DamageInfo.Damage + AdditiveMod);
        //Apply multiplicative mod. No need to clamp as we already clamped each mod individually.
        DamageEvent.DamageInfo.Damage *= MultMod;
    }

    //Check for restrictions, if ignore restrictions is false.
    if (!IgnoreRestrictions)
    {
        //Check incoming restrictions.
        if (TargetComponent->CheckIncomingDamageRestricted(DamageEvent.DamageInfo))
        {
            return DamageEvent;
        }
        //Check outgoing restrictions.
        if (GeneratorComponent && GeneratorComponent->CanEverDealDamage() && GeneratorComponent->CheckOutgoingDamageRestricted(DamageEvent.DamageInfo))
        {
            return DamageEvent;
        }
    }

    //Send the complete event to the target component.
    TargetComponent->ApplyDamage(DamageEvent);

    //Notify the generator if one exists and the event was a success.
    if (DamageEvent.Result.Success && GeneratorComponent && GeneratorComponent->CanEverDealDamage())
    {
       GeneratorComponent->NotifyOfOutgoingDamageSuccess(DamageEvent);
    }
    
    return DamageEvent;
}

FHealingEvent USaiyoraDamageFunctions::ApplyHealing(float Amount, AActor* AppliedBy, AActor* AppliedTo, UObject* Source,
    EDamageHitStyle HitStyle, EDamageSchool School, bool IgnoreRestrictions, bool IgnoreModifiers)
{
    //Initialize the event.
    FHealingEvent HealingEvent;

    //Null checks.
    if (!AppliedBy || !AppliedTo || !Source)
    {
        return HealingEvent;
    }

    //Check for valid target component.
    UDamageHandler* TargetComponent = AppliedTo->FindComponentByClass<UDamageHandler>();
    if (!TargetComponent || !TargetComponent->CanEverReceiveHealing())
    {
        return HealingEvent;
    }

    //Fill the struct.
    HealingEvent.HealingInfo.Healing = Amount;
    HealingEvent.HealingInfo.AppliedBy = AppliedBy;
    HealingEvent.HealingInfo.AppliedTo = AppliedTo;
    HealingEvent.HealingInfo.Source = Source;
    HealingEvent.HealingInfo.HitStyle = HitStyle;
    HealingEvent.HealingInfo.School = School;
    HealingEvent.HealingInfo.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    HealingEvent.HealingInfo.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
    HealingEvent.HealingInfo.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
        HealingEvent.HealingInfo.AppliedByPlane, HealingEvent.HealingInfo.AppliedToPlane);
    
    //Check for generator. Not required.
    UDamageHandler* GeneratorComponent = AppliedBy->FindComponentByClass<UDamageHandler>();

    //Modify the healing, if ignore modifiers is false.
    if (!IgnoreModifiers)
    {
        TArray<FCombatModifier> Mods;
        //Add relevant incoming mods.
        Mods.Append(TargetComponent->GetRelevantIncomingHealingMods(HealingEvent.HealingInfo));
        if (GeneratorComponent && GeneratorComponent->CanEverDealHealing())
        {
            //Add relevant outgoing mods.
            Mods.Append(GeneratorComponent->GetRelevantOutgoingHealingMods(HealingEvent.HealingInfo));
        }

        float AdditiveMod = 0.0f;
        float MultMod = 1.0f;
        //Combine mods into additive and multiplicative values.
        for (FCombatModifier const& Mod : Mods)
        {
            switch (Mod.ModifierType)
            {
                case EModifierType::Invalid :
                    break;
                case EModifierType::Additive :
                    AdditiveMod += Mod.ModifierValue;
                    break;
                case EModifierType::Multiplicative :
                    //Negative multiplicative modifiers are not allowed.
                    MultMod *= FMath::Max(0.0f, Mod.ModifierValue);
                    break;
            }
        }
        //Apply additive mod first. Clamp at 0.
        HealingEvent.HealingInfo.Healing = FMath::Max(0.0f, HealingEvent.HealingInfo.Healing + AdditiveMod);
        //Apply multiplicative mod. No need to clamp as we already clamped each mod individually.
        HealingEvent.HealingInfo.Healing *= MultMod;
    }

    //Check for restrictions, if ignore restrictions is false.
    if (!IgnoreRestrictions)
    {
        //Check incoming restrictions.
        if (TargetComponent->CheckIncomingHealingRestricted(HealingEvent.HealingInfo))
        {
            return HealingEvent;
        }
        //Check outgoing restrictions.
        if (GeneratorComponent && GeneratorComponent->CanEverDealHealing() && GeneratorComponent->CheckOutgoingHealingRestricted(HealingEvent.HealingInfo))
        {
            return HealingEvent;
        }
    }

    //Send the complete event to the target component.
    TargetComponent->ApplyHealing(HealingEvent);

    //Notify the generator if one exists and the event was a success.
    if (HealingEvent.Result.Success && GeneratorComponent && GeneratorComponent->CanEverDealHealing())
    {
       GeneratorComponent->NotifyOfOutgoingHealingSuccess(HealingEvent);
    }
    
    return HealingEvent;
}