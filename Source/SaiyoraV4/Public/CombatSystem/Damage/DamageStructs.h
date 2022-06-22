#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "CombatEnums.h"
#include "CombatStructs.h"
#include "ThreatStructs.h"
#include "DamageStructs.generated.h"

class UCombatComponent;

USTRUCT(BlueprintType)
struct FDamageResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    bool Success = false;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float AmountDealt = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float PreviousHealth = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float NewHealth = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    bool KillingBlow = false;
};

USTRUCT(BlueprintType)
struct FDamageInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    AActor* AppliedTo = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    AActor* AppliedBy = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    UObject* Source = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    bool AppliedXPlane = false;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::None;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::None;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    EDamageHitStyle HitStyle = EDamageHitStyle::None;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    EDamageSchool School = EDamageSchool::None;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float Value = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float SnapshotValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FDamagingEvent
{
    GENERATED_BODY()
 
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    FDamageInfo Info;
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    FDamageResult Result;
    UPROPERTY(BlueprintReadOnly, Category = "Threat")
    FThreatFromDamage ThreatInfo;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDamageRestriction, const FDamageInfo&, DamageInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FDamageModCondition, const FDamageInfo&, DamageInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDeathRestriction, const FDamagingEvent&, DamageEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDamageEventCallback, FDamagingEvent const&, DamageEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamageEventNotification, const FDamagingEvent&, DamageEvent);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FHealthChangeCallback, AActor*, Actor, const float, PreviousHealth, const float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthChangeNotification, AActor*, Actor, const float, PreviousHealth, const float, NewHealth);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FLifeStatusCallback, AActor*, Actor, const ELifeStatus, PreviousStatus, const ELifeStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLifeStatusNotification, AActor*, Actor, const ELifeStatus, PreviousStatus, const ELifeStatus, NewStatus);