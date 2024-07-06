#pragma once
#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "RotationBehaviors.generated.h"

USTRUCT()
struct FNPCRotationBehavior
{
	GENERATED_BODY()

	//Whether to enforce max rotation speed or just snap to the desired rotation.
	UPROPERTY(EditAnywhere)
	bool bEnforceRotationSpeed = true;
	//When enforcing a max rotation speed, this is the max degrees per second that the NPC can rotate.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bEnforceRotationSpeed"))
	float MaxRotationSpeed = 180.0f;
	//Whether to rotate only yaw, or to rotate on multiple axes.
	UPROPERTY(EditAnywhere)
	bool bOnlyYaw = true;

	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation) const {}
	virtual ~FNPCRotationBehavior() {}
};

USTRUCT()
struct FNPCRB_OrientToMovement : public FNPCRotationBehavior
{
	GENERATED_BODY()

	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation) const override;
};

USTRUCT()
struct FNPCRB_OrientToTarget : public FNPCRotationBehavior
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCTargetContext", ExcludeBaseStruct))
	FInstancedStruct TargetContext;

	virtual void ModifyRotation(const float DeltaTime, const AActor* Actor, FRotator& OutRotation) const override;
};
