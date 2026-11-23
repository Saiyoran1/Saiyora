#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "CombatEnums.h"
#include "CombatStructs.h"
#include "ThreatStructs.h"
#include "DamageStructs.generated.h"

class UPendingResurrectionFunction;

USTRUCT(BlueprintType)
struct FHealthEventSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float Value = 0.0f;
    TArray<FModifierProxy> AppliedModifiers;

    FHealthEventSnapshot() {}
    FHealthEventSnapshot(const float InValue, const TArray<FModifierProxy>& InMods)
        : Value(InValue), AppliedModifiers(InMods) {}
};

USTRUCT(BlueprintType)
struct FHealthEventResult
{
    GENERATED_BODY()

    //Whether the health event was successfully applied.
    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;
    //All of the reasons we couldn't apply this health event.
    UPROPERTY(BlueprintReadOnly)
    TArray<EHealthEventFailReason> FailReasons;
    
    //If an incoming health event restriction prevented application, this is a proxy for the restriction.
    FCombatDelegateProxy IncomingRestriction;
    //If an outgoing health event restriction prevented application, this is a proxy for the restriction.
    FCombatDelegateProxy OutgoingRestriction;
    
    //All incoming health event modifiers applied to the event, as proxy structs.
    TArray<FModifierProxy> IncomingModifiers;
    //Snapshot value of this health event for use in future ApplySnapshottedHealthEvent calls.
    //Includes the list of outgoing modifiers originally applied to this value.
    UPROPERTY(BlueprintReadOnly)
    FHealthEventSnapshot Snapshot;
    
    UPROPERTY(BlueprintReadOnly)
    float AppliedValue = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float PreviousValue = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float NewValue = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    bool bKillingBlow = false;

#if !UE_BUILD_SHIPPING
    void DEBUG_ToString(FString& OutString) const;
#endif
};

USTRUCT(BlueprintType)
struct FHealthEventInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EHealthEventType EventType = EHealthEventType::None;
    UPROPERTY(BlueprintReadOnly)
    AActor* AppliedTo = nullptr;
    UPROPERTY(BlueprintReadOnly)
    AActor* AppliedBy = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UObject* Source = nullptr;
    UPROPERTY(BlueprintReadOnly)
    bool AppliedXPlane = false;
    UPROPERTY(BlueprintReadOnly)
    ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::Both;
    UPROPERTY(BlueprintReadOnly)
    ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::Both;
    UPROPERTY(BlueprintReadOnly)
    EEventHitStyle HitStyle = EEventHitStyle::None;
    UPROPERTY(BlueprintReadOnly)
    EElementalSchool School = EElementalSchool::None;
    UPROPERTY(BlueprintReadOnly)
    float Value = 0.0f;
    //Unique ID for the combat event.
    UPROPERTY()
    int EventID = -1;

#if !UE_BUILD_SHIPPING
    void DEBUG_ToString(FString& OutString) const;
#endif
};

USTRUCT(BlueprintType)
struct FHealthEvent
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    FHealthEventInfo Info;
    UPROPERTY(BlueprintReadOnly)
    FHealthEventResult Result;
    UPROPERTY(BlueprintReadOnly)
    FThreatFromDamage ThreatInfo;

#if !UE_BUILD_SHIPPING
    void DEBUG_ToString(FString& OutString) const;
#endif
};

//A struct representing a pending resurrection.
//Basically encapsulates UPendingResurrectionFunction's data in a way that is replication-friendly and allows identification via ID over the network.
USTRUCT()
struct FPendingResurrection
{
    GENERATED_BODY()
    
    UPROPERTY(NotReplicated)
    UPendingResurrectionFunction* Source = nullptr;
    //Replicate the buff class for display info (icon, name, etc.).
    UPROPERTY()
    TSubclassOf<UBuff> BuffClass;
    
    UPROPERTY()
    FVector ResLocation = FVector::ZeroVector;
    UPROPERTY()
    bool bOverrideHealth = false;
    UPROPERTY()
    float OverrideHealthPercent = 1.0f;
    
    UPROPERTY()
    int ResID = -1;

    FPendingResurrection() {}
    FPendingResurrection(UPendingResurrectionFunction* InResFunction);

    FORCEINLINE bool operator==(const FPendingResurrection& Other) const { return Other.ResID == ResID; }
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FHealthEventRestriction, const FHealthEventInfo&, EventInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FHealthEventModCondition, const FHealthEventInfo&, EventInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDeathRestriction, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FHealthEventCallback, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealthEventNotification, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthChangeNotification, AActor*, Actor, const float, PreviousHealth, const float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLifeStatusNotification, AActor*, Actor, const ELifeStatus, PreviousStatus, const ELifeStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPendingResNotification, const TArray<FPendingResurrection>&, NewResurrections);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FResurrectionCallback, const bool, bAccepted, const FVector&, ResLocation);