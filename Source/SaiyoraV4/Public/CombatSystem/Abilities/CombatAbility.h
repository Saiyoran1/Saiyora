#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatEnums.h"
#include "DamageEnums.h"
#include "AbilityStructs.h"
#include "ResourceStructs.h"
#include "CombatAbility.generated.h"

class UCrowdControl;
class UAbilityComponent;
struct FServerAbilityResult;

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
    
    UFUNCTION(BlueprintPure)
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
    void OnRep_Deactivated();
    
//Basic Info
    
public:
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
    FName GetAbilityName() const { return Name; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    FText GetAbilityDescription() const { return Description; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    UTexture2D* GetAbilityIcon() const { return Icon; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    EDamageSchool GetAbilitySchool() const { return School; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    ESaiyoraPlane GetAbilityPlane() const { return Plane; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetAbilityTags(FGameplayTagContainer& OutTags) const { OutTags = AbilityTags; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasTag(const FGameplayTag Tag) const { return AbilityTags.HasTag(Tag); }
    
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

    UFUNCTION(BlueprintPure, Category = "Abilities")
    ECastFailReason IsCastable() const { return Castable; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool IsCastableWhileDead() const { return bCastableWhileDead; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetRestrictedCrowdControls(FGameplayTagContainer& OutCrowdControls) const { OutCrowdControls = RestrictedCrowdControls; }
    
    void AddRestrictedTag(const FGameplayTag RestrictedTag);
    void RemoveRestrictedTag(const FGameplayTag RestrictedTag);
    
protected:
    
    UFUNCTION(BlueprintNativeEvent)
    void SetupCustomCastRestrictions();
    virtual void SetupCustomCastRestrictions_Implementation() {}
    UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (GameplayTagFilter = "AbilityRestriction"))
    void ActivateCastRestriction(const FGameplayTag RestrictionTag);
    UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (GameplayTagFilter = "AbilityRestriction"))
    void DeactivateCastRestriction(const FGameplayTag RestrictionTag);
    
private:
    
    ECastFailReason Castable = ECastFailReason::InvalidAbility;
    void UpdateCastable();
    UPROPERTY(EditDefaultsOnly, Category = "Restrictions")
    bool bCastableWhileDead = false;
    UPROPERTY(EditDefaultsOnly, Category = "Restrictions", meta = (Categories = "CrowdControl"))
    FGameplayTagContainer RestrictedCrowdControls;
    TSet<FGameplayTag> CustomCastRestrictions;
    TSet<FGameplayTag> RestrictedTags;
    UPROPERTY(ReplicatedUsing = OnRep_TagsRestricted)
    bool bTagsRestricted = false;
    UFUNCTION()
    void OnRep_TagsRestricted() { UpdateCastable(); }

//Cast

public:

    UFUNCTION(BlueprintPure, Category = "Abilities")
    EAbilityCastType GetCastType() const { return CastType; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetDefaultCastLength() const { return DefaultCastTime; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetCastLength();
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasStaticCastLength() const { return bStaticCastTime; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasInitialTick() const { return bInitialTick; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetNumberOfTicks() const { return NonInitialTicks; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
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
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasGlobalCooldown() const { return bOnGlobalCooldown; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetDefaultGlobalCooldownLength() const { return DefaultGlobalCooldownLength; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetGlobalCooldownLength();
    UFUNCTION(BlueprintPure, Category = "Abilities")
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
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetDefaultCooldownLength() const { return DefaultCooldownLength; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetCooldownLength();
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasStaticCooldownLength() const { return bStaticCooldownLength; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool IsCooldownActive() const;
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetRemainingCooldown() const;
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetCurrentCooldownLength() const;
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetDefaultMaxCharges() const { return DefaultMaxCharges; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetMaxCharges() const { return AbilityCooldown.MaxCharges; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasStaticMaxCharges() const { return bStaticMaxCharges; }
    
    void AddMaxChargeModifier(const FCombatModifier& Modifier);
    void RemoveMaxChargeModifier(const UBuff* Source);
    void RecalculateMaxCharges();

    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargeCost() const { return DefaultChargeCost; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetChargeCost() const { return ChargeCost; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasStaticChargeCost() const { return bStaticChargeCost; }
    
    void AddChargeCostModifier(const FCombatModifier& Modifier);
    void RemoveChargeCostModifier(const UBuff* Source);
    void RecalculateChargeCost();

    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargesPerCooldown() const { return DefaultChargesPerCooldown; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetChargesPerCooldown() const { return ChargesPerCooldown; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasStaticChargesPerCooldown() const { return bStaticChargesPerCooldown; }
    
    void AddChargesPerCooldownModifier(const FCombatModifier& Modifier);
    void RemoveChargesPerCooldownModifier(const UBuff* Source);
    void RecalculateChargesPerCooldown();
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetCurrentCharges() const;
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void ModifyCurrentCharges(const int32 Charges, const EChargeModificationType Modification = EChargeModificationType::Additive);
    UPROPERTY(BlueprintAssignable)
    FAbilityChargeNotification OnChargesChanged;
    
    void CommitCharges(const int32 PredictionID = 0);
    
private:

    UPROPERTY(ReplicatedUsing = OnRep_AbilityCooldown)
    FAbilityCooldown AbilityCooldown;
    UFUNCTION()
    void OnRep_AbilityCooldown(const FAbilityCooldown& PreviousState);
    FTimerHandle CooldownHandle;
    void StartCooldown(const bool bUseLagCompensation = false);
    UFUNCTION()
    void CompleteCooldown();
    void CancelCooldown();
    FAbilityCooldown ClientCooldown;
    TMap<int32, int32> ChargePredictions;
    void RecalculatePredictedCooldown();

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
    UPROPERTY(ReplicatedUsing = OnRep_ChargeCost)
    int32 ChargeCost = 1;
    UFUNCTION()
    void OnRep_ChargeCost();
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

    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetDefaultAbilityCosts(TArray<FDefaultAbilityCost>& OutCosts) const { OutCosts = DefaultAbilityCosts; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetAbilityCosts(TArray<FAbilityCost>& OutCosts) const { OutCosts = AbilityCosts.Items; }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void AddResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifier& Modifier);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void RemoveResourceCostModifier(const TSubclassOf<UResource> ResourceClass, UBuff* Source);
    
    void UpdateCostFromReplication(const FAbilityCost& Cost, const bool bNewAdd = false);
    
private:
    
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    TArray<FDefaultAbilityCost> DefaultAbilityCosts;
    UPROPERTY(Replicated)
    FAbilityCostArray AbilityCosts;
    TMultiMap<TSubclassOf<UResource>, FCombatModifier> ResourceCostModifiers;
    UFUNCTION()
    void CheckResourceCostOnResourceChanged(UResource* Resource, UObject* ChangeSource, const FResourceState& PreviousState, const FResourceState& NewState);
    UFUNCTION()
    void SetupCostCheckingForNewResource(UResource* Resource);
    void UpdateCost(const TSubclassOf<UResource> ResourceClass);
    void CheckCostMet(const TSubclassOf<UResource> ResourceClass);
    TSet<TSubclassOf<UResource>> UnmetCosts;
    TSet<TSubclassOf<UResource>> UninitializedResources;

//Functionality

public:

    int32 GetPredictionID() const { return CurrentPredictionID; }
    int32 GetCurrentTick() const { return CurrentTick; }
    void PredictedTick(const int32 TickNumber, FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, const int32 PredictionID = 0);
    void ServerTick(const int32 TickNumber, FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, const int32 PredictionID = 0);
    void SimulatedTick(const int32 TickNumber, const FAbilityOrigin& Origin, const TArray<FAbilityTargetSet>& Targets);
    void PredictedCancel(FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, const int32 PredictionID = 0);
    void ServerCancel(FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, const int32 PredictionID = 0);
    void SimulatedCancel(const FAbilityOrigin& Origin, const TArray<FAbilityTargetSet>& Targets);
    void ServerInterrupt(const FInterruptEvent& InterruptEvent);
    void SimulatedInterrupt(const FInterruptEvent& InterruptEvent);
    void UpdatePredictionFromServer(const FServerAbilityResult& Result);
    
protected:

    UFUNCTION(BlueprintNativeEvent)
    void OnPredictedTick(const int32 TickNumber);
    virtual void OnPredictedTick_Implementation(const int32 TickNumber) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnServerTick(const int32 TickNumber);
    virtual void OnServerTick_Implementation(const int32 TickNumber) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnSimulatedTick(const int32 TickNumber);
    virtual void OnSimulatedTick_Implementation(const int32 TickNumber) {}
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
    void OnServerInterrupt(const FInterruptEvent& InterruptEvent);
    virtual void OnServerInterrupt_Implementation(const FInterruptEvent& InterruptEvent) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnSimulatedInterrupt(const FInterruptEvent& InterruptEvent);
    virtual void OnSimulatedInterrupt_Implementation(const FInterruptEvent& InterruptEvent) {}
    UFUNCTION(BlueprintNativeEvent)
    void OnMisprediction(const int32 PredictionID, const ECastFailReason FailReason);
    virtual void OnMisprediction_Implementation(const int32 PredictionID, const ECastFailReason FailReason) {}

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddTargetSet(const FAbilityTargetSet& TargetSet) { CurrentTargets.Add(TargetSet); }
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddTargetsAsSet(const TArray<AActor*>& Targets, const int32 SetID) { CurrentTargets.Add(FAbilityTargetSet(SetID, Targets)); }
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddTarget(AActor* Target, const int32 SetID);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void RemoveTarget(AActor* Target);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void RemoveTargetFromAllSets(AActor* Target);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void RemoveTargetSet(const int32 SetID);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void RemoveTargetFromSet(AActor* Target, const int32 SetID);
    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetTargets(TArray<FAbilityTargetSet>& OutTargets) const { OutTargets = CurrentTargets; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool GetTargetSetByID(const int32 SetID, FAbilityTargetSet& OutTargetSet) const;

    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetAbilityOrigin(FAbilityOrigin& OutOrigin) const { OutOrigin = AbilityOrigin; }
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SetAbilityOrigin(const FAbilityOrigin& Origin) { AbilityOrigin = Origin; }
    
private:

    int32 CurrentPredictionID = 0;
    int32 CurrentTick = 0;
    TArray<FAbilityTargetSet> CurrentTargets;
    FAbilityOrigin AbilityOrigin;
};