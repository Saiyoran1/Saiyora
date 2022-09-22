#pragma once
#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "CombatStructs.h"
#include "Resource.h"
#include "AbilityStructs.generated.h"

class UCombatAbility;

USTRUCT(BlueprintType)
struct FDefaultAbilityCost
{
    GENERATED_BODY();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSubclassOf<UResource> ResourceClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Cost = 0.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bStaticCost = false;
};

//This struct is used for prediction confirmation RPCs from the server, as well as normal replicated ability costs.
USTRUCT(BlueprintType)
struct FAbilityCost : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UResource> ResourceClass;
    UPROPERTY(BlueprintReadOnly)
    float Cost = 0.0f;

    FAbilityCost() {}
    FAbilityCost(const TSubclassOf<UResource> InResourceClass, const float InCost) : ResourceClass(InResourceClass), Cost(InCost) {}

    void PostReplicatedAdd(const struct FAbilityCostArray& InArraySerializer);
    void PostReplicatedChange(const struct FAbilityCostArray& InArraySerializer);

    FORCEINLINE bool operator==(const FAbilityCost& Other) const;
};

USTRUCT()
struct FAbilityCostArray: public FFastArraySerializer
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    TArray<FAbilityCost> Items;
    UPROPERTY(NotReplicated)
    UCombatAbility* OwningAbility = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FAbilityCost, FAbilityCostArray>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FAbilityCostArray> : public TStructOpsTypeTraitsBase2<FAbilityCostArray>
{
    enum 
    {
        WithNetDeltaSerializer = true,
   };
};

USTRUCT()
struct FAbilityCooldown
{
    GENERATED_BODY()

    UPROPERTY()
    int32 CurrentCharges = 0;
    UPROPERTY()
    bool OnCooldown = false;
    UPROPERTY(NotReplicated)
    bool bAcked = true;
    UPROPERTY()
    float CooldownStartTime = 0.0f;
    UPROPERTY()
    float CooldownEndTime = 0.0f;
    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY()
    int32 MaxCharges = 1;
    
};

USTRUCT(BlueprintType)
struct FAbilityOrigin
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FVector AimLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite)
    FVector AimDirection = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite)
    FVector Origin = FVector::ZeroVector;

    void Clear() { AimLocation = FVector::ZeroVector; AimDirection = FVector::ZeroVector; Origin = FVector::ZeroVector; }
    
    friend FArchive& operator<<(FArchive& Ar, FAbilityOrigin& Origin)
    {
        Ar << Origin.AimLocation;
        Ar << Origin.AimDirection;
        Ar << Origin.Origin;
        return Ar;
    }
};

USTRUCT(BlueprintType)
struct FAbilityTargetSet
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite)
    int32 SetID = 0;
    UPROPERTY(BlueprintReadWrite)
    TArray<AActor*> Targets;

    FAbilityTargetSet() {}
    FAbilityTargetSet(const int32 InSetID, const TArray<AActor*>& InTargets) : SetID(InSetID), Targets(InTargets) {}

    friend FArchive& operator<<(FArchive& Ar, FAbilityTargetSet& TargetSet)
    {
        Ar << TargetSet.SetID;
        Ar << TargetSet.Targets;
        return Ar;
    }
};

USTRUCT()
struct FAbilityParams
{
    GENERATED_BODY()

    FAbilityOrigin Origin;
    TArray<FAbilityTargetSet> Targets;

    FAbilityParams() {}
    FAbilityParams(const FAbilityOrigin& InOrigin, const TArray<FAbilityTargetSet>& InTargets) : Origin(InOrigin), Targets(InTargets) {}
};

USTRUCT(BlueprintType)
struct FAbilityEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ECastAction ActionTaken = ECastAction::Fail;
    UPROPERTY(BlueprintReadOnly)
    UCombatAbility* Ability = nullptr;
    UPROPERTY(BlueprintReadOnly)
    int32 Tick = 0;
    UPROPERTY(BlueprintReadOnly)
    ECastFailReason FailReason = ECastFailReason::None;
    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY(BlueprintReadOnly)
    FAbilityOrigin Origin;
    UPROPERTY(BlueprintReadOnly)
    TArray<FAbilityTargetSet> Targets;
};

USTRUCT(BlueprintType)
struct FCancelEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly)
    ECancelFailReason FailReason = ECancelFailReason::None;
    UPROPERTY(BlueprintReadOnly)
    UCombatAbility* CancelledAbility = nullptr;
    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY()
    int32 CancelledCastID = 0;
    UPROPERTY(BlueprintReadOnly)
    float CancelledCastStart = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float CancelledCastEnd = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float CancelTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    int32 ElapsedTicks = 0;
    UPROPERTY(BlueprintReadOnly)
    FAbilityOrigin Origin;
    UPROPERTY(BlueprintReadOnly)
    TArray<FAbilityTargetSet> Targets;
};

USTRUCT()
struct FCancelRequest
{
    GENERATED_BODY()

    UPROPERTY()
    int32 CancelID = 0;
    UPROPERTY()
    int32 CancelledCastID = 0;
    UPROPERTY()
    float CancelTime = 0.0f;
    UPROPERTY()
    FAbilityOrigin Origin;
    UPROPERTY()
    TArray<FAbilityTargetSet> Targets;
};

USTRUCT(BlueprintType)
struct FInterruptEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly)
    EInterruptFailReason FailReason = EInterruptFailReason::None;
    UPROPERTY(BlueprintReadOnly)
    AActor* InterruptAppliedTo = nullptr;
    UPROPERTY(BlueprintReadOnly)
    AActor* InterruptAppliedBy = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UCombatAbility* InterruptedAbility = nullptr;
    UPROPERTY()
    int32 CancelledCastID = 0;
    UPROPERTY(BlueprintReadOnly)
    UObject* InterruptSource = nullptr;
    UPROPERTY(BlueprintReadOnly)
    float InterruptedCastStart = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float InterruptedCastEnd = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float InterruptTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    int32 ElapsedTicks = 0;
};

USTRUCT(BlueprintType)
struct FGlobalCooldown
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bActive = false;
    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY(BlueprintReadOnly)
    float StartTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float Length = 0.0f;

    FORCEINLINE bool operator==(const FGlobalCooldown& Other) const { return Other.bActive == bActive && Other.StartTime == StartTime && Other.Length == Length; }
    FORCEINLINE bool operator!=(const FGlobalCooldown& Other) const { return Other.bActive != bActive || Other.StartTime != StartTime || Other.Length != Length; }
};

USTRUCT(BlueprintType)
struct FCastingState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bIsCasting = false;
    UPROPERTY(BlueprintReadOnly)
    UCombatAbility* CurrentCast = nullptr;
    UPROPERTY(NotReplicated)
    int32 PredictionID = 0;
    UPROPERTY(BlueprintReadOnly)
    float CastStartTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float CastLength = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    bool bInterruptible = true;
    UPROPERTY(BlueprintReadOnly, NotReplicated)
    int32 ElapsedTicks = 0;
};

USTRUCT()
struct FClientAbilityPrediction
{
    GENERATED_BODY()
    
    UPROPERTY()
    UCombatAbility* Ability = nullptr;
    bool bPredictedGCD = false;
    float GcdLength = 0.0f;
    float Time = 0.0f;
};

USTRUCT()
struct FPredictedTick
{
    GENERATED_BODY()

    int32 PredictionID = 0;
    int32 TickNumber = 0;

    FPredictedTick() {}
    FPredictedTick(const int32 ID, const int32 Tick) : PredictionID(ID), TickNumber(Tick) {}
    FORCEINLINE bool operator==(const FPredictedTick& Other) const { return Other.PredictionID == PredictionID && Other.TickNumber == TickNumber; }
};

FORCEINLINE uint32 GetTypeHash(const FPredictedTick& Tick)
{
    return HashCombine(GetTypeHash(Tick.PredictionID), GetTypeHash(Tick.TickNumber));
}

USTRUCT(BlueprintType)
struct FAutoReloadState
{
    GENERATED_BODY()

    UPROPERTY()
    bool bIsAutoReloading = false;
    UPROPERTY()
    float StartTime = 0.0f;
    UPROPERTY()
    float EndTime = 0.0f;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FAbilityModCondition, UCombatAbility*, Ability);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FAbilityClassRestriction, TSubclassOf<UCombatAbility>, AbilityClass);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FInterruptRestriction, const FInterruptEvent&, InterruptEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityInstanceNotification, UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityNotification, const FAbilityEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityCancelNotification, const FCancelEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterruptNotification, const FInterruptEvent&, InterruptEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityMispredictionNotification, const int32, PredictionID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGlobalCooldownNotification, const FGlobalCooldown&, OldGlobalCooldown, const FGlobalCooldown&, NewGlobalCooldown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCastingStateNotification, const FCastingState&, OldState, const FCastingState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityChargeNotification, UCombatAbility*, Ability, const int32, OldCharges, const int32, NewCharges);