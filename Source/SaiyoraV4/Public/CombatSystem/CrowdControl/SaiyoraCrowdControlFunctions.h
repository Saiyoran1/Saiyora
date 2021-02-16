// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraCrowdControlFunctions.generated.h"

class UCrowdControlHandler;

UCLASS()
class SAIYORAV4_API USaiyoraCrowdControlFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crowd Control")
	static UCrowdControlHandler* GetCrowdControlHandler(AActor* Target);
};
