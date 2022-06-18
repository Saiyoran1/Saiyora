#pragma once
#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "SaiyoraStructs.h"
#include "Resource.h"
#include "CoreUObject/Public/Templates/SubclassOf.h"
#include "AbilityStructs.generated.h"

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

    FAbilityCost();
    FAbilityCost(TSubclassOf<UResource> const NewResourceClass, float const NewCost);

    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<UResource> ResourceClass;
    UPROPERTY(BlueprintReadOnly)
    float Cost = 0.0f;

    void PostReplicatedAdd(const struct FAbilityCostArray& InArraySerializer);
    void PostReplicatedChange(const struct FAbilityCostArray& InArraySerializer);

    FORCEINLINE bool operator==(FAbilityCost const& Other) const { return Other.ResourceClass == ResourceClass && Other.Cost == Cost; }
};

USTRUCT()
struct FAbilityCostArray: public FFastArraySerializer
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    TArray<FAbilityCost> Items;
    UPROPERTY(NotReplicated)
    class UCombatAbility* OwningAbility = nullptr;

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
    FVector_NetQuantize100 AimLocation;
    UPROPERTY(BlueprintReadWrite)
    FVector_NetQuantizeNormal AimDirection;
    UPROPERTY(BlueprintReadWrite)
    FVector_NetQuantize100 Origin;

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

    FAbilityTargetSet();
    FAbilityTargetSet(int32 const SetID, TArray<AActor*> const& Targets);

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

    FAbilityParams();
    FAbilityParams(FAbilityOrigin const& InOrigin, TArray<FAbilityTargetSet> const& InTargets);
};

USTRUCT(BlueprintType)
struct FAbilityEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ECastAction ActionTaken = ECastAction::Fail;
    UPROPERTY(BlueprintReadOnly)
    class UCombatAbility* Ability = nullptr;
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
    class UCombatAbility* CancelledAbility = nullptr;
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
    class UCombatAbility* InterruptedAbility = nullptr;
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
    bool bGlobalCooldownActive = false;
    UPROPERTY()
    int32 PredictionID = 0;
    UPROPERTY(BlueprintReadOnly)
    float StartTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float EndTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FCastingState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bIsCasting = false;
    UPROPERTY(BlueprintReadOnly)
    class UCombatAbility* CurrentCast = nullptr;
    UPROPERTY(NotReplicated)
    int32 PredictionID = 0;
    UPROPERTY(BlueprintReadOnly)
    float CastStartTime = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float CastEndTime = 0.0f;
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
    class UCombatAbility* Ability = nullptr;
    bool bPredictedGCD = false;
    bool bPredictedCastBar = false;
};

USTRUCT()
struct FPredictedTick
{
    GENERATED_BODY()

    int32 PredictionID = 0;
    int32 TickNumber = 0;

    FPredictedTick();
    FPredictedTick(int32 const ID, int32 const Tick) { PredictionID = ID; TickNumber = Tick; }
    FORCEINLINE bool operator==(const FPredictedTick& Other) const { return Other.PredictionID == PredictionID && Other.TickNumber == TickNumber; }
};

FORCEINLINE uint32 GetTypeHash(const FPredictedTick& Tick)
{
    return HashCombine(GetTypeHash(Tick.PredictionID), GetTypeHash(Tick.TickNumber));
}

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FAbilityModCondition, class UCombatAbility*, Ability);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FAbilityClassRestriction, TSubclassOf<class UCombatAbility>, AbilityClass);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FInterruptRestriction, FInterruptEvent const&, InterruptEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityInstanceCallback, class UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityCallback, FAbilityEvent const&, Event);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityCancelCallback, FCancelEvent const&, Event);
DECLARE_DYNAMIC_DELEGATE_OneParam(FInterruptCallback, FInterruptEvent const&, InterruptEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityMispredictionCallback, int32 const, PredictionID);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FGlobalCooldownCallback, FGlobalCooldown const&, OldGlobalCooldown, FGlobalCooldown const&, NewGlobalCooldown);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FCastingStateCallback, FCastingState const&, OldState, FCastingState const&, NewState);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FAbilityChargeCallback, class UCombatAbility*, Ability, int32 const, OldCharges, int32 const, NewCharges);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityInstanceNotification, class UCombatAbility*, NewAbility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityNotification, FAbilityEvent const&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityCancelNotification, FCancelEvent const&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterruptNotification, FInterruptEvent const&, InterruptEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityMispredictionNotification, int32 const, PredictionID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGlobalCooldownNotification, FGlobalCooldown const&, OldGlobalCooldown, FGlobalCooldown const&, NewGlobalCooldown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCastingStateNotification, FCastingState const&, OldState, FCastingState const&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityChargeNotification, class UCombatAbility*, Ability, int32 const, OldCharges, int32 const, NewCharges);