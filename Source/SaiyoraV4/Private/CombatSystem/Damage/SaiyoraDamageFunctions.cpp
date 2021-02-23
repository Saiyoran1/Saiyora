// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraDamageFunctions.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

FDamagingEvent USaiyoraDamageFunctions::ApplyDamage(float Amount, AActor* AppliedBy, AActor* AppliedTo, UObject* Source,
    EDamageHitStyle HitStyle, EDamageSchool School, bool IgnoreRestrictions, bool IgnoreModifiers)
{
     //Initialize the event.
    FDamagingEvent DamageEvent;

    //Null checks.
    if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || !IsValid(Source))
    {
        return DamageEvent;
    }

    //Check for valid target component.
     if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
     {
         return DamageEvent;
     }
    UDamageHandler* TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedTo);
    if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveDamage() || TargetComponent->GetLifeStatus() != ELifeStatus::Alive)
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
    UDamageHandler* GeneratorComponent = nullptr;
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
    }
     if (IsValid(GeneratorComponent) && !GeneratorComponent->CanEverDealDamage())
     {
         return DamageEvent;
     }

    //Modify the damage, if ignore modifiers is false.
    if (!IgnoreModifiers)
    {
        //Add relevant incoming mods.
        TArray<FCombatModifier> Mods = TargetComponent->GetRelevantIncomingDamageMods(DamageEvent.DamageInfo);
        if (IsValid(GeneratorComponent))
        {
            //Add relevant outgoing mods.
            Mods.Append(GeneratorComponent->GetRelevantOutgoingDamageMods(DamageEvent.DamageInfo));
        }
        DamageEvent.DamageInfo.Damage = FCombatModifier::CombineModifiers(Mods, DamageEvent.DamageInfo.Damage);
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
        if (IsValid(GeneratorComponent) && GeneratorComponent->CheckOutgoingDamageRestricted(DamageEvent.DamageInfo))
        {
            return DamageEvent;
        }
    }

    //Send the complete event to the target component.
    TargetComponent->ApplyDamage(DamageEvent);

    //Notify the generator if one exists and the event was a success.
    if (DamageEvent.Result.Success && IsValid(GeneratorComponent))
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
    if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || !IsValid(Source))
    {
        return HealingEvent;
    }

    //Check for valid target component.
    if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return HealingEvent;
    }
    UDamageHandler* TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedTo);
    if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveHealing())
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
    UDamageHandler* GeneratorComponent = nullptr;
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
    }
    if (IsValid(GeneratorComponent) && !GeneratorComponent->CanEverDealHealing())
    {
        return HealingEvent;
    }

    //Modify the healing, if ignore modifiers is false.
    if (!IgnoreModifiers)
    {
        //Add relevant incoming mods.
        TArray<FCombatModifier> Mods = TargetComponent->GetRelevantIncomingHealingMods(HealingEvent.HealingInfo);
        if (IsValid(GeneratorComponent))
        {
            //Add relevant outgoing mods.
            Mods.Append(GeneratorComponent->GetRelevantOutgoingHealingMods(HealingEvent.HealingInfo));
        }
        HealingEvent.HealingInfo.Healing = FCombatModifier::CombineModifiers(Mods, HealingEvent.HealingInfo.Healing);
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
        if (IsValid(GeneratorComponent) && GeneratorComponent->CheckOutgoingHealingRestricted(HealingEvent.HealingInfo))
        {
            return HealingEvent;
        }
    }

    //Send the complete event to the target component.
    TargetComponent->ApplyHealing(HealingEvent);

    //Notify the generator if one exists and the event was a success.
    if (HealingEvent.Result.Success && IsValid(GeneratorComponent))
    {
       GeneratorComponent->NotifyOfOutgoingHealingSuccess(HealingEvent);
    }
    
    return HealingEvent;
}