#include "CombatLogStructs.h"
#include "Buff.h"
#include "BuffStructs.h"
#include "CombatLogComponent.h"
#include "SaiyoraGameState.h"

FLoggedBuffApplyEvent::FLoggedBuffApplyEvent(const FBuffApplyEvent& InEvent, const ASaiyoraGameState* GameStateRef)
{
	if (!IsValid(GameStateRef) || !IsValid(GameStateRef->GetCombatLog()))
	{
		return;
	}
	EventID = GameStateRef->GetCombatLog()->GetNextCombatEventID();
	Timestamp = GameStateRef->GetServerWorldTimeSeconds();
	AppliedTo = GameStateRef->GetCombatLog()->GetCombatantID(InEvent.AppliedTo);
	TargetPlane = InEvent.TargetPlane;
	AppliedBy = GameStateRef->GetCombatLog()->GetCombatantID(InEvent.AppliedBy);
	OriginPlane = InEvent.OriginPlane;
	Source = InEvent.Source;
	ActionTaken = InEvent.ActionTaken;
	BuffClass = InEvent.AffectedBuff->GetClass();
	//TODO: BuffID
	PreviousStacks = InEvent.PreviousStacks;
	PreviousDuration = InEvent.PreviousDuration;
	NewStacks = InEvent.NewStacks;
	NewDuration = InEvent.NewDuration;
}

FLoggedBuffRemoveEvent::FLoggedBuffRemoveEvent(const FBuffRemoveEvent& InEvent, const ASaiyoraGameState* GameStateRef)
{
	if (!IsValid(GameStateRef) || !IsValid(GameStateRef->GetCombatLog()))
	{
		return;
	}
	EventID = GameStateRef->GetCombatLog()->GetNextCombatEventID();
	Timestamp = GameStateRef->GetServerWorldTimeSeconds();
	RemovedFrom = GameStateRef->GetCombatLog()->GetCombatantID(InEvent.RemovedFrom);
	AppliedBy = GameStateRef->GetCombatLog()->GetCombatantID(InEvent.AppliedBy);
	BuffClass = InEvent.RemovedBuff->GetClass();
	//TODO: BuffID
	ExpireReason = InEvent.ExpireReason;
}

#pragma region Combat Log

void FCombatLog::Clear()
{
	BuffApplications.Empty();
	BuffRemovals.Empty();
}

void FCombatLog::Append(const FCombatLog& NewEntries)
{
	enum ECombatLogEventType
	{
		None,
		BuffApply,
		BuffRemove,
	};
	//Cycle through all the events and add them in order until everything has been added.
	ECombatLogEventType NextEventType = None;
	int NextEventID = INT_MAX;
	int NextBuffApplyIdx = 0;
	int NextBuffRemoveIdx = 0;
	while (NewEntries.BuffApplications.IsValidIndex(NextBuffApplyIdx)
		|| NewEntries.BuffRemovals.IsValidIndex(NextBuffRemoveIdx)
		/*OR Check all the different indexes for IsValidIndex*/)
	{
		NextEventType = None;
		NextEventID = INT_MAX;
		
		//Check each event array for which has the next timestamp.
		if (NewEntries.BuffApplications.IsValidIndex(NextBuffApplyIdx))
		{
			if (NewEntries.BuffApplications[NextBuffApplyIdx].EventID < NextEventID)
			{
				NextEventID = NewEntries.BuffApplications[NextBuffApplyIdx].EventID;
				NextEventType = BuffApply;
			}
		}
		if (NewEntries.BuffRemovals.IsValidIndex(NextBuffRemoveIdx))
		{
			if (NewEntries.BuffRemovals[NextBuffRemoveIdx].EventID < NextEventID)
			{
				NextEventID = NewEntries.BuffRemovals[NextBuffRemoveIdx].EventID;
				NextEventType = BuffRemove;
			}
		}

		switch (NextEventType)
		{
		case BuffApply :
			LogBuffApply(NewEntries.BuffApplications[NextBuffApplyIdx]);
			++NextBuffApplyIdx;
			break;
		case BuffRemove :
			LogBuffRemove(NewEntries.BuffRemovals[NextBuffRemoveIdx]);
			++NextBuffRemoveIdx;
			break;
		default :
			break;
		}
	}
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

void FCombatLog::LogBuffApply(const FLoggedBuffApplyEvent& Event)
{
	BuffApplications.Add(Event);
	//TODO: Actually log to a file.
}

void FCombatLog::LogBuffRemove(const FLoggedBuffRemoveEvent& Event)
{
	BuffRemovals.Add(Event);
	//TODO: Actually log to a file.
}

#pragma endregion 