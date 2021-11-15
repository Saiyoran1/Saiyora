#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.h"
#include "ThreatStructs.h"
#include "DamageStructs.generated.h"

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

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDamageRestriction, FDamageInfo const&, DamageInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FDamageModCondition, FDamageInfo const&, DamageInfo);

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

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDeathRestriction, FDamagingEvent const&, DamageEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDamageEventCallback, FDamagingEvent const&, DamageEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamageEventNotification, FDamagingEvent const&, DamageEvent);

//Healing

/*USTRUCT(BlueprintType)
struct FHealingResult
{
 GENERATED_BODY()
 
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 bool Success = false;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 float AmountDealt = 0.0f;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 float PreviousHealth = 0.0f;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 float NewHealth = 0.0f;
};

USTRUCT(BlueprintType)
struct FHealingInfo
{
 GENERATED_BODY()

 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 AActor* AppliedTo = nullptr;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 AActor* AppliedBy = nullptr;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 UObject* Source = nullptr;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 bool AppliedXPlane = false;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::None;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::None;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 EDamageHitStyle HitStyle = EDamageHitStyle::None;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 EDamageSchool School = EDamageSchool::None;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 float Healing = 0.0f;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 float SnapshotHealing = 0.0f;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FHealingRestriction, FHealingInfo const&, HealingInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FHealingModCondition, FHealingInfo const&, HealingInfo);

USTRUCT(BlueprintType)
struct FHealingEvent
{
 GENERATED_BODY()

 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 FHealingInfo HealingInfo;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 FHealingResult Result;
 UPROPERTY(BlueprintReadOnly, Category = "Healing")
 FThreatFromDamage ThreatInfo;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FHealingEventCallback, FHealingEvent const&, HealingEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealingEventNotification, FHealingEvent const&, HealingEvent);*/

DECLARE_DYNAMIC_DELEGATE_TwoParams(FHealthChangeCallback, float, PreviousHealth, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthChangeNotification, float, PreviousHealth, float, NewHealth);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FLifeStatusCallback, AActor*, Actor, ELifeStatus, PreviousStatus, ELifeStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLifeStatusNotification, AActor*, Actor, ELifeStatus, PreviousStatus, ELifeStatus, NewStatus);