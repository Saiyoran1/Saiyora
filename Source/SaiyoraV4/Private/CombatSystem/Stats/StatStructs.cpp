// Fill out your copyright notice in the Description page of Project Settings.


#include "StatStructs.h"
#include "StatHandler.h"
#include "Buff.h"
#include "SaiyoraObjects.h"

void FCombatStat::Setup(AActor* Owner)
{
    StatValue = NewObject<UModifiableFloatValue>(Owner);
    FFloatValueCallback OnChange;
    OnChange.BindDynamic(this, &FCombatStat::BroadcastValueChange);
    StatValue->SubscribeToValueChanged(OnChange);
}

void FReplicatedStat::PostReplicatedChange(FReplicatedStatArray const& InArraySerializer)
{
    ClientOnChanged.Broadcast(StatTag, Value);
}

void FReplicatedStatArray::UpdateStatValue(FGameplayTag const& StatTag, float const NewValue)
{
    if (!StatTag.IsValid() || !StatTag.MatchesTag(UStatHandler::GenericStatTag()))
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

