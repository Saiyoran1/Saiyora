#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilityFunctionLibrary.generated.h"

UCLASS()
class SAIYORAV4_API UAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities", meta = (NativeMakeFunc))
	static FAbilityOrigin MakeAbilityOrigin(FVector const& AimLocation, FVector const& AimDirection, FVector const& Origin);
	
	UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (AutoCreateRefTerm = "ActorsToIgnore"))
    static bool PredictLineTrace(AActor* Shooter, float const TraceLength, TEnumAsByte<ETraceTypeQuery> const TraceChannel,
    	TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID, FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet);
};
