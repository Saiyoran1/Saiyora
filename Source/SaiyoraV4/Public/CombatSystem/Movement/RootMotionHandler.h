#pragma once
#include "CoreMinimal.h"
#include "PlayerAbilityHandler.h"
#include "GameFramework/RootMotionSource.h"
#include "RootMotionHandler.generated.h"

UCLASS()
class SAIYORAV4_API URootMotionHandler : public UObject
{
	GENERATED_BODY()
protected:
	bool bHasDuration = true;
	float Duration = 0.0f;
	ERootMotionFinishVelocityMode FinishVelocityMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	FVector FinishSetVelocity = FVector::ZeroVector;
	float FinishClampVelocity = 0.0f;
	UPROPERTY()
	UCharacterMovementComponent* TargetMovement = nullptr;
	uint16 RootMotionSourceID = (uint16)ERootMotionSourceID::Invalid;
	int32 PredictionID = 0;

	float StartTime = 0.0f;
	float EndTime = 0.0f;

	bool HasTimedOut() const;
	virtual void Apply();
};

UCLASS()
class SAIYORAV4_API URootMotionJumpForceHandler : public URootMotionHandler
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (HidePin = "Source", DefaultToSelf = "Source"))
	static URootMotionJumpForceHandler* RootMotionJumpForce(UPlayerCombatAbility* Source, UCharacterMovementComponent* Movement, FRotator Rotation, float Distance, float Height, float Duration, float MinimumLandedTriggerTime,
		bool bFinishOnLanded, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish, UCurveVector* PathOffsetCurve, UCurveFloat* TimeMappingCurve);

	FRotator Rotation = FRotator::ZeroRotator;
	float Distance = 0.0f;
	float Height = 0.0f;
	float Duration = 0.0f;
	float MinimumLandedTriggerTime = 0.0f;
	bool bFinishOnLanded = true;
	UPROPERTY()
	UCurveVector* PathOffsetCurve = nullptr;
	UPROPERTY()
	UCurveFloat* TimeMappingCurve = nullptr;

	bool bHasLanded = false;
};
