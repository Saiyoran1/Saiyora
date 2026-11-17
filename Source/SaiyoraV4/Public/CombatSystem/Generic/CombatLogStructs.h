#pragma once
#include "CoreMinimal.h"
#include "BuffEnums.h"
#include "CombatStructs.h"
#include "CombatLogStructs.generated.h"

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

	FString ToString() const;

	FLoggedEventSource() {}
	FLoggedEventSource(const UObject* InSource);
};

//Base struct for all the log event types, since all events need an ID, timestamp, name, and logged flag.
USTRUCT()
struct FCombatLogEvent
{
	GENERATED_BODY()
	
	UPROPERTY()
	int EventID = -1;
	UPROPERTY()
	float Timestamp = -1.0f;
	mutable bool bLogged = false;

	virtual const TCHAR* GetEventName() const { return TEXT("INVALID"); }
	virtual bool Write(const UCombatLogComponent* LogComponent, FString& OutLog) const
	{
		checkf(false, TEXT("Tried to write a base-class combat log event to the log."));
		return false;
	}

	FCombatLogEvent() {}
	FCombatLogEvent(const int InID, const float InTime) : EventID(InID), Timestamp(InTime) {}
	virtual ~FCombatLogEvent() {}
};

USTRUCT()
struct FLoggedBuffApplyEvent : public FCombatLogEvent
{
	GENERATED_BODY()

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
	FLoggedBuffApplyEvent(const FBuffApplyEvent& InEvent, const int InID, const float InTime);

	virtual bool Write(const UCombatLogComponent* LogComponent, FString& OutLog) const override;
	virtual const TCHAR* GetEventName() const override { return TEXT("BUFF_APPLY"); }
};

USTRUCT()
struct FLoggedBuffRemoveEvent : public FCombatLogEvent
{
	GENERATED_BODY()
	
	//The buff class affected.
	UPROPERTY()
	TSubclassOf<UBuff> BuffClass = nullptr;
	//The actor that originally applied this buff.
	UPROPERTY()
	FCombatantID AppliedBy = FCombatantID();
	//The actor the buff was applied to before being removed.
	UPROPERTY()
	FCombatantID RemovedFrom = FCombatantID();
	//A generic reason for why the buff was removed.
	UPROPERTY()
	EBuffExpireReason ExpireReason = EBuffExpireReason::Invalid;

	FLoggedBuffRemoveEvent() {}
	FLoggedBuffRemoveEvent(const FBuffRemoveEvent& InEvent, const int InID, const float InTime);

	virtual bool Write(const UCombatLogComponent* LogComponent, FString& OutLog) const override;
	virtual const TCHAR* GetEventName() const override { return TEXT("BUFF_REMOVE"); }
};

USTRUCT()
struct FCombatLogEventBatch
{
	GENERATED_BODY()

public:

	void Clear();
	bool HasData() const;
	
	FLoggedBuffApplyEvent& AddEvent(const FLoggedBuffApplyEvent& Event) { return BuffApplications.Add_GetRef(Event); }
	FLoggedBuffRemoveEvent& AddEvent(const FLoggedBuffRemoveEvent& Event) { return BuffRemovals.Add_GetRef(Event); }

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

private:
	
	UPROPERTY()
	TArray<FLoggedBuffApplyEvent> BuffApplications;
	UPROPERTY()
	TArray<FLoggedBuffRemoveEvent> BuffRemovals;
};