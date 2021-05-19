// Fill out your copyright notice in the Description page of Project Settings.


#include "CrowdControl.h"
#include "Buff.h"
#include "CrowdControlHandler.h"
#include "SaiyoraBuffLibrary.h"

UBuff* UCrowdControl::FindNewDominantBuff()
{
    if (CrowdControls.Num() == 0)
    {
        return nullptr;
    }
    UBuff* Dominant = nullptr;
    float EndTime = 0.0f;
    for (UBuff* Buff : CrowdControls)
    {
        if (!Buff->GetFiniteDuration())
        {
            return Buff;
        }
        if (Buff->GetExpirationTime() > EndTime)
        {
            Dominant = Buff;
            EndTime = Buff->GetExpirationTime();
        }
    }
    return Dominant;
}

void UCrowdControl::InitializeCrowdControl(UCrowdControlHandler* OwningComponent)
{
    if (!IsValid(OwningComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("Crowd Control init with invalid handler."));
        return;
    }
    Handler = OwningComponent;
    bInitialized = true;
}

FCrowdControlStatus UCrowdControl::GetCurrentStatus() const
{
    if (Handler->GetOwnerRole() == ROLE_Authority)
    {
        FCrowdControlStatus Status;
        Status.CrowdControlClass = GetClass();
        Status.bActive = bActive;
        if (IsValid(DominantBuff))
        {
            Status.DominantBuff = DominantBuff->GetClass();
            Status.EndTime = DominantBuff->GetExpirationTime();
        }
        return Status;
    }
    
    FCrowdControlStatus Status;
    Status.CrowdControlClass = GetClass();
    Status.bActive = bActive;
    Status.DominantBuff = ReplicatedBuffClass;
    Status.EndTime = ReplicatedEndTime;
    return Status;
}

FCrowdControlStatus UCrowdControl::AddCrowdControl(UBuff* Source)
{
    FCrowdControlStatus const PreviousStatus = GetCurrentStatus();
    int32 const PreviousLength = CrowdControls.Num();
    CrowdControls.AddUnique(Source);
    if (PreviousLength == CrowdControls.Num())
    {
       return PreviousStatus;    
    }
    if (bActive && IsValid(DominantBuff))
    {
        if (DominantBuff->GetFiniteDuration())
        {
            if (Source->GetFiniteDuration())
            {
                if (DominantBuff->GetExpirationTime() < Source->GetExpirationTime())
                {
                    DominantBuff = Source;
                }
            }
            else
            {
                DominantBuff = Source;
            }
        }
    }
    else
    {
        bActive = true;
        DominantBuff = Source;
    }
    FCrowdControlStatus const NewStatus = GetCurrentStatus();
    if (NewStatus.bActive != PreviousStatus.bActive)
    {
        if (NewStatus.bActive)
        {
            OnActivation();
        }
        else
        {
            OnRemoval();
        }
    }
    return NewStatus;
}

FCrowdControlStatus UCrowdControl::RemoveCrowdControl(UBuff* Source)
{
    FCrowdControlStatus const PreviousStatus = GetCurrentStatus();
    bool const Removed = CrowdControls.RemoveSingleSwap(Source) != 0;
    if (!Removed)
    {
        return PreviousStatus;
    }
    if (CrowdControls.Num() == 0)
    {
        bActive = false;
        DominantBuff = nullptr;
    }
    else if (DominantBuff == Source)
    {
        DominantBuff = FindNewDominantBuff();
    }
    FCrowdControlStatus const NewStatus = GetCurrentStatus();
    if (NewStatus.bActive != PreviousStatus.bActive)
    {
        if (NewStatus.bActive)
        {
            OnActivation();
        }
        else
        {
            OnRemoval();
        }
    }
    return NewStatus;
}

FCrowdControlStatus UCrowdControl::UpdateCrowdControl(UBuff* RefreshedBuff)
{
    if (!CrowdControls.Contains(RefreshedBuff) || !IsValid(DominantBuff))
    {
        return AddCrowdControl(RefreshedBuff);
    }
    FCrowdControlStatus const PreviousStatus = GetCurrentStatus();
    FCrowdControlStatus NewStatus = GetCurrentStatus();
    if (RefreshedBuff == DominantBuff)
    {
        NewStatus.EndTime = RefreshedBuff->GetExpirationTime();
        return NewStatus;
    }
    if (!DominantBuff->GetFiniteDuration())
    {
        return PreviousStatus;
    }
    if (!RefreshedBuff->GetFiniteDuration())
    {
        DominantBuff = RefreshedBuff;
        NewStatus.DominantBuff = DominantBuff->GetClass();
        NewStatus.EndTime = 0.0f;
        return NewStatus;
    }
    if (DominantBuff->GetExpirationTime() >= RefreshedBuff->GetExpirationTime())
    {
        return PreviousStatus;
    }
    DominantBuff = RefreshedBuff;
    NewStatus.DominantBuff = DominantBuff->GetClass();
    NewStatus.EndTime = DominantBuff->GetExpirationTime();
    return NewStatus;
}

void UCrowdControl::RemoveNewlyRestrictedCrowdControls(FCrowdControlRestriction const& Restriction)
{
    if (!Restriction.IsBound())
    {
        return;
    }
    TArray<UBuff*> BuffsToRemove;
    for (UBuff* Buff : CrowdControls)
    {
        if (Restriction.Execute(Buff, GetClass()))
        {
            BuffsToRemove.AddUnique(Buff);
        }
    }
    for (UBuff* Buff : BuffsToRemove)
    {
        USaiyoraBuffLibrary::RemoveBuff(Buff, EBuffExpireReason::Dispel);
    }
}

FCrowdControlStatus UCrowdControl::UpdateCrowdControlFromReplication(bool const Active, TSubclassOf<UBuff> const SourceClass,
    float const EndTime)
{
    FCrowdControlStatus const PreviousStatus = GetCurrentStatus();
    bActive = Active;
    ReplicatedBuffClass = SourceClass;
    ReplicatedEndTime = EndTime;
    FCrowdControlStatus const NewStatus = GetCurrentStatus();
    if (NewStatus.bActive != PreviousStatus.bActive)
    {
        if (NewStatus.bActive)
        {
            OnActivation();
        }
        else
        {
            OnRemoval();
        }
    }
    return NewStatus;
}
