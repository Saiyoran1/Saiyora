#include "DamageStructs.h"
#include "Buff.h"
#include "DamageBuffFunctions.h"

#if !UE_BUILD_SHIPPING
void FHealthEventResult::DEBUG_ToString(FString& OutString) const
{
	OutString = FString::Printf(L"Result: %s", bSuccess ? TEXT("SUCCESS") : TEXT("FAILURE"));
	if (bSuccess)
	{
		OutString.Append(FString::Printf(L", AppliedValue: %.2f, PreviousValue: %.2f, NewValue: %.2f, KillingBlow: %s, SnapshotValue: %.2f",
			AppliedValue, PreviousValue, NewValue, bKillingBlow ? TEXT("YES") : TEXT("NO"), Snapshot.Value));
		if (Snapshot.AppliedModifiers.Num() > 0)
		{
			OutString.Append(", Outgoing Modifiers: ");
			for (int i = 0; i < Snapshot.AppliedModifiers.Num(); i++)
			{
				const FModifierProxy& Modifier = Snapshot.AppliedModifiers[i];
				if (i > 0)
				{
					OutString.Append(", ");
				}
				FString ModifierString;
				Modifier.DEBUG_ToString(ModifierString);
				OutString.Append(FString::Printf(L"{ %s }", *ModifierString));
			}
		}
		if (IncomingModifiers.Num() > 0)
		{
			OutString.Append(", Incoming Modifiers: ");
			for (int i = 0; i < IncomingModifiers.Num(); i++)
			{
				const FModifierProxy& Modifier = IncomingModifiers[i];
				if (i > 0)
				{
					OutString.Append(", ");
				}
				FString ModifierString;
				Modifier.DEBUG_ToString(ModifierString);
				OutString.Append(FString::Printf(L"{ %s }", *ModifierString));
			}
		}
	}
	else
	{
		if (FailReasons.Num() > 0)
		{
			OutString.Append(", FailReasons: { ");
			for (int i = 0; i < FailReasons.Num(); i++)
			{
				if (i != 0)
				{
					OutString.Append(", ");
				}
				OutString.Append(*UEnum::GetDisplayValueAsText(FailReasons[i]).ToString());
			}
			OutString.Append(" }");
		}
		if (!IncomingRestriction.IsEmpty())
		{
			FString RestrictionString;
			IncomingRestriction.DEBUG_ToString(RestrictionString);
			OutString.Append(FString::Printf(L", IncomingRestriction: %s", *RestrictionString));
		}
		if (!OutgoingRestriction.IsEmpty())
		{
			FString RestrictionString;
			OutgoingRestriction.DEBUG_ToString(RestrictionString);
			OutString.Append(FString::Printf(L", OutgoingRestriction: %s", *RestrictionString));
		}
	}
}

void FHealthEventInfo::DEBUG_ToString(FString& OutString) const
{
	OutString = FString::Printf(L"EventID: %i, EventType: %s, HitStyle: %s, School: %s, Value: %.2f",
		EventID, *UEnum::GetDisplayValueAsText(EventType).ToString(), *UEnum::GetDisplayValueAsText(HitStyle).ToString(), *UEnum::GetDisplayValueAsText(School).ToString(), Value);
	if (IsValid(AppliedBy))
	{
		OutString.Append(FString::Printf(L", AppliedBy: %s, OriginPlane: %s", *AppliedBy->GetName(), *UEnum::GetDisplayValueAsText(AppliedByPlane).ToString()));
	}
	if (IsValid(AppliedTo))
	{
		OutString.Append(FString::Printf(L", AppliedTo: %s, TargetPlane: %s", *AppliedTo->GetName(), *UEnum::GetDisplayValueAsText(AppliedToPlane).ToString()));
	}
	if (IsValid(Source))
	{
		OutString.Append(FString::Printf(L", Source: %s", *Source->GetName()));
	}
}

void FHealthEvent::DEBUG_ToString(FString& OutString) const
{
	FString InfoString;
	Info.DEBUG_ToString(InfoString);
	FString ResultString;
	Result.DEBUG_ToString(ResultString);
	OutString = FString::Printf(L"Info: { %s }, Result: { %s }", *InfoString, *ResultString);
	//TODO: Once threat logging is in, add ThreatInfo.DEBUG_ToString() to this.
}
#endif

FPendingResurrection::FPendingResurrection(UPendingResurrectionFunction* InResFunction)
{
	Source = InResFunction;
	BuffClass = IsValid(InResFunction->GetOwningBuff()) ? InResFunction->GetOwningBuff()->GetClass() : nullptr;
	bOverrideHealth = InResFunction->bOverrideHealth;
	OverrideHealthPercent = InResFunction->HealthPercentageOverride;
	ResLocation = InResFunction->ResLocation;
	static int ResIDCounter = 0;
	ResID = ResIDCounter++;
}