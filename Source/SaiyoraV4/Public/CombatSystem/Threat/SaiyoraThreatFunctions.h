#pragma once
#include "CoreMinimal.h"
#include "ThreatStructs.h"
#include "SaiyoraThreatFunctions.generated.h"

UCLASS()
class SAIYORAV4_API USaiyoraThreatFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat", meta = (NativeMakeFunc, AutoCreateRefTerm = "SourceModifier"))
	static FThreatFromDamage MakeThreatFromDamage(
		FThreatModCondition const& SourceModifier,
		bool const bGeneratesThreat = false,
		bool const bSeparateBaseThreat = false,
		float const BaseThreat = 0.0f,
		bool const bIgnoreModifiers = false,
		bool const bIgnoreRestrictions = false);
};
