// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraCombatLibrary.generated.h"

UCLASS()
class SAIYORAV4_API USaiyoraCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	//Time

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Time")
	static float GetWorldTime(UObject const* Context);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Time")
	static float GetActorPing(AActor const* Actor);
	
	//Plane
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	static ESaiyoraPlane GetActorPlane(AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	static bool CheckForXPlane(ESaiyoraPlane const FromPlane, ESaiyoraPlane const ToPlane);

	//Params
	UFUNCTION(BlueprintCallable, Category = "Params", meta = (AutoCreateRefTerm = "OriginLocation, OriginRotation, OriginScale, TargetLocation, TargetRotation, TargetScale, Object", AdvancedDisplay = 1))
	static void ExtractCombatParameters(UPARAM(ref) FCombatParameters const& Parameters,
		UPARAM(ref) FVector& OriginLocation,
		UPARAM(ref) FVector& OriginRotation,
		UPARAM(ref) FVector& OriginScale,
		UPARAM(ref) FVector& TargetLocation,
		UPARAM(ref) FVector& TargetRotation,
		UPARAM(ref) FVector& TargetScale,
		UPARAM(ref) UObject*& Object);
};


