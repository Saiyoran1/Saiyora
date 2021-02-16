// Fill out your copyright notice in the Description page of Project Settings.


#include "CrowdControlStructs.h"
#include "CrowdControlHandler.h"

void FReplicatedCrowdControl::PostReplicatedAdd(const FReplicatedCrowdControlArray& InArraySerializer)
{
    if (!IsValid(InArraySerializer.Handler))
    {
        return;
    }
    InArraySerializer.Handler->UpdateCrowdControlFromReplication(*this);
}

void FReplicatedCrowdControl::PostReplicatedChange(const FReplicatedCrowdControlArray& InArraySerializer)
{
    if (!IsValid(InArraySerializer.Handler))
    {
        return;
    }
    InArraySerializer.Handler->UpdateCrowdControlFromReplication(*this);
}

void FReplicatedCrowdControl::PreReplicatedRemove(const FReplicatedCrowdControlArray& InArraySerializer)
{
    if (!IsValid(InArraySerializer.Handler))
    {
        return;
    }
    InArraySerializer.Handler->RemoveCrowdControlFromReplication(CrowdControlClass);
}

void FReplicatedCrowdControlArray::UpdateArray(FCrowdControlStatus const& NewStatus)
{
    for (FReplicatedCrowdControl& Item : Items)
    {
        if (Item.CrowdControlClass == NewStatus.CrowdControlClass)
        {
            Item.bActive = NewStatus.bActive;
            Item.DominantBuff = NewStatus.DominantBuff;
            Item.EndTime = NewStatus.EndTime;
            MarkItemDirty(Item);
            return;
        }
    }
    FReplicatedCrowdControl NewItem;
    NewItem.CrowdControlClass = NewStatus.CrowdControlClass;
    NewItem.bActive = NewStatus.bActive;
    NewItem.DominantBuff = NewStatus.DominantBuff;
    NewItem.EndTime = NewStatus.EndTime;
    MarkItemDirty(Items.Add_GetRef(NewItem));
}
