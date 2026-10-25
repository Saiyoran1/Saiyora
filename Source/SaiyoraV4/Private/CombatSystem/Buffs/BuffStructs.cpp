#include "BuffStructs.h"
#include "Buff.h"
#include "SaiyoraCombatLibrary.h"

#if !UE_BUILD_SHIPPING

void FBuffApplyEvent::DEBUG_ToString(FString& OutString) const
{
	OutString = FString::Printf(L"BuffClass: %s, AffectedBuff: %s, ActionTaken: %s, PreviousStacks: %i, NewStacks: %i, PreviousDuration: %.2f, NewDuration: %.2f, NewApplyTime: %.2f",
		IsValid(BuffClass) ? *BuffClass->GetName() : TEXT("NULL"), IsValid(AffectedBuff) ? *AffectedBuff->GetName() : TEXT("NULL"), *UEnum::GetDisplayValueAsText(ActionTaken).ToString(),
		PreviousStacks, NewStacks, PreviousDuration, NewDuration, NewApplyTime);
	if (FailReasons.Num() > 0)
	{
		OutString.Append(", FailReasons: ");
		for (int i = 0; i < FailReasons.Num(); i++)
		{
			if (i != 0)
			{
				OutString.Append(", ");
			}
			OutString.Append(*UEnum::GetDisplayValueAsText(FailReasons[i]).ToString());
		}
	}
	if (IsValid(AppliedBy))
	{
		OutString.Append(FString::Printf(L", AppliedBy: %s, OriginPlane: %s", *AppliedBy->GetName(), *UEnum::GetDisplayValueAsText(OriginPlane).ToString()));
	}
	if (IsValid(AppliedTo))
	{
		OutString.Append(FString::Printf(L", AppliedTo: %s, TargetPlane: %s", *AppliedTo->GetName(), *UEnum::GetDisplayValueAsText(TargetPlane).ToString()));
	}
	if (IsValid(Source))
	{
		OutString.Append(FString::Printf(L", Source: %s", *Source->GetName()));
	}
	if (CombatParams.Num() > 0)
	{
		OutString.Append(" Params: ");
		for (int i = 0; i < CombatParams.Num(); i++)
		{
			if (i != 0)
			{
				OutString.Append(", ");
			}
			OutString.Append(USaiyoraCombatLibrary::CombatParamToString(CombatParams[i]));
		}
	}
}

void FBuffRemoveEvent::DEBUG_ToString(FString& OutString) const
{
	OutString = FString::Printf(L"BuffClass: %s, AffectedBuff: %s, Result: %s, ExpireReason: %s",
		IsValid(RemovedBuff) ? *RemovedBuff->GetClass()->GetName() : TEXT("NULL"),
		IsValid(RemovedBuff) ? *RemovedBuff->GetName() : TEXT("NULL"),
		bResult ? TEXT("SUCCESS") : TEXT("FAILURE"),
		*UEnum::GetDisplayValueAsText(ExpireReason).ToString());
	if (IsValid(AppliedBy))
	{
		OutString.Append(FString::Printf(L", AppliedBy: %s", *AppliedBy->GetName()));
	}
	if (IsValid(RemovedFrom))
	{
		OutString.Append(FString::Printf(L", RemovedFrom: %s", *RemovedFrom->GetName()));
	}
}

#endif