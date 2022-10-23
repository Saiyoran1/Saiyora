#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "GameplayTask.h"
#include "GameFramework/RootMotionSource.h"
#include "SaiyoraRootMotionHandler.generated.h"

class USaiyoraMovementComponent;
class UAbilityComponent;

UCLASS()
class SAIYORAV4_API USaiyoraRootMotionTask : public UGameplayTask
{
	GENERATED_BODY()

public:
	
	void Init(USaiyoraMovementComponent* Movement, UObject* MoveSource);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Activate() override;;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	bool IsExternal() const { return bExternal; }
	TSharedPtr<FRootMotionSource> GetRootMotionSource() const { return RMSource; }
	UObject* GetSource() const { return Source; }
	FPredictedTick GetPredictedTick() const { return PredictedTick; }
	virtual bool IsSupportedForNetworking() const override { return true; }

	UPROPERTY(Replicated)
	int32 Priority = 0;
	UPROPERTY(Replicated)
	ERootMotionAccumulateMode AccumulateMode = ERootMotionAccumulateMode::Override;
	UPROPERTY(Replicated)
	ERootMotionFinishVelocityMode FinishVelocityMode = ERootMotionFinishVelocityMode::SetVelocity;
	UPROPERTY(Replicated)
	FVector FinishSetVelocity = FVector::ZeroVector;
	UPROPERTY(Replicated)
	float FinishClampVelocity = 0.0f;
	UPROPERTY(Replicated)
	float Duration = 0.0f;
	UPROPERTY(Replicated)
	bool bIgnoreRestrictions = false;
	UPROPERTY(ReplicatedUsing = OnRep_ServerWaitID)
	int32 ServerWaitID = 0;

	FTimerHandle ServerWaitHandle;

private:

	UPROPERTY(Replicated)
	bool bExternal = true;
	UPROPERTY(Replicated)
	UObject* Source;
	UPROPERTY(Replicated)
	USaiyoraMovementComponent* MovementRef;
	UPROPERTY()
	UAbilityComponent* AbilityCompRef;
	TSharedPtr<FRootMotionSource> RMSource;
	uint16 RMSHandle = (uint16)ERootMotionSourceID::Invalid;
	bool bInitialized = false;
	FPredictedTick PredictedTick;
	virtual TSharedPtr<FRootMotionSource> MakeRootMotionSource() { return nullptr; }
	UFUNCTION()
	void OnMispredicted(const int32 PredictionID);
	UFUNCTION()
	void OnRep_ServerWaitID();
};

UCLASS()
class SAIYORAV4_API USaiyoraConstantForce : public USaiyoraRootMotionTask
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated)
	FVector Force = FVector::ZeroVector;
	UPROPERTY(Replicated)
	UCurveFloat* StrengthOverTime = nullptr;

private:

	virtual TSharedPtr<FRootMotionSource> MakeRootMotionSource() override;
};
