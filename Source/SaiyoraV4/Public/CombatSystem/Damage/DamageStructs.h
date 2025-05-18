#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "CombatEnums.h"
#include "CombatStructs.h"
#include "ThreatStructs.h"
#include "DamageStructs.generated.h"

class UPendingResurrectionFunction;
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

//A struct representing a pending resurrection.
//Basically encapsulates UPendingResurrectionFunction's data in a way that is replication-friendly and allows identification via ID over the network.
USTRUCT()
struct FPendingResurrection
{
    GENERATED_BODY()
    
    UPROPERTY(NotReplicated)
    UBuff* BuffSource = nullptr;
    UPROPERTY(NotReplicated)
    FName CallbackName;
    //Replicate the display icon because replicating the buff source itself introduces a race condition.
    //In the future it might be smart to just replicate the buff class so we can extract school/plane info if needed too.
    UPROPERTY()
    UTexture2D* Icon = nullptr;
    
    UPROPERTY()
    FVector ResLocation = FVector::ZeroVector;
    UPROPERTY()
    bool bOverrideHealth = false;
    UPROPERTY()
    float OverrideHealthPercent = 1.0f;
    
    UPROPERTY()
    int ResID = -1;

    FPendingResurrection() {}
    FPendingResurrection(const UPendingResurrectionFunction* InResFunction);

    FORCEINLINE bool operator==(const FPendingResurrection& Other) const { return Other.ResID == ResID; }
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FHealthEventRestriction, const FHealthEventInfo&, EventInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FHealthEventModCondition, const FHealthEventInfo&, EventInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDeathRestriction, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FHealthEventCallback, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealthEventNotification, const FHealthEvent&, HealthEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthChangeNotification, AActor*, Actor, const float, PreviousHealth, const float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLifeStatusNotification, AActor*, Actor, const ELifeStatus, PreviousStatus, const ELifeStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPendingResNotification, const FPendingResurrection&, PendingResurrection);