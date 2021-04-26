// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "SaiyoraStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraBuffLibrary.generated.h"

class UBuffHandler;

UCLASS()
class SAIYORAV4_API USaiyoraBuffLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//Attempt to apply a buff to an actor.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs", meta = (AutoCreateRefTerm = "EventParams"))
	static FBuffApplyEvent ApplyBuff(
		TSubclassOf<UBuff> const BuffClass,
		AActor* const AppliedBy,
		AActor* const AppliedTo,
		UObject* const Source,
		bool const DuplicateOverride,
		EBuffApplicationOverrideType const StackOverrideType,
		int32 const OverrideStacks,
		EBuffApplicationOverrideType const RefreshOverrideType,
		float const OverrideDuration,
		bool const IgnoreRestrictions,
		TArray<FCombatParameter> const& BuffParams);

	//Attempt to remove a buff from an actor.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
	static FBuffRemoveEvent RemoveBuff(UBuff* Buff, EBuffExpireReason const ExpireReason);
};
