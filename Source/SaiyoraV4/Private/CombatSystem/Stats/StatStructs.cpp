// Fill out your copyright notice in the Description page of Project Settings.


#include "StatStructs.h"
#include "StatHandler.h"
#include "Buff.h"


FCombatModifier FStatModifier::GetModifier() const
{
    FCombatModifier OutMod;
    OutMod.ModifierType = Modifier.ModType;
    OutMod.ModifierValue = Modifier.ModValue;
    if (Modifier.bStackable)
    {
        OutMod.Stacks = IsValid(Modifier.Source) ? Modifier.Source->GetCurrentStacks() : 1;
    }
    else
    {
        OutMod.Stacks = 1;
    }
    return OutMod;
}

void FReplicatedStat::PostReplicatedChange(FReplicatedStatArray const& InArraySerializer)
{
    if (IsValid(InArraySerializer.StatHandler))
    {
       InArraySerializer.StatHandler->NotifyOfReplicatedStat(StatTag, Value);
    }
}

void FReplicatedStat::PostReplicatedAdd(FReplicatedStatArray const& InArraySerializer)
{
    if (IsValid(InArraySerializer.StatHandler))
    {
        InArraySerializer.StatHandler->NotifyOfReplicatedStat(StatTag, Value);
    }
}

void FReplicatedStatArray::UpdateStatValue(FGameplayTag const& StatTag, float const NewValue)
{
    if (!StatTag.IsValid() || !StatTag.MatchesTag(UStatHandler::GenericStatTag))
    {
        return;
    }
    for (FReplicatedStat& Stat : Items)
    {
        if (Stat.StatTag.MatchesTagExact(StatTag))
        {
            Stat.Value = NewValue;
            MarkItemDirty(Stat);
            return;
        }
    }
}

