#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatEnums.h"
#include "DamageEnums.h"
#include "AbilityStructs.h"
#include "ResourceStructs.h"
#include "GameFramework/GameStateBase.h"
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
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
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
    EHealthEventSchool GetAbilitySchool() const { return School; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    ESaiyoraPlane GetAbilityPlane() const { return Plane; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetAbilityTags(FGameplayTagContainer& OutTags) const { OutTags = AbilityTags; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasTag(const FGameplayTag Tag) const { return AbilityTags.HasTag(Tag); }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool IsAutomatic() const { return bAutomatic; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool WillCancelOnRelease() const { return bCancelOnRelease; }
    
protected:
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
    FName Name;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
    FText Description;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
    UTexture2D* Icon;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
    EHealthEventSchool School;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
    ESaiyoraPlane Plane;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    FGameplayTagContainer AbilityTags;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    bool bAutomatic = false;
    UPROPERTY(EditDefaultsOnly, Category = "Info")
    bool bCancelOnRelease = false;
    
//Restrictions
    
public:

    UFUNCTION(BlueprintPure, Category = "Abilities")
    ECastFailReason IsCastable() const { return Castable; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool IsCastableWhileDead() const { return bCastableWhileDead; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetRestrictedCrowdControls(FGameplayTagContainer& OutCrowdControls) const { OutCrowdControls = RestrictedCrowdControls; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool IsCastableWhileMoving() const { return bCastableWhileMoving; }
    
    void AddRestrictedTag(const FGameplayTag RestrictedTag);
    void RemoveRestrictedTag(const FGameplayTag RestrictedTag);
    
protected:
    
    UFUNCTION(BlueprintNativeEvent)
    void SetupCustomCastRestrictions();
    virtual void SetupCustomCastRestrictions_Implementation() {}
    UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (GameplayTagFilter = "Ability.Restriction"))
    void ActivateCastRestriction(const FGameplayTag RestrictionTag);
    UFUNCTION(BlueprintCallable, Category = "Abilities", meta = (GameplayTagFilter = "Ability.Restriction"))
    void DeactivateCastRestriction(const FGameplayTag RestrictionTag);

    UPROPERTY(EditDefaultsOnly, Category = "Restrictions")
    bool bCastableWhileDead = false;
    UPROPERTY(EditDefaultsOnly, Category = "Restrictions", meta = (Categories = "CrowdControl"))
    FGameplayTagContainer RestrictedCrowdControls;
    UPROPERTY(EditDefaultsOnly, Category = "Restrictions")
    bool bCastableWhileMoving = true;
    
private:
    
    ECastFailReason Castable = ECastFailReason::InvalidAbility;
    void UpdateCastable();
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
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool CanCastWhileCasting() const { return bCastableWhileCasting; }

protected:

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
    UPROPERTY(EditDefaultsOnly, Category = "Cast")
    bool bCastableWhileCasting = false;
 
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
    
protected:
    
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
    bool IsCooldownActive() const { return AbilityCooldown.OnCooldown; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool IsCooldownAcked() const { return AbilityCooldown.bAcked; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetRemainingCooldown() const { return AbilityCooldown.OnCooldown && AbilityCooldown.bAcked ? FMath::Max(0.0f, AbilityCooldown.CooldownEndTime - GetWorld()->GetGameState()->GetServerWorldTimeSeconds()) : -1.0f; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetCurrentCooldownLength() const { return AbilityCooldown.OnCooldown && AbilityCooldown.bAcked ? FMath::Max(0.0f, AbilityCooldown.CooldownEndTime - AbilityCooldown.CooldownStartTime) : -1.0f; }
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetDefaultMaxCharges() const { return MaxCharges.GetDefaultValue(); }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetMaxCharges() const { return AbilityCooldown.MaxCharges; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasStaticMaxCharges() const { return !MaxCharges.IsModifiable(); }

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    FCombatModifierHandle AddMaxChargeModifier(const FCombatModifier& Modifier);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void RemoveMaxChargeModifier(const FCombatModifierHandle& ModifierHandle);
    void UpdateMaxChargeModifier(const FCombatModifierHandle& ModifierHandle, const FCombatModifier& NewModifier);

    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargeCost() const { return ChargeCost.GetDefaultValue(); }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetChargeCost() const { return ChargeCost.GetCurrentValue(); }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasStaticChargeCost() const { return !ChargeCost.IsModifiable(); }

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    FCombatModifierHandle AddChargeCostModifier(const FCombatModifier& Modifier);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void RemoveChargeCostModifier(const FCombatModifierHandle& ModifierHandle);
    void UpdateChargeCostModifier(const FCombatModifierHandle& ModifierHandle, const FCombatModifier& NewModifier);

    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetDefaultChargesPerCooldown() const { return ChargesPerCooldown.GetDefaultValue(); }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetChargesPerCooldown() const { return ChargesPerCooldown.GetCurrentValue(); }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    bool HasStaticChargesPerCooldown() const { return !ChargesPerCooldown.IsModifiable(); }

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    FCombatModifierHandle AddChargesPerCooldownModifier(const FCombatModifier& Modifier);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void RemoveChargesPerCooldownModifier(const FCombatModifierHandle& ModifierHandle);
    void UpdateChargesPerCooldownModifier(const FCombatModifierHandle& ModifierHandle, const FCombatModifier& NewModifier);
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
    int32 GetCurrentCharges() const { return AbilityCooldown.CurrentCharges; }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void ModifyCurrentCharges(const int32 Charges, const EChargeModificationType Modification = EChargeModificationType::Additive);
    UPROPERTY(BlueprintAssignable)
    FAbilityChargeNotification OnChargesChanged;
    
    void CommitCharges(const int32 PredictionID = 0);

protected:

    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    float DefaultCooldownLength = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    bool bStaticCooldownLength = true;
    
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
    FModifiableInt MaxCharges = FModifiableInt(1, true);
    UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_ChargeCost, Category = "Cooldown")
    FModifiableInt ChargeCost = FModifiableInt(1, true);
    UPROPERTY(EditDefaultsOnly, Replicated, Category = "Cooldown")
    FModifiableInt ChargesPerCooldown = FModifiableInt(1, true);
    
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
    TMap<int32, int32> ChargePredictions;
    int32 LastReplicatedCharges;
    void RecalculatePredictedCooldown();
    
    void OnMaxChargesUpdated(const int32 OldValue, const int32 NewValue);
    void OnChargeCostUpdated(const int32 OldValue, const int32 NewValue) { UpdateCastable(); }
    UFUNCTION()
    void OnRep_ChargeCost() { UpdateCastable(); }
    
//Costs
    
public:
    
    UFUNCTION(BlueprintPure, Category = "Abilities")
    void GetAbilityCosts(TArray<FSimpleAbilityCost>& OutCosts) const;
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    FCombatModifierHandle AddResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifier& Modifier);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
    void RemoveResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifierHandle& ModifierHandle);
    void UpdateResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifierHandle& ModifierHandle, const FCombatModifier& Modifier);
    
    void UpdateCostFromReplication(const FAbilityCost& Cost, const bool bNewAdd = false);
    
private:

    UPROPERTY(Replicated, EditDefaultsOnly, Category = "Cost")
    FAbilityCostArray AbilityCosts;
    void SetupResourceCosts();
    UFUNCTION()
    void OnResourceValueChanged(UResource* Resource, UObject* ChangeSource, const FResourceState& PreviousState, const FResourceState& NewState);
    void OnResourceCostChanged(FAbilityCost& AbilityCost);
    UFUNCTION()
    void SetupCostCheckingForNewResource(UResource* Resource);
    float GetResourceCost(const TSubclassOf<UResource> ResourceClass) const;
    float GetResourceValue(const TSubclassOf<UResource> ResourceClass) const;
    void UpdateCostMet(const TSubclassOf<UResource> ResourceClass, const bool bMet);
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