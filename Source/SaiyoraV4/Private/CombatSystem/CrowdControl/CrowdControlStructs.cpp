#include "CrowdControlStructs.h"
#include "Buff.h"

const FGameplayTag FCcTags::GenericCrowdControl = FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl")), false);
const FGameplayTag FCcTags::Stun = FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Stun")), false);
const FGameplayTag FCcTags::Incapacitate = FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Incapacitate")), false);
const FGameplayTag FCcTags::Root = FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Root")), false);
const FGameplayTag FCcTags::Silence = FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Silence")), false);
const FGameplayTag FCcTags::Disarm = FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Disarm")), false);

bool FCrowdControlStatus::AddNewBuff(UBuff* Source)
{
    if (!IsValid(Source) || Sources.Contains(Source))
    {
        return false;
    }
    Sources.Add(Source);
    if (!bActive || !IsValid(DominantBuffInstance) || !IsValid(DominantBuffClass))
    {
        bActive = true;
        SetNewDominantBuff(Source);
        return true;
    }
    if (DominantBuffInstance->HasFiniteDuration() && (!Source->HasFiniteDuration() || DominantBuffInstance->GetExpirationTime() <= Source->GetExpirationTime()))
    {
        SetNewDominantBuff(Source);
        return true;
    }
    return false;
}

bool FCrowdControlStatus::RemoveBuff(UBuff* Source)
{
    if (!IsValid(Source))
    {
        return false;
    }
    if (Sources.Remove(Source) > 0 && (Sources.Num() == 0 || DominantBuffInstance == Source))
    {
        SetNewDominantBuff(GetLongestBuff());
        return true;
    }
    return false;
}

bool FCrowdControlStatus::RefreshBuff(UBuff* Source)
{
    if (!IsValid(Source) || !Sources.Contains(Source))
    {
        return false;
    }
    if (IsValid(DominantBuffInstance) && (!DominantBuffInstance->HasFiniteDuration() || (Source->HasFiniteDuration() && Source->GetExpirationTime() <= DominantBuffInstance->GetExpirationTime())))
    {
        return false;
    }
    SetNewDominantBuff(Source);
    return true;
}

void FCrowdControlStatus::SetNewDominantBuff(UBuff* NewDominant)
{
    DominantBuffInstance = NewDominant;
    if (IsValid(NewDominant))
    {
        DominantBuffClass = NewDominant->GetClass();
        EndTime = NewDominant->HasFiniteDuration() ? NewDominant->GetExpirationTime() : 0.0f;
    }
    else
    {
        DominantBuffClass = nullptr;
        EndTime = 0.0f;
        bActive = false;
    }
}

UBuff* FCrowdControlStatus::GetLongestBuff()
{
    UBuff* LongestBuff = nullptr;
    float Expire = -1.0f;
    for (UBuff* Buff : Sources)
    {
        if (!IsValid(Buff))
        {
            continue;
        }
        if (!Buff->HasFiniteDuration())
        {
            return Buff;
        }
        if (Buff->GetExpirationTime() > Expire)
        {
            LongestBuff = Buff;
            Expire = Buff->GetExpirationTime();
        }
    }
    return LongestBuff;
}

