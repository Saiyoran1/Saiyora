#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "GameFramework/RootMotionSource.h"
#include "SaiyoraRootMotionHandler.generated.h"

class USaiyoraMovementComponent;

UCLASS()
class SAIYORAV4_API USaiyoraRootMotionHandler : public UObject
{
	GENERATED_BODY()
	
private:
	UFUNCTION()
	void OnMispredicted(const int32 PredictionID);
	bool bInitialized = false;
	UPROPERTY(ReplicatedUsing = OnRep_Finished)
	bool bFinished = false;
	UFUNCTION()
	void OnRep_Finished();
	virtual TSharedPtr<FRootMotionSource> MakeRootMotionSource() { return nullptr; }
	virtual void PostApply() {}
	virtual void PostExpire() {}
	virtual bool NeedsExpireTimer() const { return Duration < 0.0f; }
	virtual void PreDestroyFromReplication() override;
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
	
	void Init(USaiyoraMovementComponent* Movement, const int32 PredictionID, UObject* MoveSource);
	void Apply();
	void CancelRootMotion();
	
	int32 GetPredictionID() const { return SourcePredictionID; }
	UObject* GetSource() const { return Source; }
	uint16 GetHandledID() const { return RootMotionSourceID; }

	UPROPERTY(Replicated)
	int32 Priority = 0;
	UPROPERTY(Replicated)
	ERootMotionAccumulateMode AccumulateMode = ERootMotionAccumulateMode::Override;
	UPROPERTY(Replicated)
	ERootMotionFinishVelocityMode FinishVelocityMode = ERootMotionFinishVelocityMode::ClampVelocity;
	UPROPERTY(Replicated)
	FVector FinishSetVelocity = FVector::ZeroVector;
	UPROPERTY(Replicated)
	float FinishClampVelocity = 0.0f;
	UPROPERTY(Replicated)
	float Duration = 0.0f;
	UPROPERTY(Replicated)
	bool bIgnoreRestrictions = false;
	FTimerHandle PingDelayHandle;
	FTimerHandle ExpireHandle;
};

UCLASS()
class SAIYORAV4_API UJumpForceHandler : public USaiyoraRootMotionHandler
{
	GENERATED_BODY()
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual TSharedPtr<FRootMotionSource> MakeRootMotionSource() override;
	virtual void PostApply() override;
	virtual void PostExpire() override;
	virtual bool NeedsExpireTimer() const override;
	UFUNCTION()
	void OnLanded(const FHitResult& Result);
public:
	UPROPERTY(Replicated)
	FRotator Rotation;
	UPROPERTY(Replicated)
	float Distance = 0.0f;
	UPROPERTY(Replicated)
	float Height = 0.0f;
	UPROPERTY(Replicated)
	float MinimumLandedTriggerTime = 0.0f;
	UPROPERTY(Replicated)
	bool bFinishOnLanded = false;
	UPROPERTY(Replicated)
	UCurveVector* PathOffsetCurve = nullptr;
	UPROPERTY(Replicated)
	UCurveFloat* TimeMappingCurve = nullptr;
};

UCLASS()
class SAIYORAV4_API UConstantForceHandler : public USaiyoraRootMotionHandler
{
	GENERATED_BODY()

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual TSharedPtr<FRootMotionSource> MakeRootMotionSource() override;
public:
	UPROPERTY(Replicated)
	FVector Force = FVector::ZeroVector;
	UPROPERTY(Replicated)
	UCurveFloat* StrengthOverTime = nullptr;
};