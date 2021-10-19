#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "PlayerAbilityHandler.h"
#include "GameFramework/RootMotionSource.h"
#include "RootMotionHandler.generated.h"

class USaiyoraMovementComponent;

UCLASS()
class SAIYORAV4_API URootMotionHandler : public UObject
{
	GENERATED_BODY()
private:
	FAbilityMispredictionCallback MispredictionCallback;
	UFUNCTION()
	void OnMispredicted(int32 const PredictionID, ECastFailReason const FailReason);
	UFUNCTION()
	void OnTimeout();
	//Use this function for things like "stop applying root motion on landing," and other non-prediction and non-duration-based endings.
	virtual void SetupExpirationConditions() {}
protected:
	FTimerHandle DurationHandle;
	bool bHasDuration = true;
	float Duration = 0.0f;
	UPROPERTY()
	USaiyoraMovementComponent* TargetMovement = nullptr;
	UPROPERTY()
	UPlayerAbilityHandler* SourceHandler = nullptr;
	uint16 RootMotionSourceID = (uint16)ERootMotionSourceID::Invalid;
	int32 PredictionID = 0;
	void Expire();
public:
	void Init(USaiyoraMovementComponent* Movement, UPlayerAbilityHandler* AbilityHandler, uint16 SourceID, bool const bDurationBased, float const ExpireDuration = 0.0f, int32 const AbilityPredictionID = 0);
	uint16 GetHandledID() const { return RootMotionSourceID; }
};
