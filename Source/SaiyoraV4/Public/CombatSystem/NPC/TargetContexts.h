#pragma once
#include "CoreMinimal.h"
#include "TargetContexts.generated.h"

//A struct that is intended to be inherited from to be used as FInstancedStructs for NPCs to use.
//Indicates how to determine the "target" of a given NPC action, such as looking at a target or checking distance to a target.
USTRUCT()
struct FNPCTargetContext
{
	GENERATED_BODY()

	virtual AActor* GetBestTarget(const AActor* Querier) const { return nullptr; }
	virtual ~FNPCTargetContext() {}
};

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
