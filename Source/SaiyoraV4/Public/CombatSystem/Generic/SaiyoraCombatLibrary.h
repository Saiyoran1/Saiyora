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

	//Net

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Time")
	static float GetActorPing(AActor const* Actor);
	static EActorNetPermission GetActorNetPermission(AActor const* Actor);
	
	//Plane
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	static ESaiyoraPlane GetActorPlane(AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	static bool CheckForXPlane(ESaiyoraPlane const FromPlane, ESaiyoraPlane const ToPlane);

	//Modifier

	UFUNCTION(BlueprintCallable, Category = "Modifier", meta = (DefaultToSelf = "Source", HidePin = "Source", NativeMakeFunc))
	static FCombatModifier MakeCombatModifier(class UBuff* Source, enum EModifierType const ModifierType, float const ModifierValue, bool const bStackable);
	UFUNCTION(BlueprintCallable, Category = "Modifier", meta = (DefaultToSelf = "Source", HidePin = "Source", NativeMakeFunc))
	static FCombatModifier MakeBuffFunctionCombatModifier(class UBuffFunction* Source, enum EModifierType const ModifierType, float const ModifierValue, bool const bStackable);
};


