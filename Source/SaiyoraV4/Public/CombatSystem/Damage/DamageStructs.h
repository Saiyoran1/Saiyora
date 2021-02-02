// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "DamageEnums.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.h"
#include "DamageStructs.generated.h"

//Damage

USTRUCT(BlueprintType)
struct FDamageResult
{
 GENERATED_BODY()

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 bool Success = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 float AmountDealt = 0.0f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 float PreviousHealth = 0.0f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 float NewHealth = 0.0f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 bool KillingBlow = false;
};

USTRUCT(BlueprintType)
struct FDamageInfo
{
 GENERATED_BODY()

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 AActor* AppliedTo = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 AActor* AppliedBy = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 UObject* Source = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 bool AppliedXPlane = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 EDamageHitStyle HitStyle = EDamageHitStyle::None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 EDamageSchool School = EDamageSchool::None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 float Damage = 0.0f;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDamageCondition, FDamageInfo const&, DamageInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FDamageModCondition, FDamageInfo const&, DamageInfo);

USTRUCT(BlueprintType)
struct FDamagingEvent
{
 GENERATED_BODY()
 
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 FDamageInfo DamageInfo;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
 FDamageResult Result;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FDeathCondition, FDamagingEvent const&, DamageEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDamageEventCallback, FDamagingEvent const&, DamageEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamageEventNotification, FDamagingEvent const&, DamageEvent);

//Healing

USTRUCT(BlueprintType)
struct FHealingResult
{
 GENERATED_BODY()
 
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 bool Success = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 float AmountDealt = 0.0f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 float PreviousHealth = 0.0f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 float NewHealth = 0.0f;
};

USTRUCT(BlueprintType)
struct FHealingInfo
{
 GENERATED_BODY()

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 AActor* AppliedTo = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 AActor* AppliedBy = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 UObject* Source = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 bool AppliedXPlane = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 EDamageHitStyle HitStyle = EDamageHitStyle::None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 EDamageSchool School = EDamageSchool::None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 float Healing = 0.0f;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FHealingCondition, FHealingInfo const&, HealingInfo);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FHealingModCondition, FHealingInfo const&, HealingInfo);

USTRUCT(BlueprintType)
struct FHealingEvent
{
 GENERATED_BODY()

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 FHealingInfo HealingInfo;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Healing")
 FHealingResult Result;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FHealingEventCallback, FHealingEvent const&, HealingEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealingEventNotification, FHealingEvent const&, HealingEvent);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FHealthChangeCallback, float, PreviousHealth, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthChangeNotification, float, PreviousHealth, float, NewHealth);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FLifeStatusCallback, ELifeStatus, PreviousStatus, ELifeStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLifeStatusNotification, ELifeStatus, PreviousStatus, ELifeStatus, NewStatus);