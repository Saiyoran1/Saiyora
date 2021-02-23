// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatSystem/Abilities/AbilityHandler.h"
#include "PlayerAbilityHandler.generated.h"


UCLASS(Blueprintable)
class SAIYORAV4_API UPlayerAbilityHandler : public UAbilityHandler
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

public:
	
	virtual FCastEvent UseAbility(::TSubclassOf<UCombatAbility> const AbilityClass) override;

private:

	int32 PredictionCastID = 0;
	virtual void GenerateNewCastID(FCastEvent& CastEvent) override;
	
	//This function is a workaround, called on the server when the client sends it a UseAbility RPC.
	//When the server generates a new CastID, it will get this set value from the client instead of a new generated value.
	void SetPredictedCastID(int32 const PredictionID) { PredictionCastID = PredictionID; }

	FGlobalCooldown PredictedGlobal;
	virtual void StartGlobalCooldown(UCombatAbility* Ability, int32 const CastID) override;
	void RollBackFailedGlobal(int32 const FailID);
	virtual void OnRep_GlobalCooldownState(FGlobalCooldown const& PreviousGlobal) override;

	//TODO: Child class of UResourceHandler, for prediction.
	UPROPERTY()
	UResourceHandler* PlayerResourceHandler;
};