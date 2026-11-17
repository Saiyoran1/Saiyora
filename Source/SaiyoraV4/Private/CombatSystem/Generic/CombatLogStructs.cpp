#include "CombatLogStructs.h"
#include "Buff.h"
#include "BuffStructs.h"
#include "SaiyoraCombatInterface.h"
#include "CombatAbility.h"
#include "CombatLogComponent.h"
#include "SaiyoraCombatLibrary.h"

FLoggedEventSource::FLoggedEventSource(const UObject* InSource)
{
	if (IsValid(InSource))
	{
		SourceClass = InSource->GetClass();
		if (SourceClass->IsChildOf(UCombatAbility::StaticClass()))
		{
			Type = ECombatEventSourceType::Ability;
		}
		else if (SourceClass->IsChildOf(UBuff::StaticClass()))
		{
			Type = ECombatEventSourceType::Buff;
		}
		else if (SourceClass->IsChildOf(AActor::StaticClass())
			&& SourceClass->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			Type = ECombatEventSourceType::Combatant;
		}
		else
		{
			Type = ECombatEventSourceType::Object;
		}
	}
	else
	{
		Type = ECombatEventSourceType::None;
	}
}

FString FLoggedEventSource::ToString() const
{
	return FString::Printf(TEXT("{ %s:%s }"),
		*USaiyoraCombatLibrary::EnumValueToString(Type), *GetNameSafe(SourceClass));
}

FLoggedBuffApplyEvent::FLoggedBuffApplyEvent(const FBuffApplyEvent& InEvent, const int InID, const float InTime)
	: FCombatLogEvent(InID, InTime)
{
	AppliedTo = UCombatLogComponent::GetCombatantID(InEvent.AppliedTo);
	TargetPlane = InEvent.TargetPlane;
	AppliedBy = InEvent.AppliedByID;
	OriginPlane = InEvent.OriginPlane;
	Source = FLoggedEventSource(InEvent.Source);
	ActionTaken = InEvent.ActionTaken;
	BuffClass = InEvent.AffectedBuff->GetClass();
	PreviousStacks = InEvent.PreviousStacks;
	PreviousDuration = InEvent.PreviousDuration;
	NewStacks = InEvent.NewStacks;
	NewDuration = InEvent.NewDuration;
}

bool FLoggedBuffApplyEvent::Write(const UCombatLogComponent* LogComponent, FString& OutLog) const
{
	FText AppliedToName;
	if (AppliedTo.IsValid()
		&& !LogComponent->GetCombatantDisplayName(AppliedTo, AppliedToName))
	{
		return false;
	}
	FText AppliedByName;
	if (AppliedBy.IsValid()
		&& !LogComponent->GetCombatantDisplayName(AppliedBy, AppliedByName))
	{
		return false;
	}
	
	TArray<FString> Strings;
	Strings.Add(FString::Printf(TEXT("\"%s\""), *AppliedToName.ToString()));
	Strings.Add(USaiyoraCombatLibrary::EnumValueToString(TargetPlane));
	Strings.Add(FString::Printf(TEXT("\"%s\""), *AppliedByName.ToString()));
	Strings.Add(USaiyoraCombatLibrary::EnumValueToString(OriginPlane));
	Strings.Add(Source.ToString());
	Strings.Add(USaiyoraCombatLibrary::EnumValueToString(ActionTaken));
	Strings.Add(GetNameSafe(BuffClass));
	Strings.Add(FString::FromInt(PreviousStacks));
	Strings.Add(FString::Printf(TEXT("%.3f"), PreviousDuration));
	Strings.Add(FString::FromInt(NewStacks));
	Strings.Add(FString::Printf(TEXT("%.3f"), NewDuration));
	
	OutLog.Append(FString::Join(Strings, TEXT(",")));
	return true;
}

FLoggedBuffRemoveEvent::FLoggedBuffRemoveEvent(const FBuffRemoveEvent& InEvent, const int InID, const float InTime)
	: FCombatLogEvent(InID, InTime)
{
	RemovedFrom = UCombatLogComponent::GetCombatantID(InEvent.RemovedFrom);
	AppliedBy = InEvent.AppliedByID;
	BuffClass = InEvent.RemovedBuff->GetClass();
	ExpireReason = InEvent.ExpireReason;
}

bool FLoggedBuffRemoveEvent::Write(const UCombatLogComponent* LogComponent, FString& OutLog) const
{
	FText RemovedFromName;
	if (RemovedFrom.IsValid()
		&& !LogComponent->GetCombatantDisplayName(RemovedFrom, RemovedFromName))
	{
		return false;
	}
	FText AppliedByName;
	if (AppliedBy.IsValid()
		&& !LogComponent->GetCombatantDisplayName(AppliedBy, AppliedByName))
	{
		return false;
	}
	TArray<FString> Strings;
	Strings.Add(FString::Printf(TEXT("\"%s\""), *RemovedFromName.ToString()));
	Strings.Add(FString::Printf(TEXT("\"%s\""), *AppliedByName.ToString()));
	Strings.Add(GetNameSafe(BuffClass));
	Strings.Add(USaiyoraCombatLibrary::EnumValueToString(ExpireReason));

	OutLog.Append(FString::Join(Strings, TEXT(",")));
	return true;
}

void FCombatLogEventBatch::Clear()
{
	BuffApplications.Empty();
	BuffRemovals.Empty();
}

bool FCombatLogEventBatch::HasData() const
{
	return BuffApplications.Num() > 0
		|| BuffRemovals.Num() > 0;
}