// Fill out your copyright notice in the Description page of Project Settings.
#include "SaiyoraStructs.h"

#include "Buff.h"

int32 FModifierCollection::GlobalID = 0;

void FCombatModifier::Reset()
{
    Source = nullptr;
    bFromBuff = false;
    ModType = EModifierType::Invalid;
    ModValue = 0.0f;
    bStackable = false;
}

void FCombatModifier::Activate()
{
    if (bFromBuff && IsValid(Source))
    {
        //TODO: Add per-buff callbacks in addition to handler callbacks.
        //Subscribe to owning buff stacked, passing OnBuffStacked.
        //Subscribe to owning buff removed, passing OnBuffRemoved.
    }
}

void FCombatModifier::OnBuffStacked(FBuffApplyEvent const& Event)
{
    if (Event.Result.AffectedBuff == Source && Event.Result.PreviousStacks != Event.Result.NewStacks)
    {
        OnStacksChanged.Broadcast();
    }
}

void FCombatModifier::OnBuffRemoved(FBuffRemoveEvent const& Event)
{
    if (Event.RemovedBuff == Source)
    {
        ModType = EModifierType::Invalid;
        OnSourceRemoved.Broadcast();
    }
}

float FCombatModifier::ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue)
{
    FCombatModifier AddMod;
    FCombatModifier MultMod;
    CombineModifiers(ModArray, AddMod, MultMod);
    return FMath::Max(0.0f, FMath::Max(0.0f, BaseValue + AddMod.ModValue) * MultMod.ModValue);
}

void FCombatModifier::CombineModifiers(TArray<FCombatModifier> const& ModArray, FCombatModifier& OutAddMod,
    FCombatModifier& OutMultMod)
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
    OutAddMod.Reset();
    OutAddMod.ModType = EModifierType::Additive;
    OutAddMod.ModValue = AddMod;
    OutMultMod.Reset();
    OutMultMod.ModType = EModifierType::Multiplicative;
    OutMultMod.ModValue = MultMod;
}

int32 FModifierCollection::GetID()
{
    GlobalID++;
    if (GlobalID == -1)
    {
        GlobalID++;
    }
    return GlobalID;
}

int32 FModifierCollection::AddModifier(FCombatModifier const& Modifier)
{
    if (Modifier.ModType == EModifierType::Invalid)
    {
        return -1;
    }
    int32 const ModID = GetID();
    FCombatModifier& NewMod = IndividualModifiers.Add(ModID, Modifier);
    NewMod.Activate();
    FModifierCallback StackCallback;
    StackCallback.BindRaw(this, &FModifierCollection::RecalculateMods);
    NewMod.OnStacksChanged.Add(StackCallback);
    FModifierCallback RemoveCallback;
    RemoveCallback.BindRaw(this, &FModifierCollection::PurgeInvalidMods);
    NewMod.OnSourceRemoved.Add(RemoveCallback);
    RecalculateMods();
    return ModID;
}

void FModifierCollection::RemoveModifier(int32 const ModifierID)
{
    if (ModifierID == -1)
    {
        return;
    }
    if (IndividualModifiers.Remove(ModifierID) > 0)
    {
        RecalculateMods();
    }
}

void FModifierCollection::RecalculateMods()
{
    float const PreviousAddMod = SummedAddMod.ModValue;
    float const PreviousMultMod = SummedMultMod.ModValue;
    TArray<FCombatModifier> Mods;
    IndividualModifiers.GenerateValueArray(Mods);
    FCombatModifier::CombineModifiers(Mods, SummedAddMod, SummedMultMod);
    if (PreviousAddMod != SummedAddMod.ModValue || PreviousMultMod != SummedMultMod.ModValue)
    {
        OnModifiersChanged.Broadcast();
    }
}

void FModifierCollection::PurgeInvalidMods()
{
    TArray<int32> InvalidIDs;
    for (TTuple<int32, FCombatModifier> const& Mod : IndividualModifiers)
    {
        if (Mod.Value.ModType == EModifierType::Invalid)
        {
            InvalidIDs.Add(Mod.Key);
        }
    }
    int32 Removed = 0;
    for (int32 const ID : InvalidIDs)
    {
        Removed += IndividualModifiers.Remove(ID);
    }
    if (Removed > 0)
    {
        RecalculateMods();
    }
}