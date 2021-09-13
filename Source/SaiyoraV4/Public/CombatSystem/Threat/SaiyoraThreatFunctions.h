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
		bool bIgnoreRestrictions,
		bool bIgnoreModifiers,
		FThreatModCondition const& SourceModifier);
};
