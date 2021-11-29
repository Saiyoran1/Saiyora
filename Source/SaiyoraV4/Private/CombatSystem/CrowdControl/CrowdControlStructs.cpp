#include "CrowdControlStructs.h"
#include "Buff.h"

bool FCrowdControlStatus::AddNewBuff(UBuff* Source)
{
    if (!IsValid(Source) || Sources.Contains(Source))
    {
        return false;
    }
    Sources.AddUnique(Source);
    if (!bActive)
    {
        bActive = true;
        SetNewDominantBuff(Source);
        return true;
    }
    if (!IsValid(DominantBuffInstance) || !IsValid(DominantBuffClass))
    {
        SetNewDominantBuff(Source);
        return true;
    }
    if (!DominantBuffInstance->HasFiniteDuration())
    {
        return false;
    }
    if (!Source->HasFiniteDuration())
    {
        SetNewDominantBuff(Source);
        return true;
    }
    if (DominantBuffInstance->GetExpirationTime() > Source->GetExpirationTime())
    {
        return false;
    }
    SetNewDominantBuff(Source);
    return true;
}

bool FCrowdControlStatus::RemoveBuff(UBuff* Source)
{
    if (!IsValid(Source) || !Sources.Contains(Source))
    {
        return false;
    }
    Sources.Remove(Source);
    if (Sources.Num() == 0)
    {
        bActive = false;
        SetNewDominantBuff(nullptr);
        return true;
    }
    if (DominantBuffInstance != Source)
    {
        return false;
    }
    UBuff* Longest = GetLongestBuff();
    SetNewDominantBuff(Longest);
    return true;
}

bool FCrowdControlStatus::RefreshBuff(UBuff* Source)
{
    if (!IsValid(Source) || !Sources.Contains(Source))
    {
        return false;
    }
    UBuff* Longest = GetLongestBuff();
    if (DominantBuffInstance == Longest)
    {
        return false;
    }
    SetNewDominantBuff(Longest);
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
            LongestBuff = Buff;
            break;
        }
        if (Buff->GetExpirationTime() > Expire)
        {
            LongestBuff = Buff;
            Expire = Buff->GetExpirationTime();
        }
    }
    return LongestBuff;
}

