#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "UObject/NoExportTypes.h"
#include "DamageEnums.h"
#include "ResourceStructs.h"
#include "Resource.h"
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
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    class ASaiyoraGameMode* GetGameModeRef() const { return GameModeRef; }
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
    UPROPERTY()
    class ASaiyoraGameMode* GameModeRef;
    
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
    void GetAbilityTags(FGameplayTagContainer& OutTags) const { OutTags = AbilityTags; }
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
    void GetRestrictedCrowdControls(FGameplayTagContainer& OutCrowdControls) const { OutCrowdControls = RestrictedCrowdControls; }
    void AddRestrictedTag(FGameplayTag const RestrictedTag);
    void RemoveRestrictedTag(FGameplayTag const RestrictedTag);
    
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
    UPROPERTY(EditDefaultsOnly, Category = "Restrictions", meta = (GameplayTagFilter = "CrowdControl"))
    FGameplayTagContainer RestrictedCrowdControls;
    TSet<FGameplayTag> CustomCastRestrictions;
    TSet<FGameplayTag> RestrictedTags;
    UPROPERTY(ReplicatedUsing=OnRep_TagsRestricted)
    bool bTagsRestricted = false;
    UFUNCTION()
    void OnRep_TagsRestricted() { UpdateCastable(); }

//Cast

public:

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    EAbilityCastType GetCastType() const { return CastType; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetDefaultCastLength() const { return DefaultCastTime; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    float GetCastLength();
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
    float GetGlobalCooldownLength();
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
    float GetCooldownLength();
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
    void AddMaxChargeModifier(FCombatModifier const& Modifier);
    void RemoveMaxChargeModifier(UBuff* Source);
    void RecalculateMaxCharges();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargeCost() const { return DefaultChargeCost; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetChargeCost() const { return ChargeCost; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticChargeCost() const { return bStaticChargeCost; }
    void AddChargeCostModifier(FCombatModifier const& Modifier);
    void RemoveChargeCostModifier(UBuff* Source);
    void RecalculateChargeCost();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargesPerCooldown() const { return DefaultChargesPerCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetChargesPerCooldown() const { return ChargesPerCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool HasStaticChargesPerCooldown() const { return bStaticChargesPerCooldown; }
    void AddChargesPerCooldownModifier(FCombatModifier const& Modifier);
    void RemoveChargesPerCooldownModifier(UBuff* Source);
    void RecalculateChargesPerCooldown();
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    int32 GetCurrentCharges() const;
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

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    void GetDefaultAbilityCosts(TArray<FDefaultAbilityCost>& OutCosts) const { OutCosts = DefaultAbilityCosts; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    void GetAbilityCosts(TArray<FAbilityCost>& OutCosts) const { OutCosts = AbilityCosts.Items; }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void AddResourceCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void RemoveResourceCostModifier(TSubclassOf<UResource> const ResourceClass, UBuff* Source);
    void UpdateCostFromReplication(FAbilityCost const& Cost, bool const bNewAdd = false);
    
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
    int32 GetCurrentTick() const { return CurrentTick; }
    void PredictedTick(int32 const TickNumber, FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, int32 const PredictionID = 0);
    void ServerTick(int32 const TickNumber, FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, int32 const PredictionID = 0);
    void SimulatedTick(int32 const TickNumber, FAbilityOrigin const& Origin, TArray<FAbilityTargetSet> const& Targets);
    void PredictedCancel(FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, int32 const PredictionID = 0);
    void ServerCancel(FAbilityOrigin& Origin, TArray<FAbilityTargetSet>& Targets, int32 const PredictionID = 0);
    void SimulatedCancel(FAbilityOrigin const& Origin, TArray<FAbilityTargetSet> const& Targets);
    void ServerInterrupt(FInterruptEvent const& InterruptEvent);
    void SimulatedInterrupt(FInterruptEvent const& InterruptEvent);
    void UpdatePredictionFromServer(struct FServerAbilityResult const& Result);
    
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
    void AddTargetSet(FAbilityTargetSet const& TargetSet) { CurrentTargets.Add(TargetSet); }
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddTargetsAsSet(TArray<AActor*> const& Targets, int32 const SetID) { CurrentTargets.Add(FAbilityTargetSet(SetID, Targets)); }
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddTarget(AActor* Target, int32 const SetID);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void RemoveTarget(AActor* Target);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void RemoveTargetFromAllSets(AActor* Target);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void RemoveTargetSet(int32 const SetID);
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void RemoveTargetFromSet(AActor* Target, int32 const SetID);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    void GetTargets(TArray<FAbilityTargetSet>& OutTargets) const { OutTargets.Empty(); OutTargets = CurrentTargets; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    bool GetTargetSetByID(int32 const ID, FAbilityTargetSet& OutTargetSet) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
    void GetAbilityOrigin(FAbilityOrigin& OutOrigin) const { OutOrigin.Clear(); OutOrigin = AbilityOrigin; }
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void SetAbilityOrigin(FAbilityOrigin const& Origin) { AbilityOrigin = Origin; }
    
private:

    int32 CurrentPredictionID = 0;
    int32 CurrentTick = 0;
    TArray<FAbilityTargetSet> CurrentTargets;
    FAbilityOrigin AbilityOrigin;
};