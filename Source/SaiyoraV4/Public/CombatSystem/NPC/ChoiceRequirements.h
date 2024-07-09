#pragma once
#include "NPCStructs.h"
#include "ChoiceRequirements.generated.h"

class UThreatHandler;

#pragma region Choice Requirement

//A struct that is intended to be inherited from to be used as FInstancedStructs inside FNPCCombatChoice.
//These requirements are event-based, and must be set up in c++. They basically constantly update their owning choice with whether they are met.
USTRUCT()
struct FNPCChoiceRequirement
{
	GENERATED_BODY()

public:
	
	void Init(FNPCCombatChoice* Choice, AActor* NPC);
	virtual bool IsMet() const { return false; }
	
	virtual ~FNPCChoiceRequirement() {}

	FString DEBUG_GetReqName() const { return DEBUG_RequirementName; }

protected:
	
	AActor* GetOwningNPC() const { return OwningNPC; }
	FNPCCombatChoice GetOwningChoice() const { return *OwningChoice; }

	FString DEBUG_RequirementName = "";

private:

	virtual void SetupRequirement() {}
	
	bool bIsMet = false;
	UPROPERTY()
	AActor* OwningNPC = nullptr;
	FNPCCombatChoice* OwningChoice = nullptr;
};

#pragma endregion 
#pragma region Time In Combat

USTRUCT()
struct FNPCCR_TimeInCombat : public FNPCChoiceRequirement
{
	GENERATED_BODY()

	virtual bool IsMet() const override;
	virtual void SetupRequirement() override;

private:

	//How much time must have passed for this requirement to be met (or cease to be met).
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float TimeRequirement = 1.0f;
	//If this requirement should be met after the allotted time has elapsed, or before.
	UPROPERTY(EditAnywhere)
	bool bMetAfterTimeReached = true;

	UPROPERTY()
	UThreatHandler* ThreatHandler =  nullptr;
};

#pragma endregion
#pragma region Range Of Target

USTRUCT()
struct FNPCCR_RangeOfTarget : public FNPCChoiceRequirement
{
	GENERATED_BODY()

	virtual bool IsMet() const override;

private:

	//Context for what target we want to be in range of.
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCTargetContext", ExcludeBaseStruct))
	FInstancedStruct TargetContext;
	//The range to check against.
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float Range = 100.0f;
	//Whether to check that the NPC is within the given range or outside of the given range.
	UPROPERTY(EditAnywhere)
	bool bWithinRange = true;
	//Whether to check 3D distance or 2D distance.
	UPROPERTY(EditAnywhere)
	bool bIncludeZDistance = true;
};

#pragma endregion 