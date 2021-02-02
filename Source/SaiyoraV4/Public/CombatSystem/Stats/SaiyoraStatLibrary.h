// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraStatLibrary.generated.h"

class UStatHandler;

UCLASS()
class SAIYORAV4_API USaiyoraStatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat")
	static UStatHandler* GetStatHandler(AActor* Target);

};
