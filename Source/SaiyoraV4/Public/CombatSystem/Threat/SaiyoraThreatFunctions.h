// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "ThreatStructs.h"
#include "SaiyoraThreatFunctions.generated.h"

UCLASS()
class SAIYORAV4_API USaiyoraThreatFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	static FThreatEvent AddThreat(
		float const BaseThreat,
		AActor* AppliedBy,
		AActor* AppliedTo,
		UObject* Source,
		bool const bIgnoreRestrictions,
		bool const bIgnoreModifiers,
		FThreatModCondition const& SourceModifier);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat", meta = (NativeMakeFunc, AutoCreateRefTerm = "SourceModifier"))
	static FThreatFromDamage MakeThreatFromDamage(
		FThreatModCondition const& SourceModifier,
		bool const bGeneratesThreat = false,
		bool const bSeparateBaseThreat = false,
		float const BaseThreat = 0.0f,
		bool const bIgnoreModifiers = false,
		bool const bIgnoreRestrictions = false);
};
