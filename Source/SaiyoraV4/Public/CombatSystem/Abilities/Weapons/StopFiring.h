#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "StopFiring.generated.h"

class ASaiyoraPlayerCharacter;

UCLASS()
class SAIYORAV4_API UStopFiring : public UCombatAbility
{
	GENERATED_BODY()

public:

	UStopFiring();

protected:

	virtual void PreInitializeAbility_Implementation() override;
	virtual void OnPredictedTick_Implementation(const int32 TickNumber) override;
	virtual void OnServerTick_Implementation(const int32 TickNumber) override;
	virtual void OnSimulatedTick_Implementation(const int32 TickNumber) override;

private:

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
	void StopFiringWeapon();
};