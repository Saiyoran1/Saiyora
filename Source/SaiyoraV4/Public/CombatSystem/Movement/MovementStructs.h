#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "MovementEnums.h"
#include "MovementStructs.generated.h"

class UCombatAbility;
class USaiyoraMovementComponent;

USTRUCT()
struct FCustomMoveParams
{
	GENERATED_BODY()

	UPROPERTY()
	ESaiyoraCustomMove MoveType = ESaiyoraCustomMove::None;
	UPROPERTY()
	FVector Target = FVector::ZeroVector;
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY()
	bool bStopMovement = false;
	UPROPERTY()
	bool bIgnoreRestrictions = false;
};

USTRUCT()
struct FServerWaitingCustomMove
{
	GENERATED_BODY();
	
	FCustomMoveParams MoveParams;
	FTimerHandle TimerHandle;

	FServerWaitingCustomMove() {}
	FServerWaitingCustomMove(const FCustomMoveParams& Move) : MoveParams(Move) {}
};

USTRUCT()
struct FClientPendingCustomMove
{
	GENERATED_BODY()

	TSubclassOf<UCombatAbility> AbilityClass;
	int32 PredictionID = 0;
	float OriginalTimestamp = 0.0f;
	FCustomMoveParams MoveParams;
	FAbilityOrigin Origin;
	TArray<FAbilityTargetSet> Targets;
	

	void Clear() { AbilityClass = nullptr; PredictionID = 0; OriginalTimestamp = 0.0f; MoveParams = FCustomMoveParams(); Targets.Empty(); }
};

USTRUCT()
struct FServerMoveStatChange : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag StatTag;
	UPROPERTY()
	float Value = 0.0f;
	UPROPERTY()
	int32 ChangeID = 0;
	FTimerHandle ChangeHandle;

	FServerMoveStatChange() {}
	FServerMoveStatChange(const FGameplayTag InTag, const float InValue, const int32 InChangeID) : StatTag(InTag), Value(InValue), ChangeID(InChangeID) {}

	void PostReplicatedAdd(const struct FServerMoveStatArray& InArraySerializer);
	void PostReplicatedChange(const struct FServerMoveStatArray& InArraySerializer);
};

USTRUCT()
struct FServerMoveStatArray: public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FServerMoveStatChange> Items;
	UPROPERTY(NotReplicated)
	USaiyoraMovementComponent* OwningComponent = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FServerMoveStatChange, FServerMoveStatArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FServerMoveStatArray> : public TStructOpsTypeTraitsBase2<FServerMoveStatArray>
{
	enum 
	{
		WithNetDeltaSerializer = true,
   };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementChanged, AActor*, Actor, const bool, bNewMovement);