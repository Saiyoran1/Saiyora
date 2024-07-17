#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SaiyoraGameInstance.generated.h"

class USaiyoraUIDataAsset;
class UCombatDebugOptions;

UCLASS()
class SAIYORAV4_API USaiyoraGameInstance : public UGameInstance
{
	GENERATED_BODY()

#pragma region Debug

public:

	UPROPERTY(EditAnywhere, Category = "Debug")
	UCombatDebugOptions* CombatDebugOptions = nullptr;

#pragma endregion
#pragma region Data Assets

public:

	UPROPERTY(EditAnywhere, Category = "UI")
	USaiyoraUIDataAsset* UIDataAsset = nullptr;

#pragma endregion 
};
