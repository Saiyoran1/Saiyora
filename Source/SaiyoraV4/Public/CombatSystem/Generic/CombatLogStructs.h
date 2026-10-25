#pragma once
#include "CoreMinimal.h"
#include "BuffEnums.h"
#include "CombatStructs.h"
#include "CombatLogStructs.generated.h"

struct FBuffRemoveEvent;
struct FBuffApplyEvent;

USTRUCT()
struct FLoggedBuffApplyEvent
{
	GENERATED_BODY()

	UPROPERTY()
	int EventID = -1;
	UPROPERTY()
	float Timestamp = -1.0f;

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
	UObject* Source = nullptr;

	//Whether this was a new application, stack, or refresh.
	UPROPERTY()
	EBuffApplyAction ActionTaken = EBuffApplyAction::Failed;
	//Unique ID of the affected buff.
	UPROPERTY()
	int32 BuffID = -1;
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
};

USTRUCT()
struct FLoggedBuffRemoveEvent
{
	GENERATED_BODY()

	UPROPERTY()
	int EventID = -1;
	UPROPERTY()
	float Timestamp = -1.0f;

	//Unique ID of the removed buff.
	UPROPERTY()
	int32 BuffID = -1;
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
};

USTRUCT()
struct FCombatLog
{
	GENERATED_BODY()

public:

	friend class ASaiyoraGameState;

	void Clear();
	void Append(const FCombatLog& NewEntries);
	bool HasData() const;

private:

	void LogBuffApply(const FLoggedBuffApplyEvent& Event);
	UPROPERTY()
	TArray<FLoggedBuffApplyEvent> BuffApplications;
	void LogBuffRemove(const FLoggedBuffRemoveEvent& Event);
	UPROPERTY()
	TArray<FLoggedBuffRemoveEvent> BuffRemovals;
};