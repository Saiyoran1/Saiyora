#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "CombatEnums.h"
#include "CombatStructs.h"
#include "ThreatStructs.h"
#include "DamageStructs.generated.h"

class UCombatComponent;

USTRUCT(BlueprintType)
struct FHealthEventResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool Success = false;
    UPROPERTY(BlueprintReadOnly)
    float AppliedValue = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float PreviousValue = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    float NewValue = 0.0f;
    UPROPERTY(BlueprintReadOnly)
    bool KillingBlow = false;
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
    UPROPERTY(BlueprintReadOnly)
    float SnapshotValue = 0.0f;
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
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FResDecisionCallback, const bool, bAccepted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPendingResNotification, const bool, bResAvailable, const FVector&, ResLocation);

USTRUCT(BlueprintType)
struct FPendingResurrection
{
    GENERATED_BODY()
    
    UPROPERTY(NotReplicated)
    UBuff* ResSource = nullptr;
    FResDecisionCallback DecisionCallback;
    
    UPROPERTY(BlueprintReadOnly)
    bool bResAvailable = false;
    UPROPERTY(BlueprintReadOnly)
    FVector ResLocation = FVector::ZeroVector;

    void Clear()
    {
        ResSource = nullptr;
        DecisionCallback.Unbind();
        bResAvailable = false;
        ResLocation = FVector::ZeroVector;
    }
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FHealthEventRestriction, const FHealthEventInfo&, EventInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FHealthEventModCondition, const FHealthEventInfo&, EventInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDeathRestriction, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FHealthEventCallback, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealthEventNotification, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthChangeNotification, AActor*, Actor, const float, PreviousHealth, const float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLifeStatusNotification, AActor*, Actor, const ELifeStatus, PreviousStatus, const ELifeStatus, NewStatus);
