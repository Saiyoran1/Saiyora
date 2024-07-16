#pragma once
#include "NPCStructs.h"
#include "ChoiceRequirements.generated.h"

class UThreatHandler;

#pragma region Choice Requirement

//Used for arrays of choice requirements to determine whether conditions being met requires all conditions, or only one condition.
UENUM()
enum class EChoiceRequirementCriteria : uint8
{
	Any,
	All
};

//A struct that is intended to be inherited from to be used as FInstancedStructs inside FNPCCombatChoice.
//Requirements that must be met for an NPC to choose a combat choice, rotation behavior, etc.
USTRUCT()
struct FNPCChoiceRequirement
{
	GENERATED_BODY()

public:
	
	void Init(AActor* NPC);
	virtual bool IsMet() const { return bIsMet; }
	
	virtual ~FNPCChoiceRequirement() {}

	FString DEBUG_GetReqName() const { return DEBUG_RequirementName; }

protected:
	
	AActor* GetOwningNPC() const { return OwningNPC; }

	FString DEBUG_RequirementName = "";

private:

	virtual void SetupRequirement() {}
	
	bool bIsMet = false;
	UPROPERTY()
	AActor* OwningNPC = nullptr;
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
#pragma region Rotation to Target

USTRUCT()
struct FNPCCR_RotationToTarget : public FNPCChoiceRequirement
{
	GENERATED_BODY()

	virtual bool IsMet() const override;

private:

	//Target to check rotation against.
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCTargetContext", ExcludeBaseStruct))
	FInstancedStruct TargetContext;
	//The angle the NPC must be within (or without).
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float AngleThreshold = 15.0f;
	//Whether the NPC should be facing within the angle threshold, or outside of the angle threshold.
	UPROPERTY(EditAnywhere)
	bool bWithinAngle = true;
	//Whether to check angle in 3 dimensions or only check yaw.
	UPROPERTY(EditAnywhere)
	bool bIncludeZAngle = false;
};