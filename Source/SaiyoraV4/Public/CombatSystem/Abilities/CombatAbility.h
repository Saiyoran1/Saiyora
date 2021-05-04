// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "AbilityStructs.h"
#include "CombatAbility.generated.h"

struct FCombatModifier;
class UCrowdControl;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UCombatAbility : public UObject
{
	GENERATED_BODY()

public:

    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UWorld* GetWorld() const override;

private:

    UPROPERTY(EditDefaultsOnly, Category = "Display Info")
    FName Name;
    UPROPERTY(EditDefaultsOnly, Category = "Display Info")
    FText Description;
    UPROPERTY(EditDefaultsOnly, Category = "Display Info")
    UTexture2D* Icon;
    UPROPERTY(EditDefaultsOnly, Category = "Display Info", meta = (Categories = "Ability"))
    FGameplayTag AbilityClass;

    UPROPERTY(EditDefaultsOnly, Category = "Cast Info")
    EAbilityCastType CastType = EAbilityCastType::None;
    UPROPERTY(EditDefaultsOnly, Category = "Cast Info")
    float DefaultCastTime = 0.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cast Info")
    bool bStaticCastTime = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cast Info")
    bool bInitialTick = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cast Info")
    int32 NonInitialTicks = 0;
    UPROPERTY(EditDefaultsOnly, Category = "Cast Info")
    TSet<int32> TicksWithPredictionParams;
    
    UPROPERTY(EditDefaultsOnly, Category = "Crowd Control Info")
    bool bInterruptible = true;
    UPROPERTY(EditDefaultsOnly, Category = "Crowd Control Info")
    TArray<TSubclassOf<UCrowdControl>> RestrictedCrowdControls;
    UPROPERTY(EditDefaultsOnly, Category = "Crowd Control Info")
    bool bCastableWhileDead = false;
    
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    bool bOnGlobalCooldown = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    float DefaultGlobalCooldownLength = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    bool bStaticGlobalCooldown = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    int32 DefaultMaxCharges = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    float DefaultCooldown = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    bool bStaticCooldown = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    int32 DefaultChargesPerCooldown = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    int32 DefaultChargesPerCast = 1;

    UPROPERTY(EditDefaultsOnly, Category = "Cost Info")
    TArray<FAbilityCost> DefaultAbilityCosts;

        //***Ability Costs***

    //Mutable cost values, these are what are actually checked at runtime and modified.
    UPROPERTY()
    TMap<FGameplayTag, FAbilityCost> AbilityCosts;
    
        //***Ability Cooldown***

    //Server authoritative cooldown progress and charge status that the client can extrapolate from.
    UPROPERTY(ReplicatedUsing = OnRep_AbilityCooldown)
    FAbilityCooldown AbilityCooldown;
    FAbilityCooldown PredictedCooldown;
    TMap<int32, int32> ChargePredictions;
    void PurgeOldPredictions();
    void RecalculatePredictedCooldown();
    UPROPERTY(Replicated)
    int32 MaxCharges = 1;
    int32 ChargesPerCooldown = 1;
    FTimerHandle CooldownHandle;
    UPROPERTY(Replicated)
    int32 ChargesPerCast = 1;
    FAbilityChargeNotification OnChargesChanged;
    void StartCooldown(bool const bFromPrediction);
    UFUNCTION()
    void CompleteCooldown();
    TArray<FAbilityModCondition> CooldownMods;
    
    bool bCustomCastConditionsMet = true;
    TArray<FName> CustomCastRestrictions;

    //References
    UPROPERTY(ReplicatedUsing = OnRep_OwningComponent)
    class UAbilityHandler* OwningComponent;
    bool bInitialized = false;
    UPROPERTY(ReplicatedUsing = OnRep_Deactivated)
    bool bDeactivated = false;
    
    //OnReps
    UFUNCTION()
    void OnRep_OwningComponent();
    UFUNCTION()
    void OnRep_AbilityCooldown();
    UFUNCTION()
    void OnRep_Deactivated(bool const Previous);

    TSet<int32> StoredTickPredictionParameters;

public:

    //Getters
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FName GetAbilityName() const { return Name; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FText GetAbilityDescription() const { return Description; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    UTexture2D* GetAbilityIcon() const { return Icon; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FGameplayTag GetAbilityClass() const { return AbilityClass; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    EAbilityCastType GetCastType() const { return CastType; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetDefaultCastLength() const { return DefaultCastTime; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasStaticCastLength() const { return bStaticCastTime; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetHasInitialTick() const { return bInitialTick; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetNumberOfTicks() const { return NonInitialTicks; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetInterruptible() const { return bInterruptible; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetCastableWhileDead() const { return bCastableWhileDead; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    TArray<TSubclassOf<UCrowdControl>> GetRestrictedCrowdControls() const { return RestrictedCrowdControls; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool CheckCustomCastConditionsMet() const { return bCustomCastConditionsMet; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetHasGlobalCooldown() const { return bOnGlobalCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetDefaultGlobalCooldownLength() const { return DefaultGlobalCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasStaticGlobalCooldown() const { return bStaticGlobalCooldown; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetDefaultCooldown() const { return DefaultCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetHasStaticCooldown() const { return bStaticCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetMaxCharges() const { return MaxCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetCurrentCharges() const;
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetRemainingCooldown() const;
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetCurrentCooldownLength() const;
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetCooldownActive() const;
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetChargeCost() const { return ChargesPerCast; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool CheckChargesMet() const;
    void CommitCharges(int32 const PredictionID);
    void PredictCommitCharges(int32 const PredictionID);
    void RollbackFailedCharges(int32 const PredictionID);
    void UpdatePredictedChargesFromServer(FServerAbilityResult const& ServerResult);
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetAbilityCost(FGameplayTag const& ResourceTag) const;
    UFUNCTION(BlueprintCallable, BlueprintPure)
    void GetAbilityCosts(TArray<FAbilityCost>& OutCosts) const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UAbilityHandler* GetHandler() const { return OwningComponent; }
    
    bool GetInitialized() const { return bInitialized; }
    bool GetDeactivated() const { return bDeactivated; }

    bool GetTickNeedsPredictionParams(int32 const TickNumber) const;
    
    //Internal ability functions, these adjust necessary properties then call the Blueprint implementations.
    
    void InitializeAbility(UAbilityHandler* AbilityComponent);
    void DeactivateAbility();
    void PredictedTick(int32 const TickNumber, FCombatParameters& PredictionParams);
    void ServerPredictedTick(int32 const TickNumber, FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams);
    void ServerNonPredictedTick(int32 const TickNumber, FCombatParameters& BroadcastParams);
    void SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams);
    void CompleteCast();
    void InterruptCast(FInterruptEvent const& InterruptEvent);
    void CancelCast();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToChargesChanged(FAbilityChargeCallback const& Callback);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromChargesChanged(FAbilityChargeCallback const& Callback);

protected:
    
    //Blueprint functions for defining the ability behavior.

    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialize();
    UFUNCTION(BlueprintImplementableEvent)
    void SetupCustomCastRestrictions();
    UFUNCTION(BlueprintImplementableEvent)
    void OnDeactivate();
    UFUNCTION(BlueprintImplementableEvent)
    void OnPredictedTick(int32 const TickNumber, TArray<FCombatParameter>& PredictionParams);
    UFUNCTION(BlueprintImplementableEvent)
    void OnServerPredictedTick(int32 const TickNumber, TArray<FCombatParameter> const& PredictionParams, TArray<FCombatParameter>& BroadcastParams);
    UFUNCTION(BlueprintImplementableEvent)
    void OnServerNonPredictedTick(int32 const TickNumber, TArray<FCombatParameter>& BroadcastParams);
    UFUNCTION(BlueprintImplementableEvent)
    bool OnSimulatedTick(int32 const TickNumber, TArray<FCombatParameter> const& BroadcastParams);
    UFUNCTION(BlueprintImplementableEvent)
    void OnCastComplete();
    UFUNCTION(BlueprintImplementableEvent)
    void OnCastInterrupted(FInterruptEvent const& InterruptEvent);
    UFUNCTION(BlueprintImplementableEvent)
    void OnCastCancelled();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void ActivateCastRestriction(FName const& RestrictionName);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void DeactivateCastRestriction(FName const& RestrictionName);
};