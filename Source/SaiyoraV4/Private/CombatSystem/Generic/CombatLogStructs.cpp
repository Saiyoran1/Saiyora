#include "CombatLogStructs.h"
#include "Buff.h"
#include "BuffStructs.h"
#include "SaiyoraCombatInterface.h"
#include "CombatAbility.h"
#include "CombatLogComponent.h"
#include "SaiyoraGameState.h"

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

FLoggedBuffApplyEvent::FLoggedBuffApplyEvent(const FBuffApplyEvent& InEvent, const ASaiyoraGameState* GameStateRef)
{
	if (!IsValid(GameStateRef) || !IsValid(GameStateRef->GetCombatLog()))
	{
		return;
	}
	EventID = GameStateRef->GetCombatLog()->GetNextCombatEventID();
	Timestamp = GameStateRef->GetServerWorldTimeSeconds();
	
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
	//TODO: Write a string!
	return true;
}

FLoggedBuffRemoveEvent::FLoggedBuffRemoveEvent(const FBuffRemoveEvent& InEvent, const ASaiyoraGameState* GameStateRef)
{
	if (!IsValid(GameStateRef) || !IsValid(GameStateRef->GetCombatLog()))
	{
		return;
	}
	EventID = GameStateRef->GetCombatLog()->GetNextCombatEventID();
	Timestamp = GameStateRef->GetServerWorldTimeSeconds();
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
	//TODO: Write a string!
	return true;
}

#pragma region Combat Log

void FCombatLog::Init(UCombatLogComponent* InCompRef)
{
	CompRef = InCompRef;
	TryResolveEvents();
}

void FCombatLog::Clear()
{
	BuffApplications.Empty();
	BuffRemovals.Empty();
}

void FCombatLog::Append(const FCombatLog& NewEntries)
{
	NewEntries.ForEachCombatEvent([this](const auto& Event)
	{
		const auto& AddedEvent = AddEvent(Event);
		WriteEventToLog(AddedEvent);
	});
}

bool FCombatLog::HasData() const
{
	return BuffApplications.Num() > 0
		|| BuffRemovals.Num() > 0;
}

void FCombatLog::AddBuffApplyEvent(const FBuffApplyEvent& Event, ASaiyoraGameState* GameStateRef)
{
	BuffApplications.Emplace(Event, GameStateRef);
}

void FCombatLog::AddBuffRemoveEvent(const FBuffRemoveEvent& Event, ASaiyoraGameState* GameStateRef)
{
	BuffRemovals.Emplace(Event, GameStateRef);
}

void FCombatLog::TryResolveEvents()
{
	ForEachCombatEvent([this](const auto& Event)
	{
		if (!Event.bLogged)
		{
			WriteEventToLog(Event);
		}
	});
}

bool FCombatLog::HasValidComponentRef() const
{
	return IsValid(CompRef);
}

#pragma endregion 