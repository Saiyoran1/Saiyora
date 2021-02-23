// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatSystem/Abilities/CombatAbility.h"
#include "PlayerAbility.generated.h"

UCLASS()
class SAIYORAV4_API UPlayerAbility : public UCombatAbility
{
	GENERATED_BODY()

public:
	
	virtual int32 GetCurrentCharges() const override;
	virtual bool CheckChargesMet() const override;
	virtual void CommitCharges(int32 const CastID) override;

private:

	TMap<int32, int32> ChargePredictions;
	int32 PredictedCharges;
	
	void PurgeOldPredictions();
	void PurgeFailedPrediction(int32 const FailID);
	void RecalculatePredictedCharges();
	virtual void OnRep_AbilityCooldown() override;

	//TODO: CollectParamsForServer().
};
