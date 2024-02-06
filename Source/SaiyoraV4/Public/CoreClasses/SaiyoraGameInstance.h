#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SaiyoraGameInstance.generated.h"

class UCombatDebugOptions;

UCLASS()
class SAIYORAV4_API USaiyoraGameInstance : public UGameInstance
{
	GENERATED_BODY()

#pragma region Debug

public:

	UPROPERTY(EditAnywhere, Category = "Debug")
	UCombatDebugOptions* CombatDebugOptions;

#pragma endregion 
};
