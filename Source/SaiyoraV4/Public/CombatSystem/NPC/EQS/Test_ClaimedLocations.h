#pragma once
#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "Test_ClaimedLocations.generated.h"

//Returns a negative score for locations overlapping locations that NPCs besides the querier have already claimed as their move goal.
UCLASS()
class SAIYORAV4_API UTest_ClaimedLocations : public UEnvQueryTest
{
	GENERATED_BODY()

	UTest_ClaimedLocations();

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
};
