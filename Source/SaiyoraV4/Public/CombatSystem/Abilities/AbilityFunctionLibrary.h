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
	static FAbilityTarget MakeAbilityTarget(int32 const IDNumber, FVector const& Origin, FVector const& AimLocation, FVector const& AimDirection, AActor* HitTarget);
};
