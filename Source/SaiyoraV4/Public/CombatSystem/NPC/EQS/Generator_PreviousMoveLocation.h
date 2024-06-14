#pragma once
#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "Generator_PreviousMoveLocation.generated.h"

UCLASS()
class SAIYORAV4_API UEQG_PreviousMoveLocation : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:

	UEQG_PreviousMoveLocation();
	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
};
