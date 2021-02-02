// Fill out your copyright notice in the Description page of Project Settings.


#include "StatStructs.h"
#include "StatHandler.h"


void FReplicatedStat::PostReplicatedChange(FReplicatedStatArray const& InArraySerializer)
{
    if (InArraySerializer.StatHandler)
    {
       InArraySerializer.StatHandler->NotifyOfReplicatedStat(StatTag, Value);
    }
}

void FReplicatedStat::PostReplicatedAdd(FReplicatedStatArray const& InArraySerializer)
{
    if (InArraySerializer.StatHandler)
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

