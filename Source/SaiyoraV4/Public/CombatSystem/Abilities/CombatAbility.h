// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AbilityStructs.h"
#include "DamageEnums.h"
#include "CombatAbility.generated.h"

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
    UPROPERTY(EditDefaultsOnly, Category = "Display Info")
    EDamageSchool AbilitySchool;
    UPROPERTY(EditDefaultsOnly, Category = "Display Info")
    ESaiyoraPlane AbilityPlane;

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
    TMap<TSubclassOf<UResource>, FAbilityCost> AbilityCosts;
    TMultiMap<TSubclassOf<UResource>, FCombatModifier> CostModifiers;
    void RecalculateAbilityCost(TSubclassOf<UResource> const ResourceClass);
    
        //***Ability Cooldown***

    //Server authoritative cooldown progress and charge status that the client can extrapolate from.
protected:
    FAbilityCooldown AbilityCooldown;
    UPROPERTY()
    int32 MaxCharges = 1;
    UPROPERTY()
    int32 ChargesPerCooldown = 1;
    FTimerHandle CooldownHandle;
    UPROPERTY()
    int32 ChargesPerCast = 1;
    FAbilityChargeNotification OnChargesChanged;
    void StartCooldown();
    UFUNCTION()
    void CompleteCooldown();
    TArray<FAbilityModCondition> CooldownMods;
private:
    bool bCustomCastConditionsMet = true;
    TArray<FName> CustomCastRestrictions;

    //References
protected:
    UPROPERTY(ReplicatedUsing = OnRep_OwningComponent)
    class UAbilityHandler* OwningComponent;
    bool bInitialized = false;
    UPROPERTY(ReplicatedUsing = OnRep_Deactivated)
    bool bDeactivated = false;
private: 
    //OnReps
    UFUNCTION()
    void OnRep_OwningComponent();
    UFUNCTION()
    void OnRep_Deactivated(bool const Previous);

public:

    //Getters
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FName GetAbilityName() const { return Name; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FText GetAbilityDescription() const { return Description; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    UTexture2D* GetAbilityIcon() const { return Icon; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    EDamageSchool GetAbilitySchool() const { return AbilitySchool; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    ESaiyoraPlane GetAbilityPlane() const { return AbilityPlane; }
    
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
    int32 GetCurrentCharges() const { return AbilityCooldown.CurrentCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetRemainingCooldown() const { return AbilityCooldown.OnCooldown ? GetWorld()->GetTimerManager().GetTimerRemaining(CooldownHandle) : 0.0f; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetCurrentCooldownLength() const { return AbilityCooldown.OnCooldown ? AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime : 0.0f; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetCooldownActive() const { return AbilityCooldown.OnCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetChargeCost() const { return ChargesPerCast; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool CheckChargesMet() const { return AbilityCooldown.CurrentCharges >= ChargesPerCast; }
    void CommitCharges();
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetAbilityCost(TSubclassOf<UResource> const ResourceClass) const;
    UFUNCTION(BlueprintCallable, BlueprintPure)
    void GetAbilityCosts(TArray<FAbilityCost>& OutCosts) const { AbilityCosts.GenerateValueArray(OutCosts); }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UAbilityHandler* GetHandler() const { return OwningComponent; }
    
    bool GetInitialized() const { return bInitialized; }
    bool GetDeactivated() const { return bDeactivated; }
    
    //Internal ability functions, these adjust necessary properties then call the Blueprint implementations.
    
    virtual void InitializeAbility(UAbilityHandler* AbilityComponent);
    void DeactivateAbility();
    void ServerTick(int32 const TickNumber, FCombatParameters& BroadcastParams);
    void SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams);
    void CompleteCast();
    void InterruptCast(FInterruptEvent const& InterruptEvent);
    void ServerCancel(FCombatParameters& BroadcastParams);
    void SimulatedCancel(FCombatParameters const& BroadcastParams);

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToChargesChanged(FAbilityChargeCallback const& Callback);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromChargesChanged(FAbilityChargeCallback const& Callback);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void AddAbilityCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void RemoveAbilityCostModifier(TSubclassOf<UResource> const ResourceClass, int32 const ModifierID);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void ModifyCurrentCharges(int32 const Charges, bool const bAdditive = true);

protected:

    UPROPERTY(BlueprintReadWrite, Transient, Category = "Abilities")
    FCombatParameters BroadcastParameters;
    
    //Blueprint functions for defining the ability behavior.

    UFUNCTION(BlueprintNativeEvent)
    void OnInitialize();
    UFUNCTION(BlueprintNativeEvent)
    void SetupCustomCastRestrictions();
    UFUNCTION(BlueprintNativeEvent)
    void OnDeactivate();
    
    UFUNCTION(BlueprintNativeEvent)
    void OnServerTick(int32 const TickNumber);
    UFUNCTION(BlueprintNativeEvent)
    void OnSimulatedTick(int32 const TickNumber);
    UFUNCTION(BlueprintNativeEvent)
    void OnCastComplete();
    UFUNCTION(BlueprintNativeEvent)
    void OnCastInterrupted(FInterruptEvent const& InterruptEvent);
    UFUNCTION(BlueprintNativeEvent)
    void OnServerCancel();
    UFUNCTION(BlueprintNativeEvent)
    void OnSimulatedCancel(TArray<FCombatParameter> const& BroadcastParams);

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void ActivateCastRestriction(FName const& RestrictionName);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void DeactivateCastRestriction(FName const& RestrictionName);
};