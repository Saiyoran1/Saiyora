#pragma once
#include "CoreMinimal.h"
#include "ChoiceRequirements.h"
#include "InstancedStruct.h"
#include "RotationBehaviors.generated.h"

enum class EChoiceRequirementCriteria : uint8;
//Struct for debug drawing rotation info.
USTRUCT()
struct FRotationDebugInfo
{
	GENERATED_BODY()
	
	FRotator OriginalRotation = FRotator::ZeroRotator;
	bool bClampingRotation = false;
	FRotator UnclampedRotation = FRotator::ZeroRotator;
	FRotator FinalRotation = FRotator::ZeroRotator;
};

//A struct intended to be inherited from for use as FInstancedStruct for NPCs.
//Describes a basic behavior for NPC rotation during combat.
USTRUCT()
struct FNPCRotationBehavior
{
	GENERATED_BODY()

	void Initialize(AActor* Actor);
	bool IsInitialized() const { return bInitialized; }
	
	virtual ~FNPCRotationBehavior() {}

	//Whether to enforce max rotation speed or just snap to the desired rotation.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bDeferRotation"))
	bool bEnforceRotationSpeed = true;
	//When enforcing a max rotation speed, this is the max degrees per second that the NPC can rotate.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bDeferRotation && bEnforceRotationSpeed"))
	float MaxRotationSpeed = 540.0f;
	//Whether to rotate only yaw, or to rotate on multiple axes.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bDeferRotation"))
	bool bOnlyYaw = true;

	//Virtual function for child classes to initialize any extra variables.
	virtual void InitializeBehavior(AActor* Actor) {}
	//Virtual function for child classes to actually perform rotation logic.
	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const {}

protected:

	//This property exists so that "meta-behaviors" that defer rotation to one of a few sub-behaviors can hide their base properties in the editor.
	UPROPERTY()
	bool bDeferRotation = false;

private:

	bool bInitialized = false;
};

//Causes NPCs to turn in the direction of their velocity.
USTRUCT()
struct FNPCRB_OrientToMovement : public FNPCRotationBehavior
{
	GENERATED_BODY()

	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const override;
};

//Causes NPCs to turn to face their target, using a target context.
USTRUCT()
struct FNPCRB_OrientToTarget : public FNPCRotationBehavior
{
	GENERATED_BODY()

	//Context for what target to face toward.
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCTargetContext", ExcludeBaseStruct))
	FInstancedStruct TargetContext;

	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const override;
};

//Causes NPCs to turn in the direction of their navmesh path.
USTRUCT()
struct FNPCRB_OrientToPath : public FNPCRotationBehavior
{
	GENERATED_BODY()
	
	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const override;
};

//Switches between two different rotation behaviors depending on whether a set of conditions are met or not.
USTRUCT()
struct FNPCRB_SwitchOnConditions : public FNPCRotationBehavior
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCRotationBehavior", ExcludeBaseStruct))
	FInstancedStruct ConditionsMetBehavior;
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCRotationBehavior", ExcludeBaseStruct))
	FInstancedStruct ConditionsUnmetBehavior;
	UPROPERTY(EditAnywhere)
	EChoiceRequirementCriteria Criteria = EChoiceRequirementCriteria::All;
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCChoiceRequirement", ExcludeBaseStruct))
	TArray<FInstancedStruct> Conditions;

	virtual void InitializeBehavior(AActor* Actor) override;
	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const override;

	//This hides the base properties of the struct in the editor, since we'll be using a sub-behavior's properties anyway.
	FNPCRB_SwitchOnConditions() { bDeferRotation = true; }

private:

	enum class ERotationSwitchValidity : uint8
	{
		BothValid,
		BothInvalid,
		ConditionsMetValid,
		ConditionsUnmetValid
	};

	ERotationSwitchValidity Validity = ERotationSwitchValidity::BothValid;
};
