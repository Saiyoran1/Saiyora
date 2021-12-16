#pragma once
#include "CoreMinimal.h"
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
    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UWorld* GetWorld() const override;
    UFUNCTION(BlueprintCallable, BlueprintPure)
    UAbilityComponent* GetHandler() const { return OwningComponent; }
    bool IsInitialized() const { return bInitialized; }
    bool IsDeactivated() const { return bDeactivated; }
    virtual void InitializeAbility(UAbilityComponent* AbilityComponent);
    void DeactivateAbility();
protected:
    UPROPERTY(ReplicatedUsing = OnRep_OwningComponent)
    UAbilityComponent* OwningComponent;
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
    FName GetAbilityName() const { return AbilityName; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    FText GetAbilityDescription() const { return Description; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    UTexture2D* GetAbilityIcon() const { return Icon; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    EDamageSchool GetAbilitySchool() const { return AbilitySchool; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    ESaiyoraPlane GetAbilityPlane() const { return AbilityPlane; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    void GetAbilityTags(FGameplayTagContainer& OutTags) const { OutTags.AppendTags(AbilityTags); }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    bool HasTag(FGameplayTag const Tag) const { return AbilityTags.HasTag(Tag); }
private:
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FName AbilityName;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FText Description;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    UTexture2D* Icon;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    EDamageSchool AbilitySchool;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    ESaiyoraPlane AbilityPlane;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FGameplayTagContainer AbilityTags;
//Casting
public:
    UFUNCTION(BlueprintCallable, BlueprintPure)
    EAbilityCastType GetCastType() const { return CastType; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetDefaultCastLength() const { return DefaultCastTime; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetCastLength() const { return IsValid(CastLength) ? CastLength->GetValue() : DefaultCastTime; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasStaticCastLength() const { return bStaticCastTime; }
    UFUNCTION(BlueprintCallable)
    int32 AddCastLengthModifier(FCombatModifier const& Modifier);
    UFUNCTION(BlueprintCallable)
    void RemoveCastLengthModifier(int32 const ModifierID);
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasInitialTick() const { return bInitialTick; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetNumberOfTicks() const { return NonInitialTicks; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsInterruptible() const { return bInterruptible; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsCastableWhileDead() const { return bCastableWhileDead; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    TSet<ECrowdControlType> GetRestrictedCrowdControls() const { return RestrictedCrowdControls; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool CheckCustomCastConditionsMet() const { return bCustomCastConditionsMet; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    ECastFailReason IsCastable() const { return Castable; }
    UFUNCTION(BlueprintCallable)
    void SubscribeToCastableChanged(FCastableCallback const& Callback);
    UFUNCTION(BlueprintCallable)
    void UnsubscribeFromCastableChanged(FCastableCallback const& Callback);
    void PredictedTick(int32 const TickNumber, FCombatParameters& PredictionParams, int32 const PredictionID = 0);
    void ServerTick(int32 const TickNumber, FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams, int32 const PredictionID = 0);
    void SimulatedTick(int32 const TickNumber, FCombatParameters const& BroadcastParams);
    void CompleteCast();
    void UpdatePredictionFromServer(FServerAbilityResult const& Result);
    void ServerInterrupt(FInterruptEvent const& InterruptEvent);
    void SimulatedInterrupt(FInterruptEvent const& InterruptEvent);
    void PredictedCancel(FCombatParameters& PredictionParams, int32 const PredictionID = 0);
    void ServerCancel(FCombatParameters const& PredictionParams, FCombatParameters& BroadcastParams, int32 const PredictionID = 0);
    void SimulatedCancel(FCombatParameters const& BroadcastParams);
    void AddRestrictedTag(FGameplayTag const RestrictedTag);
    void RemoveRestrictedTag(FGameplayTag const RestrictedTag);
    void AddClassRestriction();
    void RemoveClassRestriction();
protected:
    UFUNCTION(BlueprintNativeEvent)
    void SetupCustomCastRestrictions();
    UFUNCTION(BlueprintCallable, Category = "Cast")
    void ActivateCastRestriction(FGameplayTag const RestrictionTag);
    UFUNCTION(BlueprintCallable, Category = "Cast")
    void DeactivateCastRestriction(FGameplayTag const RestrictionTag);
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
    FCombatParameters BroadcastParameters;
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddBroadcastParameter(FCombatParameter const& Parameter);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FCombatParameter GetBroadcastParamByName(FString const& Name);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    FCombatParameter GetBroadcastParamByType(ECombatParamType const Type);
    void UpdateCastable();
    virtual void InitialCastableChecks();
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
    TSet<ECrowdControlType> RestrictedCrowdControls;
    bool bCustomCastConditionsMet = true;
    TSet<FGameplayTag> CustomCastRestrictions;
    bool bTagsRestricted = false;
    TSet<FGameplayTag> RestrictedTags;
    bool bClassRestricted = false;
    ECastFailReason Castable = ECastFailReason::InvalidAbility;
    FCastableNotification OnCastableChanged;
//Global Cooldown
public:
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasGlobalCooldown() const { return bOnGlobalCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetGlobalCooldownLength() const { return DefaultGlobalCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
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
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    float GetDefaultCooldown() const { return DefaultCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    float GetCooldownLength() const { return IsValid(CooldownLength) ? CooldownLength->GetValue() : DefaultCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    bool HasStaticCooldownLength() const { return bStaticCooldownLength; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    virtual bool GetCooldownActive() const { return AbilityCooldown.OnCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    virtual float GetRemainingCooldown() const { return AbilityCooldown.OnCooldown ? GetWorld()->GetTimerManager().GetTimerRemaining(CooldownHandle) : 0.0f; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    virtual float GetCurrentCooldownLength() const { return AbilityCooldown.OnCooldown ? AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime : 0.0f; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetDefaultMaxCharges() const { return DefaultMaxCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetMaxCharges() const { return IsValid(MaxCharges) ? MaxCharges->GetValue() : DefaultMaxCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    bool GetHasStaticMaxCharges() const { return bStaticMaxCharges; }
    UFUNCTION(BlueprintCallable)
    int32 AddMaxChargeModifier(FCombatModifier const& Modifier);
    UFUNCTION(BlueprintCallable)
    void RemoveMaxChargeModifier(int32 const ModifierID);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetCurrentCharges() const { return AbilityCooldown.CurrentCharges; }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Cooldown")
    void ModifyCurrentCharges(int32 const Charges, bool const bAdditive = true);
    UFUNCTION(BlueprintCallable, Category = "Cooldown")
    void SubscribeToChargesChanged(FAbilityChargeCallback const& Callback);
    UFUNCTION(BlueprintCallable, Category = "Cooldown")
    void UnsubscribeFromChargesChanged(FAbilityChargeCallback const& Callback);
    void CommitCharges(int32 const PredictionID = 0);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetDefaultChargeCost() const { return DefaultChargeCost; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetChargeCost() const { return IsValid(ChargeCost) ? ChargeCost->GetValue() : DefaultChargeCost; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    bool HasStaticChargeCost() const { return bStaticChargeCost; }
    UFUNCTION(BlueprintCallable)
    int32 AddChargeCostModifier(FCombatModifier const& Modifier);
    UFUNCTION(BlueprintCallable)
    void RemoveChargeCostModifier(int32 const ModifierID);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetDefaultChargesPerCooldown() const { return DefaultChargesPerCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    int32 GetChargesPerCooldown() const { return IsValid(ChargesPerCooldown) ? ChargesPerCooldown->GetValue() : DefaultChargesPerCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cooldown")
    bool GetHasStaticChargesPerCooldown() const { return bStaticChargesPerCooldown; }
    UFUNCTION(BlueprintCallable)
    int32 AddChargesPerCooldownModifier(FCombatModifier const& Modifier);
    UFUNCTION(BlueprintCallable)
    void RemoveChargesPerCooldownModifier(int32 const ModifierID);
private:
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultMaxCharges = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticMaxCharges = true;

    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultChargesPerCooldown = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticChargesPerCooldown = true;

    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    int32 DefaultChargeCost = 1;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticChargeCost = true;

    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    float DefaultCooldown = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticCooldownLength = true;

protected:
    UPROPERTY(ReplicatedUsing=OnRep_AbilityCooldown)
    FAbilityCooldown AbilityCooldown;
    UFUNCTION()
    virtual void OnRep_AbilityCooldown() { return; }
    FTimerHandle CooldownHandle;
    FAbilityChargeNotification OnChargesChanged;
    void StartCooldown();
    UFUNCTION()
    void CompleteCooldown();
    void CancelCooldown();
    bool ChargeCostMet = false;
    virtual void CheckChargeCostMet();
    virtual void AdjustCooldownFromMaxChargesChanged();
//Costs
public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cost")
    FDefaultAbilityCost GetDefaultAbilityCost(TSubclassOf<UResource> const ResourceClass) const;
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cost")
    float GetAbilityCost(TSubclassOf<UResource> const ResourceClass) const;
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cost")
    void GetDefaultAbilityCosts(TArray<FDefaultAbilityCost>& OutCosts) const { OutCosts = DefaultAbilityCosts; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cost")
    void GetAbilityCosts(TMap<TSubclassOf<UResource>, float>& OutCosts) const;
    UFUNCTION(BlueprintCallable)
    int32 AddCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier);
    UFUNCTION(BlueprintCallable)
    void RemoveCostModifier(TSubclassOf<UResource> const ResourceClass, int32 const ModifierID);
private:
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    TArray<FDefaultAbilityCost> DefaultAbilityCosts;
    UPROPERTY()
    TMap<TSubclassOf<UResource>, class UAbilityResourceCost*> AbilityCosts;
    FGenericCallback CostModsCallback;
    UFUNCTION()
    void ForceResourceCostRecalculations();
    UFUNCTION()
    void CheckResourceCostOnResourceChanged(UResource* Resource, UObject* ChangeSource, FResourceState const& PreviousState, FResourceState const& NewState);
    UFUNCTION()
    void CheckResourceCostOnCostUpdated(TSubclassOf<UResource> ResourceClass, float const NewCost);
    void CostUnmet(TSubclassOf<UResource> ResourceClass);
    void CostMet(TSubclassOf<UResource> ResourceClass);
    TSet<TSubclassOf<UResource>> UnmetCosts;
    bool ResourceCostsMet = false;
};
