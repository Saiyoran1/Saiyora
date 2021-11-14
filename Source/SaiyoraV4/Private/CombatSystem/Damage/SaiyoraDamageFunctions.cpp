#include "SaiyoraDamageFunctions.h"
#include "DamageHandler.h"
#include "MegaComponent/CombatComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

FDamagingEvent USaiyoraDamageFunctions::ApplyDamage(float const Amount, AActor* AppliedBy, AActor* AppliedTo, UObject* Source,
    EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
    bool const bFromSnapshot, FThreatFromDamage const& ThreatParams)
{
     //Initialize the event.
    FDamagingEvent DamageEvent;

    //Null checks.
    if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || !IsValid(Source))
    {
        return DamageEvent;
    }

     if (AppliedBy->GetLocalRole() != ROLE_Authority)
     {
         return DamageEvent;
     }

    //Check for valid target component.
     if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
     {
         return DamageEvent;
     }
    UCombatComponent* TargetComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(AppliedTo);
    if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveDamage() || TargetComponent->GetLifeStatus() != ELifeStatus::Alive)
    {
        return DamageEvent;
    }

    //Fill the struct.
    DamageEvent.DamageInfo.Damage = Amount;
    DamageEvent.DamageInfo.SnapshotDamage = Amount;
    DamageEvent.DamageInfo.AppliedBy = AppliedBy;
    DamageEvent.DamageInfo.AppliedTo = AppliedTo;
    DamageEvent.DamageInfo.Source = Source;
    DamageEvent.DamageInfo.HitStyle = HitStyle;
    DamageEvent.DamageInfo.School = School;
    DamageEvent.DamageInfo.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    DamageEvent.DamageInfo.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
    DamageEvent.DamageInfo.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
        DamageEvent.DamageInfo.AppliedByPlane, DamageEvent.DamageInfo.AppliedToPlane);

    DamageEvent.ThreatInfo = ThreatParams;
     if (!ThreatParams.SeparateBaseThreat)
     {
         DamageEvent.ThreatInfo.BaseThreat = DamageEvent.DamageInfo.Damage;
     }
    
    //Check for generator. Not required.
    UCombatComponent* GeneratorComponent = nullptr;
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(AppliedBy);
    }

    //Modify the damage, if ignore modifiers is false.
    if (!bIgnoreModifiers)
    {
        //TODO: I think snapshot calculation might not work in situations without a generator component?
        if (!bFromSnapshot && IsValid(GeneratorComponent))
        {
            //Apply relevant outgoing mods, save off snapshot damage for use in DoTs.
            TArray<FCombatModifier> OutgoingMods;
            GeneratorComponent->GetOutgoingDamageMods(DamageEvent.DamageInfo, OutgoingMods);
            DamageEvent.DamageInfo.Damage = FCombatModifier::ApplyModifiers(OutgoingMods, DamageEvent.DamageInfo.Damage);
            DamageEvent.DamageInfo.SnapshotDamage = DamageEvent.DamageInfo.Damage;
        }
        //Apply relevant incoming mods.
        TArray<FCombatModifier> IncomingMods;
        TargetComponent->GetIncomingDamageMods(DamageEvent.DamageInfo, IncomingMods);
        DamageEvent.DamageInfo.Damage = FCombatModifier::ApplyModifiers(IncomingMods, DamageEvent.DamageInfo.Damage);
    }

    //Check for restrictions, if ignore restrictions is false.
    if (!bIgnoreRestrictions)
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
       GeneratorComponent->NotifyOfOutgoingDamage(DamageEvent);
    }
    
    return DamageEvent;
}

FHealingEvent USaiyoraDamageFunctions::ApplyHealing(float const Amount, AActor* AppliedBy, AActor* AppliedTo, UObject* Source,
    EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
    bool const bFromSnapshot, FThreatFromDamage const& ThreatParams)
{
    //Initialize the event.
    FHealingEvent HealingEvent;

    //Null checks.
    if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || !IsValid(Source))
    {
        return HealingEvent;
    }

    if (AppliedBy->GetLocalRole() != ROLE_Authority)
    {
        return HealingEvent;
    }

    //Check for valid target component.
    if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return HealingEvent;
    }
    UCombatComponent* TargetComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(AppliedTo);
    if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveHealing())
    {
        return HealingEvent;
    }

    //Fill the struct.
    HealingEvent.HealingInfo.Healing = Amount;
    HealingEvent.HealingInfo.SnapshotHealing = Amount;
    HealingEvent.HealingInfo.AppliedBy = AppliedBy;
    HealingEvent.HealingInfo.AppliedTo = AppliedTo;
    HealingEvent.HealingInfo.Source = Source;
    HealingEvent.HealingInfo.HitStyle = HitStyle;
    HealingEvent.HealingInfo.School = School;
    HealingEvent.HealingInfo.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    HealingEvent.HealingInfo.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
    HealingEvent.HealingInfo.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
        HealingEvent.HealingInfo.AppliedByPlane, HealingEvent.HealingInfo.AppliedToPlane);

    HealingEvent.ThreatInfo = ThreatParams;
    if (!ThreatParams.SeparateBaseThreat)
    {
        HealingEvent.ThreatInfo.BaseThreat = HealingEvent.HealingInfo.Healing;
    }
    
    //Check for generator. Not required.
    UCombatComponent* GeneratorComponent = nullptr;
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(AppliedBy);
    }

    //Modify the healing, if ignore modifiers is false.
    if (!bIgnoreModifiers)
    {
        //Apply relevant outgoing modifiers, save off snapshot healing for use in HoTs.
        //TODO: I think snapshot calculation might not work in situations without a generator component?
        if (!bFromSnapshot && IsValid(GeneratorComponent))
        {
            TArray<FCombatModifier> OutgoingMods;
            GeneratorComponent->GetOutgoingHealingMods(HealingEvent.HealingInfo, OutgoingMods);
            HealingEvent.HealingInfo.Healing = FCombatModifier::ApplyModifiers(OutgoingMods, HealingEvent.HealingInfo.Healing);
            HealingEvent.HealingInfo.SnapshotHealing = HealingEvent.HealingInfo.Healing;
        }
        //Apply relevant incoming mods.
        TArray<FCombatModifier> IncomingMods;
        TargetComponent->GetIncomingHealingMods(HealingEvent.HealingInfo, IncomingMods);
        HealingEvent.HealingInfo.Healing = FCombatModifier::ApplyModifiers(IncomingMods, HealingEvent.HealingInfo.Healing);
    }

    //Check for restrictions, if ignore restrictions is false.
    if (!bIgnoreRestrictions)
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
       GeneratorComponent->NotifyOfOutgoingHealing(HealingEvent);
    }
    
    return HealingEvent;
}

float USaiyoraDamageFunctions::GetSnapshotDamage(float const Amount, AActor* AppliedBy, AActor* AppliedTo,
    UObject* Source, EDamageHitStyle const HitStyle, EDamageSchool const School,
    bool const bIgnoreModifiers)
{
    //Null checks.
    if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || !IsValid(Source))
    {
        return 0.0f;
    }

    //Check for valid target component.
     if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
     {
         return 0.0f;
     }
    UDamageHandler* TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedTo);
    if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveDamage() || TargetComponent->GetLifeStatus() != ELifeStatus::Alive)
    {
        return 0.0f;
    }

    //Fill the struct.
    
    //Check for generator. Not required.
    UDamageHandler* GeneratorComponent = nullptr;
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
    }
     if (IsValid(GeneratorComponent) && !GeneratorComponent->CanEverDealDamage())
     {
         return 0.0f;
     }

    if (!bIgnoreModifiers && IsValid(GeneratorComponent))
    {
        FDamagingEvent DamageEvent;
        DamageEvent.DamageInfo.Damage = Amount;
        DamageEvent.DamageInfo.SnapshotDamage = Amount;
        DamageEvent.DamageInfo.AppliedBy = AppliedBy;
        DamageEvent.DamageInfo.AppliedTo = AppliedTo;
        DamageEvent.DamageInfo.Source = Source;
        DamageEvent.DamageInfo.HitStyle = HitStyle;
        DamageEvent.DamageInfo.School = School;
        DamageEvent.DamageInfo.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
        DamageEvent.DamageInfo.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
        DamageEvent.DamageInfo.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
            DamageEvent.DamageInfo.AppliedByPlane, DamageEvent.DamageInfo.AppliedToPlane);
        //Apply relevant outgoing mods.
        return FCombatModifier::ApplyModifiers(
            GeneratorComponent->GetOutgoingDamageMods(DamageEvent.DamageInfo), DamageEvent.DamageInfo.Damage);
    }
    return Amount;
}

float USaiyoraDamageFunctions::GetSnapshotHealing(float const Amount, AActor* AppliedBy, AActor* AppliedTo,
    UObject* Source, EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreModifiers)
{
    //Null checks.
    if (!IsValid(AppliedBy) || !IsValid(AppliedTo) || !IsValid(Source))
    {
        return 0.0f;
    }

    //Check for valid target component.
     if (!AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
     {
         return 0.0f;
     }
    UDamageHandler* TargetComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedTo);
    if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveHealing() || TargetComponent->GetLifeStatus() != ELifeStatus::Alive)
    {
        return 0.0f;
    }

    //Fill the struct.
    
    //Check for generator. Not required.
    UDamageHandler* GeneratorComponent = nullptr;
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
    }
     if (IsValid(GeneratorComponent) && !GeneratorComponent->CanEverDealHealing())
     {
         return 0.0f;
     }

    if (!bIgnoreModifiers && IsValid(GeneratorComponent))
    {
        FHealingEvent HealingEvent;
        HealingEvent.HealingInfo.Healing = Amount;
        HealingEvent.HealingInfo.SnapshotHealing = Amount;
        HealingEvent.HealingInfo.AppliedBy = AppliedBy;
        HealingEvent.HealingInfo.AppliedTo = AppliedTo;
        HealingEvent.HealingInfo.Source = Source;
        HealingEvent.HealingInfo.HitStyle = HitStyle;
        HealingEvent.HealingInfo.School = School;
        HealingEvent.HealingInfo.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
        HealingEvent.HealingInfo.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
        HealingEvent.HealingInfo.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
            HealingEvent.HealingInfo.AppliedByPlane, HealingEvent.HealingInfo.AppliedToPlane);
        //Apply relevant outgoing mods.
        return FCombatModifier::ApplyModifiers(
            GeneratorComponent->GetOutgoingHealingMods(HealingEvent.HealingInfo), HealingEvent.HealingInfo.Healing);
    }
    return Amount;
}
