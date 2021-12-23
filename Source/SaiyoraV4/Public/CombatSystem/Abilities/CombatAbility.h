#pragma once
#include "CoreMinimal.h"

#include "AbilityComponent.h"
#include "UObject/NoExportTypes.h"
#include "AbilityStructs.h"
#include "CrowdControlEnums.h"
#include "DamageEnums.h"
#include "SaiyoraObjects.h"
#include "CombatAbility.generated.h"

class UCrowdControl;
class UAbilityComponent;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UCombatAbility : public UObject
{
	GENERATED_BODY()

//Setup
    
public:

    virtual UWorld* GetWorld() const override;
    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PostNetReceive() override;
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    UAbilityComponent* GetHandler() const { return OwningComponent; }
    void InitializeAbility(UAbilityComponent* AbilityComponent);
    bool IsInitialized() const { return bInitialized; }
    void DeactivateAbility();
    bool IsDeactivated() const { return bDeactivated; }
    
protected:
    
    UFUNCTION(BlueprintNativeEvent)
    void PreInitializeAbility();
    virtual void PreInitializeAbility_Implementation() {}
    UFUNCTION(BlueprintNativeEvent)
    void PreDeactivateAbility();
    virtual void PreDeactivateAbility_Implementation() {}
    
private:
    
    UPROPERTY(Replicated)
    UAbilityComponent* OwningComponent;
    bool bInitialized = false;
    UPROPERTY(ReplicatedUsing = OnRep_Deactivated)
    bool bDeactivated = false; 
    UFUNCTION()
    void OnRep_Deactivated(bool const Previous);
    
//Basic Info
    
public:
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FName GetAbilityName() const { return Name; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FText GetAbilityDescription() const { return Description; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    UTexture2D* GetAbilityIcon() const { return Icon; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    EDamageSchool GetAbilitySchool() const { return School; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    ESaiyoraPlane GetAbilityPlane() const { return Plane; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    void GetAbilityTags(FGameplayTagContainer& OutTags) const { OutTags.AppendTags(AbilityTags); }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasTag(FGameplayTag const Tag) const { return AbilityTags.HasTag(Tag); }
    
private:
    
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FName Name;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FText Description;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    UTexture2D* Icon;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    EDamageSchool School;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    ESaiyoraPlane Plane;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FGameplayTagContainer AbilityTags;
    
//Restrictions
    
public:

    UFUNCTION(BlueprintCallable, BlueprintPure)
    ECastFailReason IsCastable() const { return Castable; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsCastableWhileDead() const { return bCastableWhileDead; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    void GetRestrictedCrowdControls(TSet<ECrowdControlType>& OutCrowdControls) const { OutCrowdControls.Append(RestrictedCrowdControls); }
    void AddRestrictedTag(FGameplayTag const RestrictedTag);
    void RemoveRestrictedTag(FGameplayTag const RestrictedTag);
    void AddClassRestriction();
    void RemoveClassRestriction();
    
protected:
    
    UFUNCTION(BlueprintNativeEvent)
    void SetupCustomCastRestrictions();
    virtual void SetupCustomCastRestrictions_Implementation() {}
    UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (GameplayTagFilter="AbilityRestriction"))
    void ActivateCastRestriction(FGameplayTag const RestrictionTag);
    UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (GameplayTagFilter="AbilityRestriction"))
    void DeactivateCastRestriction(FGameplayTag const RestrictionTag);
    
private:
    
    ECastFailReason Castable = ECastFailReason::InvalidAbility;
    void UpdateCastable();
    UPROPERTY(EditDefaultsOnly, Category = "Restrictions")
    bool bCastableWhileDead = false;
    UPROPERTY(EditDefaultsOnly, Category = "Restrictions")
    TSet<ECrowdControlType> RestrictedCrowdControls;
    TSet<FGameplayTag> CustomCastRestrictions;
    TSet<FGameplayTag> RestrictedTags;
    bool bClassRestricted = false;

//Cast

public:

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    EAbilityCastType GetCastType() const { return CastType; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetDefaultCastLength() const { return DefaultCastTime; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetCastLength() const { return !bStaticCastTime && IsValid(OwningComponent) && OwningComponent->GetOwnerRole() == ROLE_Authority ?
        OwningComponent->CalculateCastLength(this, false) : DefaultCastTime; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticCastLength() const { return bStaticCastTime; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasInitialTick() const { return bInitialTick; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetNumberOfTicks() const { return NonInitialTicks; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsInterruptible() const { return bInterruptible; }

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
    UPROPERTY(EditDefaultsOnly, Category = "Cast")
    bool bInterruptible = true;
 
//Global Cooldown
    
public:
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasGlobalCooldown() const { return bOnGlobalCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetDefaultGlobalCooldownLength() const { return DefaultGlobalCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetGlobalCooldownLength() const { return !bStaticGlobalCooldownLength && IsValid(OwningComponent) && OwningComponent->GetOwnerRole() == ROLE_Authority ?
        OwningComponent->CalculateGlobalCooldownLength(this, false) : DefaultGlobalCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticGlobalCooldownLength() const { return bStaticGlobalCooldownLength; }
    
private:
    
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bOnGlobalCooldown = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    float DefaultGlobalCooldownLength = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticGlobalCooldownLength = true;
    
//Cooldown
    
public:
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetDefaultCooldownLength() const { return DefaultCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetCooldownLength() const { return !bStaticCooldownLength && IsValid(OwningComponent) && OwningComponent->GetOwnerRole() == ROLE_Authority ?
        OwningComponent->CalculateCooldownLength(this, false) : DefaultCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticCooldownLength() const { return bStaticCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool IsCooldownActive() const;
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetRemainingCooldown() const;
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetCurrentCooldownLength() const;
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetDefaultMaxCharges() const { return DefaultMaxCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetMaxCharges() const { return AbilityCooldown.MaxCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticMaxCharges() const { return bStaticMaxCharges; }
    void AddMaxChargeModifier(UBuff* Source, FCombatModifier const& Modifier);
    void RemoveMaxChargeModifier(UBuff* Source);
    void RecalculateMaxCharges();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargeCost() const { return DefaultChargeCost; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetChargeCost() const { return ChargeCost; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticChargeCost() const { return bStaticChargeCost; }
    void AddChargeCostModifier(UBuff* Source, FCombatModifier const& Modifier);
    void RemoveChargeCostModifier(UBuff* Source);
    void RecalculateChargeCost();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargesPerCooldown() const { return DefaultChargesPerCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetChargesPerCooldown() const { return ChargesPerCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticChargesPerCooldown() const { return bStaticChargesPerCooldown; }
    void AddChargesPerCooldownModifier(UBuff* Source, FCombatModifier const& Modifier);
    void RemoveChargesPerCooldownModifier(UBuff* Source);
    void RecalculateChargesPerCooldown();
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetCurrentCharges() const { return AbilityCooldown.CurrentCharges; }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void ModifyCurrentCharges(int32 const Charges, EChargeModificationType const Modification = EChargeModificationType::Additive);
    void CommitCharges(int32 const PredictionID = 0);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SubscribeToChargesChanged(FAbilityChargeCallback const& Callback);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void UnsubscribeFromChargesChanged(FAbilityChargeCallback const& Callback);
    
private:

    UPROPERTY(ReplicatedUsing=OnRep_AbilityCooldown)
    FAbilityCooldown AbilityCooldown;
    UFUNCTION()
    void OnRep_AbilityCooldown(FAbilityCooldown const& PreviousState);
    FTimerHandle CooldownHandle;
    void StartCooldown(bool const bUseLagCompensation = false);
    UFUNCTION()
    void CompleteCooldown();
    void CancelCooldown();
    FAbilityCooldown ClientCooldown;
    TMap<int32, int32> ChargePredictions;
    void RecalculatePredictedCooldown();
    FAbilityChargeNotification OnChargesChanged;

    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    float DefaultCooldownLength = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticCooldownLength = true;
    
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultMaxCharges = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticMaxCharges = true;
    UPROPERTY()
    TMap<UBuff*, FCombatModifier> MaxChargeModifiers;

    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultChargeCost = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticChargeCost = true;
    UPROPERTY(ReplicatedUsing=OnRep_ChargeCost)
    int32 ChargeCost = 1;
    //TODO: OnRep_ChargeCost client castable check.
    UPROPERTY()
    TMap<UBuff*, FCombatModifier> ChargeCostModifiers;

    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultChargesPerCooldown = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticChargesPerCooldown = true;
    int32 ChargesPerCooldown = 1;
    UPROPERTY()
    TMap<UBuff*, FCombatModifier> ChargesPerCooldownModifiers;
    
//Costs
    
public:

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    void GetDefaultAbilityCosts(TArray<FDefaultAbilityCost>& OutCosts) const { OutCosts.Append(DefaultAbilityCosts); }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    void GetAbilityCosts(TArray<FAbilityCost>& OutCosts) const { OutCosts.Append(AbilityCosts.Items); }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void AddResourceCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void RemoveResourceCostModifier(TSubclassOf<UResource> const ResourceClass, UBuff* Source);
    
private:
    
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    TArray<FDefaultAbilityCost> DefaultAbilityCosts;
    UPROPERTY(Replicated)
    FAbilityCostArray AbilityCosts;
    TMultiMap<TSubclassOf<UResource>, FCombatModifier> ResourceCostModifiers;
    FResourceValueCallback OnResourceChanged;
    UFUNCTION()
    void CheckResourceCostOnResourceChanged(UResource* Resource, UObject* ChangeSource, FResourceState const& PreviousState, FResourceState const& NewState);
    void UpdateCost(TSubclassOf<UResource> const ResourceClass);
    void CheckCostMet(TSubclassOf<UResource> const ResourceClass);
    TSet<TSubclassOf<UResource>> UnmetCosts;

//Functionality

public:

    int32 GetPredictionID() const { return CurrentPredictionID; }
    void PredictedTick(int32 const TickNumber, FCombatParameters& PredictionParams, int32 const PredictionID = 0);
    void ServerTick(int32 const TickNumber, FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams, int32 const PredictionID = 0);
    void SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams);
    void PredictedCancel(FCombatParameters& PredictionParams, int32 const PredictionID = 0);
    void ServerCancel(FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams, int32 const PredictionID = 0);
    void SimulatedCancel(FCombatParameters const& BroadcastParams);
    void ServerInterrupt(FInterruptEvent const& InterruptEvent);
    void SimulatedInterrupt(FInterruptEvent const& InterruptEvent);
    void UpdatePredictionFromServer(FServerAbilityResult const& Result);
    
protected:

    UFUNCTION(BlueprintNativeEvent)
    void OnPredictedTick(int32 const TickNumber);
    virtual void OnPredictedTick_Implementation(int32 const TickNumber) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnServerTick(int32 const TickNumber);
    virtual void OnServerTick_Implementation(int32 const TickNumber) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnSimulatedTick(int32 const TickNumber);
    virtual void OnSimulatedTick_Implementation(int32 const TickNumber) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnPredictedCancel();
    virtual void OnPredictedCancel_Implementation() {}
    UFUNCTION(BlueprintNativeEvent)
    void OnServerCancel();
    virtual void OnServerCancel_Implementation() {}
    UFUNCTION(BlueprintNativeEvent)
    void OnSimulatedCancel();
    virtual void OnSimulatedCancel_Implementation() {}
    UFUNCTION(BlueprintNativeEvent)
    void OnServerInterrupt(FInterruptEvent const& InterruptEvent);
    virtual void OnServerInterrupt_Implementation(FInterruptEvent const& InterruptEvent) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnSimulatedInterrupt(FInterruptEvent const& InterruptEvent);
    virtual void OnSimulatedInterrupt_Implementation(FInterruptEvent const& InterruptEvent) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnMisprediction(int32 const PredictionID, ECastFailReason const FailReason);
    virtual void OnMisprediction_Implementation(int32 const PredictionID, ECastFailReason const FailReason) {}

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddPredictionParameter(FCombatParameter const& Parameter);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FCombatParameter GetPredictionParamByName(FString const& ParamName);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FCombatParameter GetPredictionParamByType(ECombatParamType const ParamType);

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddBroadcastParameter(FCombatParameter const& Parameter);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FCombatParameter GetBroadcastParamByName(FString const& ParamName);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FCombatParameter GetBroadcastParamByType(ECombatParamType const ParamType);

private:

    int32 CurrentPredictionID = 0;
    FCombatParameters PredictionParameters;
    FCombatParameters BroadcastParameters;
};