#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "PlayerAbilityHandler.h"
#include "GameFramework/RootMotionSource.h"
#include "SaiyoraRootMotionHandler.generated.h"

class USaiyoraMovementComponent;

UCLASS()
class SAIYORAV4_API USaiyoraRootMotionHandler : public UObject
{
	GENERATED_BODY()
	
private:
	FAbilityMispredictionCallback MispredictionCallback;
	UFUNCTION()
	void OnMispredicted(int32 const PredictionID, ECastFailReason const FailReason);
	bool bInitialized = false;
	UPROPERTY(ReplicatedUsing=OnRep_Finished)
	bool bFinished = false;
	UFUNCTION()
	void OnRep_Finished();
	virtual TSharedPtr<FRootMotionSource> MakeRootMotionSource() { return nullptr; }
	virtual void PostInit() {}
protected:
	UPROPERTY(Replicated)
	USaiyoraMovementComponent* TargetMovement = nullptr;
	UPROPERTY(Replicated)
	int32 SourcePredictionID = 0;
	UPROPERTY(Replicated)
	UObject* Source = nullptr;
	uint16 RootMotionSourceID = (uint16)ERootMotionSourceID::Invalid;
	void Expire();
public:
	virtual UWorld* GetWorld() const override;
	virtual void PostNetReceive() override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void Init(USaiyoraMovementComponent* Movement, int32 const PredictionID, UObject* MoveSource);
	void Apply();
	
	int32 GetPredictionID() const { return SourcePredictionID; }
	UObject* GetSource() const { return Source; }
	uint16 GetHandledID() const { return RootMotionSourceID; }

	UPROPERTY(Replicated)
	int32 Priority = 0;
	UPROPERTY(Replicated)
	ERootMotionAccumulateMode AccumulateMode = ERootMotionAccumulateMode::Override;
	UPROPERTY(Replicated)
	ERootMotionFinishVelocityMode FinishVelocityMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	UPROPERTY(Replicated)
	FVector FinishSetVelocity = FVector::ZeroVector;
	UPROPERTY(Replicated)
	float FinishClampVelocity = 0.0f;
	FTimerHandle PingDelayHandle;
};

UCLASS()
class SAIYORAV4_API UTestJumpForceHandler : public USaiyoraRootMotionHandler
{
	GENERATED_BODY()
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual TSharedPtr<FRootMotionSource> MakeRootMotionSource() override;
public:
	UPROPERTY(Replicated)
	FRotator Rotation;
	UPROPERTY(Replicated)
	float Distance;
	UPROPERTY(Replicated)
	float Height;
	UPROPERTY(Replicated)
	float Duration;
	UPROPERTY(Replicated)
	float MinimumLandedTriggerTime;
	UPROPERTY(Replicated)
	bool bFinishOnLanded;
	UPROPERTY(Replicated)
	UCurveVector* PathOffsetCurve;
	UPROPERTY(Replicated)
	UCurveFloat* TimeMappingCurve;
};