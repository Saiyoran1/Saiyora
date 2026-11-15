#pragma once
#include "CoreMinimal.h"
#include "BuffEnums.h"
#include "CombatStructs.h"
#include "CombatLogStructs.generated.h"

class ASaiyoraGameState;
class UCombatLogComponent;
struct FBuffRemoveEvent;
struct FBuffApplyEvent;

USTRUCT()
struct FLoggedEventSource
{
	GENERATED_BODY()

	UPROPERTY()
	ECombatEventSourceType Type = ECombatEventSourceType::None;
	UPROPERTY()
	TSubclassOf<UObject> SourceClass = nullptr;

	FLoggedEventSource() {}
	FLoggedEventSource(const UObject* InSource);
};

USTRUCT()
struct FLoggedBuffApplyEvent
{
	GENERATED_BODY()

	UPROPERTY()
	int EventID = -1;
	UPROPERTY()
	float Timestamp = -1.0f;
	mutable bool bLogged = false;

	//The actor the buff was applied to.
	UPROPERTY()
	FCombatantID AppliedTo = FCombatantID();
	//The plane of the actor the buff was applied to, at application time.
	UPROPERTY()
	ESaiyoraPlane TargetPlane = ESaiyoraPlane::Both;

	//The actor the buff was applied by.
	UPROPERTY()
	FCombatantID AppliedBy = FCombatantID();
	//The plane of the actor the buff was applied by, at application time.
	UPROPERTY()
	ESaiyoraPlane OriginPlane = ESaiyoraPlane::Both;

	//The source of the buff, usually an ability, but can be anything
	UPROPERTY()
	FLoggedEventSource Source = FLoggedEventSource();

	//Whether this was a new application, stack, or refresh.
	UPROPERTY()
	EBuffApplyAction ActionTaken = EBuffApplyAction::Failed;
	//The buff class affected
	UPROPERTY()
	TSubclassOf<UBuff> BuffClass = nullptr;
	//The buff instance's stacks before this event
	UPROPERTY()
	int32 PreviousStacks = 0;
	//The buff instance's stacks after this event
	UPROPERTY()
	int32 NewStacks = 0;
	//The buff instance's remaining duration before this event
	UPROPERTY()
	float PreviousDuration = 0.0f;
	//The buff instance's remaining duration after this event
	UPROPERTY()
	float NewDuration = 0.0f;
	
	FLoggedBuffApplyEvent() {}
	FLoggedBuffApplyEvent(const FBuffApplyEvent& InEvent, const ASaiyoraGameState* GameStateRef);

	bool Write(const UCombatLogComponent* LogComponent, FString& OutLog) const;
};

USTRUCT()
struct FLoggedBuffRemoveEvent
{
	GENERATED_BODY()

	UPROPERTY()
	int EventID = -1;
	UPROPERTY()
	float Timestamp = -1.0f;
	mutable bool bLogged = false;
	
	//The buff class affected.
	UPROPERTY()
	TSubclassOf<UBuff> BuffClass = nullptr;
	//The actor that removed this buff
	UPROPERTY()
	FCombatantID AppliedBy = FCombatantID();
	//The actor the buff was applied to before being removed
	UPROPERTY()
	FCombatantID RemovedFrom = FCombatantID();
	//A generic reason for why the buff was removed.
	UPROPERTY()
	EBuffExpireReason ExpireReason = EBuffExpireReason::Invalid;

	FLoggedBuffRemoveEvent() {}
	FLoggedBuffRemoveEvent(const FBuffRemoveEvent& InEvent, const ASaiyoraGameState* GameStateRef);

	bool Write(const UCombatLogComponent* LogComponent, FString& OutLog) const;
};

USTRUCT()
struct FCombatLog
{
	GENERATED_BODY()

public:

	void Init(UCombatLogComponent* InCompRef);
	void Clear();
	void Append(const FCombatLog& NewEntries);
	bool HasData() const;

	void AddBuffApplyEvent(const FBuffApplyEvent& Event, ASaiyoraGameState* GameStateRef);
	void AddBuffRemoveEvent(const FBuffRemoveEvent& Event, ASaiyoraGameState* GameStateRef);

	void TryResolveEvents();

private:

	UPROPERTY()
	UCombatLogComponent* CompRef = nullptr;
	//This function exists so I can make a templated Write function without including the header for UCombatLogComponent.
	bool HasValidComponentRef() const;
	
	FLoggedBuffApplyEvent& AddEvent(const FLoggedBuffApplyEvent& Event) { return BuffApplications.Add_GetRef(Event); }
	UPROPERTY()
	TArray<FLoggedBuffApplyEvent> BuffApplications;
	FLoggedBuffRemoveEvent& AddEvent(const FLoggedBuffRemoveEvent& Event) { return BuffRemovals.Add_GetRef(Event); }
	UPROPERTY()
	TArray<FLoggedBuffRemoveEvent> BuffRemovals;

	template <typename T>
	void WriteEventToLog(const T& Event)
	{
		if (!HasValidComponentRef())
		{
			return;
		}
		FString LogOutput;
		if (!Event.Write(CompRef, LogOutput))
		{
			return;
		}
		//TODO: Write LogOutput to a file!
		Event.bLogged = true;
	}

	//Template function for iterating over all events (of all types) in event ID order.
	template <typename T>
	void ForEachCombatEvent(T&& Action) const
	{
		ECombatLogEventType NextEventType = ECombatLogEventType::None;
		int NextEventID = INT_MAX;
		int NextBuffApplyIdx = 0;
		int NextBuffRemoveIdx = 0;
		while (BuffApplications.IsValidIndex(NextBuffApplyIdx)
			|| BuffRemovals.IsValidIndex(NextBuffRemoveIdx)
			/*OR Check all the different indexes for IsValidIndex*/)
		{
			NextEventType = ECombatLogEventType::None;
			NextEventID = INT_MAX;
		
			//Check each event array for which has the next ID.
			if (BuffApplications.IsValidIndex(NextBuffApplyIdx))
			{
				if (BuffApplications[NextBuffApplyIdx].EventID < NextEventID)
				{
					NextEventID = BuffApplications[NextBuffApplyIdx].EventID;
					NextEventType = ECombatLogEventType::BuffApply;
				}
			}
			if (BuffRemovals.IsValidIndex(NextBuffRemoveIdx))
			{
				if (BuffRemovals[NextBuffRemoveIdx].EventID < NextEventID)
				{
					NextEventID = BuffRemovals[NextBuffRemoveIdx].EventID;
					NextEventType = ECombatLogEventType::BuffRemove;
				}
			}

			switch (NextEventType)
			{
			case ECombatLogEventType::BuffApply :
				Action(BuffApplications[NextBuffApplyIdx]);
				++NextBuffApplyIdx;
				break;
			case ECombatLogEventType::BuffRemove :
				Action(BuffRemovals[NextBuffRemoveIdx]);
				++NextBuffRemoveIdx;
				break;
			default :
				break;
			}
		}
	}
};