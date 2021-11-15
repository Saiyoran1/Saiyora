#include "SaiyoraDamageFunctions.h"
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
    DamageEvent.Info.Value = Amount;
    DamageEvent.Info.SnapshotValue = Amount;
    DamageEvent.Info.AppliedBy = AppliedBy;
    DamageEvent.Info.AppliedTo = AppliedTo;
    DamageEvent.Info.Source = Source;
    DamageEvent.Info.HitStyle = HitStyle;
    DamageEvent.Info.School = School;
    DamageEvent.Info.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    DamageEvent.Info.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
    DamageEvent.Info.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
        DamageEvent.Info.AppliedByPlane, DamageEvent.Info.AppliedToPlane);

    DamageEvent.ThreatInfo = ThreatParams;
     if (!ThreatParams.SeparateBaseThreat)
     {
         DamageEvent.ThreatInfo.BaseThreat = DamageEvent.Info.Value;
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
        //Damage that is not snapshotted and has a generator component should apply outgoing modifiers.
        if (!bFromSnapshot && IsValid(GeneratorComponent))
        {
            //Apply relevant outgoing mods, save off snapshot damage for use in DoTs.
            TArray<FCombatModifier> OutgoingMods;
            GeneratorComponent->GetOutgoingDamageMods(DamageEvent.Info, OutgoingMods);
            DamageEvent.Info.Value = FCombatModifier::ApplyModifiers(OutgoingMods, DamageEvent.Info.Value);
            DamageEvent.Info.SnapshotValue = DamageEvent.Info.Value;
        }
        //Apply relevant incoming mods.
        TArray<FCombatModifier> IncomingMods;
        TargetComponent->GetIncomingDamageMods(DamageEvent.Info, IncomingMods);
        DamageEvent.Info.Value = FCombatModifier::ApplyModifiers(IncomingMods, DamageEvent.Info.Value);
    }

    //Check for restrictions, if ignore restrictions is false.
    if (!bIgnoreRestrictions)
    {
        //Check incoming restrictions.
        if (TargetComponent->CheckIncomingDamageRestricted(DamageEvent.Info))
        {
            return DamageEvent;
        }
        //Check outgoing restrictions.
        if (IsValid(GeneratorComponent) && GeneratorComponent->CheckOutgoingDamageRestricted(DamageEvent.Info))
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

FDamagingEvent USaiyoraDamageFunctions::ApplyHealing(float const Amount, AActor* AppliedBy, AActor* AppliedTo, UObject* Source,
    EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions, bool const bIgnoreModifiers,
    bool const bFromSnapshot, FThreatFromDamage const& ThreatParams)
{
    //Initialize the event.
    FDamagingEvent HealingEvent;

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
    HealingEvent.Info.Value = Amount;
    HealingEvent.Info.SnapshotValue = Amount;
    HealingEvent.Info.AppliedBy = AppliedBy;
    HealingEvent.Info.AppliedTo = AppliedTo;
    HealingEvent.Info.Source = Source;
    HealingEvent.Info.HitStyle = HitStyle;
    HealingEvent.Info.School = School;
    HealingEvent.Info.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
    HealingEvent.Info.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
    HealingEvent.Info.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
        HealingEvent.Info.AppliedByPlane, HealingEvent.Info.AppliedToPlane);

    HealingEvent.ThreatInfo = ThreatParams;
    if (!ThreatParams.SeparateBaseThreat)
    {
        HealingEvent.ThreatInfo.BaseThreat = HealingEvent.Info.Value;
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
        if (!bFromSnapshot && IsValid(GeneratorComponent))
        {
            TArray<FCombatModifier> OutgoingMods;
            GeneratorComponent->GetOutgoingHealingMods(HealingEvent.Info, OutgoingMods);
            HealingEvent.Info.Value = FCombatModifier::ApplyModifiers(OutgoingMods, HealingEvent.Info.Value);
            HealingEvent.Info.SnapshotValue = HealingEvent.Info.Value;
        }
        //Apply relevant incoming mods.
        TArray<FCombatModifier> IncomingMods;
        TargetComponent->GetIncomingHealingMods(HealingEvent.Info, IncomingMods);
        HealingEvent.Info.Value = FCombatModifier::ApplyModifiers(IncomingMods, HealingEvent.Info.Value);
    }

    //Check for restrictions, if ignore restrictions is false.
    if (!bIgnoreRestrictions)
    {
        //Check incoming restrictions.
        if (TargetComponent->CheckIncomingHealingRestricted(HealingEvent.Info))
        {
            return HealingEvent;
        }
        //Check outgoing restrictions.
        if (IsValid(GeneratorComponent) && GeneratorComponent->CheckOutgoingHealingRestricted(HealingEvent.Info))
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
    UCombatComponent* TargetComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(AppliedTo);
    if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveDamage() || TargetComponent->GetLifeStatus() != ELifeStatus::Alive)
    {
        return 0.0f;
    }

    //Fill the struct.
    
    //Check for generator. Not required.
    UCombatComponent* GeneratorComponent = nullptr;
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(AppliedBy);
    }
    if (!bIgnoreModifiers && IsValid(GeneratorComponent))
    {
        FDamagingEvent DamageEvent;
        DamageEvent.Info.Value = Amount;
        DamageEvent.Info.SnapshotValue = Amount;
        DamageEvent.Info.AppliedBy = AppliedBy;
        DamageEvent.Info.AppliedTo = AppliedTo;
        DamageEvent.Info.Source = Source;
        DamageEvent.Info.HitStyle = HitStyle;
        DamageEvent.Info.School = School;
        DamageEvent.Info.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
        DamageEvent.Info.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
        DamageEvent.Info.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
            DamageEvent.Info.AppliedByPlane, DamageEvent.Info.AppliedToPlane);
        //Apply relevant outgoing mods.
        TArray<FCombatModifier> OutgoingMods;
        GeneratorComponent->GetOutgoingDamageMods(DamageEvent.Info, OutgoingMods);
        return FCombatModifier::ApplyModifiers(OutgoingMods, DamageEvent.Info.Value);
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
    UCombatComponent* TargetComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(AppliedTo);
    if (!IsValid(TargetComponent) || !TargetComponent->CanEverReceiveHealing() || TargetComponent->GetLifeStatus() != ELifeStatus::Alive)
    {
        return 0.0f;
    }

    //Fill the struct.
    
    //Check for generator. Not required.
    UCombatComponent* GeneratorComponent = nullptr;
    if (AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        GeneratorComponent = ISaiyoraCombatInterface::Execute_GetGenericCombatComponent(AppliedBy);
    }

    if (!bIgnoreModifiers && IsValid(GeneratorComponent))
    {
        FDamagingEvent HealingEvent;
        HealingEvent.Info.Value = Amount;
        HealingEvent.Info.SnapshotValue = Amount;
        HealingEvent.Info.AppliedBy = AppliedBy;
        HealingEvent.Info.AppliedTo = AppliedTo;
        HealingEvent.Info.Source = Source;
        HealingEvent.Info.HitStyle = HitStyle;
        HealingEvent.Info.School = School;
        HealingEvent.Info.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedBy);
        HealingEvent.Info.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(AppliedTo);
        HealingEvent.Info.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(
            HealingEvent.Info.AppliedByPlane, HealingEvent.Info.AppliedToPlane);
        //Apply relevant outgoing mods.
        TArray<FCombatModifier> OutgoingMods;
        GeneratorComponent->GetOutgoingHealingMods(HealingEvent.Info, OutgoingMods);
        return FCombatModifier::ApplyModifiers(OutgoingMods, HealingEvent.Info.Value);
    }
    return Amount;
}
