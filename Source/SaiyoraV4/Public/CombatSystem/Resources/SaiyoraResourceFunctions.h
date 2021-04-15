// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ResourceHandler.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraResourceFunctions.generated.h"

UCLASS()
class SAIYORAV4_API USaiyoraResourceFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Resources", meta = (GameplayTagFilter = "Resource"))
	static void ModifyResource(AActor* Target, FGameplayTag const& ResourceTag, float const Amount, UObject* Source, bool const bIgnoreModifiers);
};
