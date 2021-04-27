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
	static float GetActorPing(AActor const* Actor);
	
	//Plane
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	static ESaiyoraPlane GetActorPlane(AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	static bool CheckForXPlane(ESaiyoraPlane const FromPlane, ESaiyoraPlane const ToPlane);
};


