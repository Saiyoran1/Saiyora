#include "CrowdControlStructs.h"
#include "Buff.h"

bool FCrowdControlStatus::AddNewBuff(UBuff* Source)
{
    if (!IsValid(Source) || Sources.Contains(Source))
    {
        return false;
    }
    Sources.Add(Source);
    //If this is the only buff applying this crowd control, we need to set active and set dominant buff
    if (!bActive || !IsValid(DominantBuffInstance) || !IsValid(DominantBuffClass))
    {
        bActive = true;
        SetNewDominantBuff(Source);
        return true;
    }
    //If this buff has a longer duration than the current dominant buff, update the dominant buff
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
    //We want to return true when the dominant buff changes, or false if it didn't
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
    //Return true if the dominant buff changes, or false if it didn't
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

