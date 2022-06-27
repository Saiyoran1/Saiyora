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
    ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::None;
    UPROPERTY(BlueprintReadOnly)
    ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::None;
    UPROPERTY(BlueprintReadOnly)
    EEventHitStyle HitStyle = EEventHitStyle::None;
    UPROPERTY(BlueprintReadOnly)
    EHealthEventSchool School = EHealthEventSchool::None;
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

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FHealthEventRestriction, const FHealthEventInfo&, EventInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FHealthEventModCondition, const FHealthEventInfo&, EventInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDeathRestriction, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealthEventNotification, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthChangeNotification, AActor*, Actor, const float, PreviousHealth, const float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLifeStatusNotification, AActor*, Actor, const ELifeStatus, PreviousStatus, const ELifeStatus, NewStatus);