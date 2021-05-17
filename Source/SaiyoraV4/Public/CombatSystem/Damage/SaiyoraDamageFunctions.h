// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraDamageFunctions.generated.h"

class UDamageHandler;

UCLASS()
class SAIYORAV4_API USaiyoraDamageFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//Apply damage to an actor.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	static FDamagingEvent ApplyDamage(
		float const Amount,
		AActor* AppliedBy,
		AActor* AppliedTo,
		UObject* Source,
		EDamageHitStyle const HitStyle,
		EDamageSchool const School,
		bool const bIgnoreRestrictions,
		bool const bIgnoreModifiers,
		bool const bFromSnapshot);

	//Apply healing to an actor.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	static FHealingEvent ApplyHealing(
		float const Amount,
		AActor* AppliedBy,
		AActor* AppliedTo,
		UObject* Source,
		EDamageHitStyle const HitStyle,
		EDamageSchool const School,
		bool const bIgnoreRestrictions,
		bool const bIgnoreModifiers,
		bool const bFromSnapshot);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Damage")
	static float GetSnapshotDamage(
		float const Amount,
		AActor* AppliedBy,
		AActor* AppliedTo,
		UObject* Source,
		EDamageHitStyle const HitStyle,
		EDamageSchool const School,
		bool const bIgnoreModifiers);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Healing")
	static float GetSnapshotHealing(
		float const Amount,
		AActor* AppliedBy,
		AActor* AppliedTo,
		UObject* Source,
		EDamageHitStyle const HitStyle,
		EDamageSchool const School,
		bool const bIgnoreModifiers);
};
