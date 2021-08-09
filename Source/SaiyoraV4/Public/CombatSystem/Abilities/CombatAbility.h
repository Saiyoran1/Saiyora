// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AbilityStructs.h"
#include "DamageEnums.h"
#include "CombatAbility.generated.h"

class UCrowdControl;
class UAbilityHandler;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UCombatAbility : public UObject
{
	GENERATED_BODY()

//Setup
public:
    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UWorld* GetWorld() const override;
    UFUNCTION(BlueprintCallable, BlueprintPure)
    UAbilityHandler* GetHandler() const { return OwningComponent; }
    bool GetInitialized() const { return bInitialized; }
    bool GetDeactivated() const { return bDeactivated; }
    virtual void InitializeAbility(UAbilityHandler* AbilityComponent);
    void DeactivateAbility();
protected:
    UPROPERTY(ReplicatedUsing = OnRep_OwningComponent)
    UAbilityHandler* OwningComponent;
    bool bInitialized = false;
    UFUNCTION(BlueprintNativeEvent)
    void OnInitialize();
    UPROPERTY(ReplicatedUsing = OnRep_Deactivated)
    bool bDeactivated = false;  
    UFUNCTION(BlueprintNativeEvent)
    void OnDeactivate();
private: 
    UFUNCTION()
    void OnRep_OwningComponent();
    UFUNCTION()
    void OnRep_Deactivated(bool const Previous);
//Basic Info
public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    FName GetAbilityName() const { return Name; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    FText GetAbilityDescription() const { return Description; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    UTexture2D* GetAbilityIcon() const { return Icon; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    EDamageSchool GetAbilitySchool() const { return AbilitySchool; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    ESaiyoraPlane GetAbilityPlane() const { return AbilityPlane; }
private:
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FName Name;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FText Description;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    UTexture2D* Icon;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    EDamageSchool AbilitySchool;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    ESaiyoraPlane AbilityPlane;
//Casting
public:
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
    void ServerTick(int32 const TickNumber, FCombatParameters& BroadcastParams);
    void SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams);
    void CompleteCast();
    void InterruptCast(FInterruptEvent const& InterruptEvent);
    void ServerCancel(FCombatParameters& BroadcastParams); 
    void SimulatedCancel(FCombatParameters const& BroadcastParams);
protected:
    UFUNCTION(BlueprintNativeEvent)
    void SetupCustomCastRestrictions();
    UFUNCTION(BlueprintCallable, Category = "Cast")
    void ActivateCastRestriction(FName const& RestrictionName);
    UFUNCTION(BlueprintCallable, Category = "Cast")
    void DeactivateCastRestriction(FName const& RestrictionName);
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
    void OnSimulatedCancel();
    UPROPERTY(BlueprintReadWrite, Transient, Category = "Abilities")
    FCombatParameters BroadcastParameters;
private:
    UPROPERTY(EditDefaultsOnly, Category = "Cast")
    EAbilityCastType CastType = EAbilityCastType::None;
    UPROPERTY(EditDefaultsOnly, Category = "Cast")
    float DefaultCastTime = 0.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cast")
    bool bStaticCastTime = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cast")
    bool bInitialTick = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cast")
    int32 NonInitialTicks = 0;
    UPROPERTY(EditDefaultsOnly, Category = "Crowd Control")
    bool bInterruptible = true;
    UPROPERTY(EditDefaultsOnly, Category = "Crowd Control")
    bool bCastableWhileDead = false;
    UPROPERTY(EditDefaultsOnly, Category = "Crowd Control")
    TArray<TSubclassOf<UCrowdControl>> RestrictedCrowdControls;
    bool bCustomCastConditionsMet = true;
    TArray<FName> CustomCastRestrictions;
//Global Cooldown
public:
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetHasGlobalCooldown() const { return bOnGlobalCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetDefaultGlobalCooldownLength() const { return DefaultGlobalCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasStaticGlobalCooldown() const { return bStaticGlobalCooldown; }
private:
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bOnGlobalCooldown = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    float DefaultGlobalCooldownLength = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticGlobalCooldown = true;
//Cooldown
public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    float GetDefaultCooldown() const { return DefaultCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    bool GetHasStaticCooldown() const { return bStaticCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    bool GetCooldownActive() const { return AbilityCooldown.OnCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    float GetRemainingCooldown() const { return AbilityCooldown.OnCooldown ? GetWorld()->GetTimerManager().GetTimerRemaining(CooldownHandle) : 0.0f; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    float GetCurrentCooldownLength() const { return AbilityCooldown.OnCooldown ? AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime : 0.0f; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetDefaultMaxCharges() const { return DefaultMaxCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    bool GetHasStaticMaxCharges() const { return bStaticMaxCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetCurrentCharges() const { return AbilityCooldown.CurrentCharges; }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Cooldown")
    void ModifyCurrentCharges(int32 const Charges, bool const bAdditive = true);
    UFUNCTION(BlueprintCallable, Category = "Cooldown")
    void SubscribeToChargesChanged(FAbilityChargeCallback const& Callback);
    UFUNCTION(BlueprintCallable, Category = "Cooldown")
    void UnsubscribeFromChargesChanged(FAbilityChargeCallback const& Callback);
    void CommitCharges();
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargeCost() const { return DefaultChargeCost; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticChargeCost() const { return bStaticChargeCost; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargesPerCooldown() const { return DefaultChargesPerCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool GetHasStaticChargesPerCooldown() const { return bStaticChargesPerCooldown; }
private:
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultMaxCharges = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticMaxCharges = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    float DefaultCooldown = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticCooldown = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultChargesPerCooldown = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticChargesPerCooldown = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultChargeCost = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticChargeCost = true;
protected:
    FAbilityCooldown AbilityCooldown;
    FTimerHandle CooldownHandle;
    FAbilityChargeNotification OnChargesChanged;
    void StartCooldown();
    UFUNCTION()
    void CompleteCooldown();
//Costs
public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cost")
    FAbilityCost GetDefaultAbilityCost(TSubclassOf<UResource> const ResourceClass) const;
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cost")
    void GetDefaultAbilityCosts(TArray<FAbilityCost>& OutCosts) const { OutCosts = DefaultAbilityCosts; }
private:
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    TArray<FAbilityCost> DefaultAbilityCosts;
};