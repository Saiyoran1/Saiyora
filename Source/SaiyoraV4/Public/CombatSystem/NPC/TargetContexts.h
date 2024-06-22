#pragma once
#include "CoreMinimal.h"
#include "NPCStructs.h"
#include "TargetContexts.generated.h"

//A context that simply returns the NPC's threat target, if one exists.
USTRUCT()
struct FNPCTC_HighestThreat : public FNPCTargetContext
{
	GENERATED_BODY()
	
	virtual AActor* GetBestTarget(const AActor* Querier) const override;
};

//A context that returns the closest threat target the NPC is in combat with.
USTRUCT()
struct FNPCTC_ClosestTarget : public FNPCTargetContext
{
	GENERATED_BODY()

	virtual AActor* GetBestTarget(const AActor* Querier) const override;
};
