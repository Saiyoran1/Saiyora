#pragma once
#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "RotationBehaviors.generated.h"

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

	void Initialize(const AActor* Actor);
	bool IsInitialized() const { return bInitialized; }
	
	virtual ~FNPCRotationBehavior() {}

	//Whether to enforce max rotation speed or just snap to the desired rotation.
	UPROPERTY(EditAnywhere)
	bool bEnforceRotationSpeed = true;
	//When enforcing a max rotation speed, this is the max degrees per second that the NPC can rotate.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bEnforceRotationSpeed"))
	float MaxRotationSpeed = 540.0f;
	//Whether to rotate only yaw, or to rotate on multiple axes.
	UPROPERTY(EditAnywhere)
	bool bOnlyYaw = true;

	//Virtual function for child classes to initialize any extra variables.
	virtual void InitializeBehavior(const AActor* Actor) {}
	//Virtual function for child classes to actually perform rotation logic.
	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const {}

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

//A rotation behavior that chooses between two other rotation behaviors based on distance to a target.
USTRUCT()
struct FNPCRB_SwitchOnDistance : public FNPCRotationBehavior
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCRotationBehavior", ExcludeBaseStruct))
	FInstancedStruct InRangeBehavior;
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCRotationBehavior", ExcludeBaseStruct))
	FInstancedStruct OutOfRangeBehavior;
	UPROPERTY(EditAnywhere)
	float DistanceThreshold = 2000.0f;
	UPROPERTY(EditAnywhere)
	float StickinessFactor = .1f;
	UPROPERTY(EditAnywhere)
	bool bIncludeZDistance = true;
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCTargetContext", ExcludeBaseStruct))
	FInstancedStruct TargetContext;

	virtual void InitializeBehavior(const AActor* Actor) override;
	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation, FRotator& UnclampedRotation) const override;

private:

	enum class ERotationSwitchBehavior : uint8
	{
		Valid,
		InRange,
		OutOfRange,
		Invalid
	};

	ERotationSwitchBehavior SwitchBehavior = ERotationSwitchBehavior::Valid;
	mutable bool bPreviouslyInRange = false;
};
