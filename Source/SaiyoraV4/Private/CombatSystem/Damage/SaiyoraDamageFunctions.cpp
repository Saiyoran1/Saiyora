#include "SaiyoraDamageFunctions.h"
/*#include "MegaComponent/CombatComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

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
}*/
