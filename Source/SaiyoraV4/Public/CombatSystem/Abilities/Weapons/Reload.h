#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "Reload.generated.h"

class ASaiyoraPlayerCharacter;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UReload : public UCombatAbility
{
	GENERATED_BODY()

public:

	UReload();

	virtual void PostInitializeAbility_Implementation() override;
	virtual void OnPredictedTick_Implementation(const int32 TickNumber) override;
	virtual void OnServerTick_Implementation(const int32 TickNumber) override;
	virtual void OnSimulatedTick_Implementation(const int32 TickNumber) override;
	virtual void OnMisprediction_Implementation(const int32 PredictionID, const TArray<ECastFailReason>& FailReasons) override;
	virtual void OnPredictedCancel_Implementation() override;
	virtual void OnServerCancel_Implementation() override;
	virtual void OnSimulatedCancel_Implementation() override;

private:

    UPROPERTY(EditDefaultsOnly, Category = "Reload")
    UAnimMontage* ReloadMontage;

	UPROPERTY()
	UResource* AmmoRef;
	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
	UFUNCTION()
	void OnResourceAdded(UResource* NewResource);
	void SetupAmmoTracking(UResource* AmmoResource);
	UFUNCTION()
	void OnAmmoChanged(UResource* Resource, UObject* ChangeSource, const FResourceState& PreviousState, const FResourceState& NewState);
	bool bWaitingOnReloadResult = false;
	bool bFullAmmo = false;
	void StartReloadMontage();
	void CancelReloadMontage();
};