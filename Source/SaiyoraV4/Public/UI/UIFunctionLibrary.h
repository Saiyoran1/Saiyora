﻿#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UIFunctionLibrary.generated.h"

class UPlayerHUD;
enum class EHealthEventType : uint8;
enum class EElementalSchool : uint8;
class USaiyoraUIDataAsset;


UCLASS()
class SAIYORAV4_API UUIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

#pragma region Setup

public:

	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static USaiyoraUIDataAsset* GetUIDataAsset(UObject* WorldContext);
	
private:
	
	static USaiyoraUIDataAsset* UIDataAsset;

#pragma endregion
#pragma region Widget Classes

public:
	
	static TSubclassOf<UPlayerHUD> GetPlayerHUDClass(UObject* WorldContext);
	
#pragma endregion 
#pragma region Formatting Strings
	
public:

	//Converts seconds into a string formatted as mm:ss.
	UFUNCTION(BlueprintPure)
	static FString GetTimeDisplayString(const float Seconds);

#pragma endregion 
#pragma region Colors

public:

	//Get the color associated with a specific elemental school.
	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static FLinearColor GetElementalSchoolColor(UObject* WorldContext, const EElementalSchool School);
	//Get the color for the player's health bar while alive.
	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static FLinearColor GetPlayerHealthColor(UObject* WorldContext);
	//Get the color for dead player health bars or party frames.
	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static FLinearColor GetPlayerDeadColor(UObject* WorldContext);
	//Get the outline color for floating health event text.
	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static FLinearColor GetHealthEventOutlineColor(UObject* WorldContext, const EHealthEventType EventType);
	//Gets the color for a cast bar that cannot be interrupted.
	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static FLinearColor GetUninterruptibleCastColor(UObject* WorldContext);
	//Gets the color for a cast bar that has been interrupted.
	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static FLinearColor GetInterruptedCastColor(UObject* WorldContext);
	//Gets the color for a cast bar that has been cancelled.
	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static FLinearColor GetCancelledCastColor(UObject* WorldContext);

#pragma endregion 
};
