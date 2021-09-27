// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityHandler.h"
#include "CombatSystem/Abilities/CombatAbility.h"
#include "PlayerCombatAbility.generated.h"

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UPlayerCombatAbility : public UCombatAbility
{
	GENERATED_BODY()
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
//Display Info
	UPROPERTY(EditDefaultsOnly, Category = "Display Info")
	bool bHiddenCastBar = false;
	UPROPERTY(EditDefaultsOnly, Category = "Display Info")
	bool bHiddenOnActionBar = false;
	UPROPERTY(EditDefaultsOnly, Category = "Display Info")
	EActionBarType ActionBar = EActionBarType::None;
	UPROPERTY(EditDefaultsOnly, Category = "Cast Info")
    TSet<int32> TicksWithPredictionParams;
public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetHiddenCastBar() const { return bHiddenCastBar; }
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetHiddenOnActionBar() const { return bHiddenOnActionBar; }
	UFUNCTION(BlueprintCallable, BlueprintPure)
	EActionBarType GetActionBar() const { return ActionBar; }
//Cooldown
private:
	FAbilityCooldown ClientCooldown;
	virtual void OnRep_AbilityCooldown() override;
    TMap<int32, int32> ChargePredictions;
    void PurgeOldPredictions();
    void RecalculatePredictedCooldown();
    TSet<int32> StoredTickPredictionParameters;
	virtual void AdjustCooldownFromMaxChargesChanged() override;
public:
	virtual bool GetCooldownActive() const override;
	virtual float GetRemainingCooldown() const override;
	virtual float GetCurrentCooldownLength() const override;
	virtual void CheckChargeCostMet() override;
	void PredictCommitCharges(int32 const PredictionID);
	void CommitCharges(int32 const PredictionID = 0);
	void UpdatePredictedChargesFromServer(int32 const PredictionID, int32 const ChargesSpent);
	void StartCooldownFromPrediction();
//Casting
	bool GetTickNeedsPredictionParams(int32 const TickNumber) const { return TicksWithPredictionParams.Contains(TickNumber); }
	void PredictedTick(int32 const TickNumber, FCombatParameters& PredictionParams);
	void ServerTick(int32 const TickNumber, FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams);
	void PredictedCancel(FCombatParameters& PredictionParams);
	void ServerCancel(FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams);
	void AbilityMisprediction(int32 const PredictionID, ECastFailReason const FailReason);
protected:
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Abilities")
	FCombatParameters PredictionParameters;
	UFUNCTION(BlueprintNativeEvent)
	void OnPredictedTick(int32 const TickNumber);
	UFUNCTION(BlueprintNativeEvent)
	void OnPredictedCancel();
	UFUNCTION(BlueprintNativeEvent)
	void OnAbilityMispredicted(int32 const PredictionID, ECastFailReason const FailReason);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddPredictionParameter(FCombatParameter const& Parameter);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FCombatParameter GetPredictionParamByID(int32 const ID);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FCombatParameter GetPredictionParamByType(ECombatParamType const Type);
};