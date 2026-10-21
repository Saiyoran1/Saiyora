#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SaiyoraGameInstance.generated.h"

UCLASS()
class SAIYORAV4_API USaiyoraGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void Init() override;
	virtual void Shutdown() override;
};
