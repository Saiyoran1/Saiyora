#pragma once
#include "CoreMinimal.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "Generator_PathablePointsDonut.generated.h"

UCLASS()
class SAIYORAV4_API UEQG_PathablePointsDonut : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_BODY()

public:

	UEQG_PathablePointsDonut();
	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

private:

	UPROPERTY(EditDefaultsOnly, Category = Generator)
	FAIDataProviderFloatValue MinRange;
	UPROPERTY(EditDefaultsOnly, Category = Generator)
	FAIDataProviderFloatValue MaxRange;
	UPROPERTY(EditDefaultsOnly, Category = Generator)
	FAIDataProviderFloatValue DistanceBetweenPoints;

	UPROPERTY(EditDefaultsOnly, Category = Generator)
	TSubclassOf<UEnvQueryContext> CenterActor;
	
};
