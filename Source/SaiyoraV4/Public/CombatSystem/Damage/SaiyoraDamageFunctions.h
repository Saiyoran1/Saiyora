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
		float Amount,
		AActor* AppliedBy,
		AActor* AppliedTo,
		UObject* Source,
		EDamageHitStyle HitStyle,
		EDamageSchool School,
		bool IgnoreRestriction,
		bool IgnoreModifiers);

	//Apply healing to an actor.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	static FHealingEvent ApplyHealing(
		float Amount,
		AActor* AppliedBy,
		AActor* AppliedTo,
		UObject* Source,
		EDamageHitStyle HitStyle,
		EDamageSchool School,
		bool IgnoreRestrictions,
		bool IgnoreModifiers);
};
