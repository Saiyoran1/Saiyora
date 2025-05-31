#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SaiyoraGameInstance.generated.h"

class UMapsDataAsset;
class USaiyoraUIDataAsset;
class UCombatDebugOptions;

UCLASS()
class SAIYORAV4_API USaiyoraGameInstance : public UGameInstance
{
	GENERATED_BODY()

#pragma region Data Assets

public:

	UPROPERTY(EditDefaultsOnly, Category = "Data Assets")
	USaiyoraUIDataAsset* UIDataAsset = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Data Assets")
	UMapsDataAsset* MapsDataAsset = nullptr;

#pragma endregion
#pragma region Debug

public:

	UPROPERTY(EditAnywhere, Category = "Debug")
	UCombatDebugOptions* CombatDebugOptions = nullptr;

#pragma endregion
};
