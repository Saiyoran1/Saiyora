#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UIFunctionLibrary.generated.h"

class UPlayerHUD;
enum class EHealthEventType : uint8;
enum class EElementalSchool : uint8;

UCLASS()
class SAIYORAV4_API UUIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

#pragma region Formatting Strings
	
public:

	//Converts seconds into a string formatted as mm:ss.
	UFUNCTION(BlueprintPure)
	static FString GetTimeDisplayString(const float Seconds);
	UFUNCTION(BlueprintPure)
	static FString GetShortKeybind(const FKey& Key);
	UFUNCTION(BlueprintPure)
	static FString GetInputChordString(const FInputChord& InputChord);

#pragma endregion 
#pragma region Colors

public:

	//Get the color associated with a specific elemental school.
	UFUNCTION(BlueprintPure)
	static FLinearColor GetElementalSchoolColor(const EElementalSchool School);
	//Get the outline color for floating health event text.
	UFUNCTION(BlueprintPure)
	static FLinearColor GetHealthEventOutlineColor(const EHealthEventType EventType);

#pragma endregion 
};
