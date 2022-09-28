#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "Reload.generated.h"

UCLASS(Blueprintable)
class SAIYORAV4_API UReload : public UCombatAbility
{
	GENERATED_BODY()

public:

	UReload();

	virtual void PreInitializeAbility_Implementation() override;
	virtual void OnPredictedTick_Implementation(const int32 TickNumber) override;
	virtual void OnMisprediction_Implementation(const int32 PredictionID, const ECastFailReason FailReason) override;

private:

	UPROPERTY()
	UResource* AmmoRef;
	UFUNCTION()
	void OnResourceAdded(UResource* NewResource);
	void SetupAmmoTracking(UResource* AmmoResource);
	UFUNCTION()
	void OnAmmoChanged(UResource* Resource, UObject* ChangeSource, const FResourceState& PreviousState, const FResourceState& NewState);
	bool bWaitingOnReloadResult = false;
	bool bFullAmmo = false;
};