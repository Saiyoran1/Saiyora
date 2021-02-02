// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "AbilityStructs.h"
#include "SaiyoraStructs.h"
#include "CombatAbility.generated.h"

struct FCombatModifier;
UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UCombatAbility : public UObject
{
	GENERATED_BODY()

public:

    static const FGameplayTag CooldownLengthStatTag;
    static const float MinimumCooldownLength;

    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

    UPROPERTY(EditDefaultsOnly, Category = "Display Info")
    FName Name;
    UPROPERTY(EditDefaultsOnly, Category = "Display Info")
    FText Description;
    UPROPERTY(EditDefaultsOnly, Category = "Display Info")
    UTexture2D* Icon;

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
    FGameplayTagContainer NonRestrictedCrowdControls;  //Crowd Controls that do not stop or prevent casting this ability.
    
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    bool bOnGlobalCooldown = true;
    UPROPERTY(EditDefaultsOnly, Category = "Cooldown Info")
    float GlobalCooldownLength = 1.0f;
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
    UPROPERTY(ReplicatedUsing = OnRep_MaxCharges)
    int32 MaxCharges = 1;
    int32 ChargesPerCooldown = 1;
    FTimerHandle CooldownHandle;
    UPROPERTY(ReplicatedUsing = OnRep_ChargesPerCast)
    int32 ChargesPerCast = 1;
    //This delegate exists mainly for UI display but could be used for some gameplay on the server. Never fires on simulated proxies, since cooldown information is only rep'd to owning client.
    FAbilityChargeNotification OnChargesChanged;
    void StartCooldown();
    void CompleteCooldown();
    TArray<FCombatModifier> CooldownModifiers;
    
    //Variable representing the bound custom cast conditions, evaluated on server and replicated.
    UPROPERTY(ReplicatedUsing = OnRep_CustomCastConditionsMet)
    bool bCustomCastConditionsMet;   

    //References
    UPROPERTY(ReplicatedUsing = OnRep_OwningComponent)
    class UAbilityComponent* OwningComponent;
    bool bInitialized = false;
    
    //OnReps
    UFUNCTION()
    void OnRep_OwningComponent();
    UFUNCTION()
    void OnRep_AbilityCooldown();
    UFUNCTION()
    void OnRep_MaxCharges();
    UFUNCTION()
    void OnRep_ChargesPerCast();
    UFUNCTION()
    void OnRep_CustomCastConditionsMet();

public:

    //Getters
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FName GetAbilityName() const { return Name; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FText GetAbilityDescription() const { return Description; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    UTexture2D* GetAbilityIcon() const { return Icon; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    EAbilityCastType GetCastType() const { return CastType; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetCastTime() const { return DefaultCastTime; } //TODO: Implement cast time calculation.
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetHasInitialTick() const { return bInitialTick; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetNumberOfTicks() const { return NonInitialTicks; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetInterruptible() const { return bInterruptible; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FGameplayTagContainer GetNonRestrictedCrowdControls() const { return NonRestrictedCrowdControls; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool CheckCustomCastConditionsMet() const { return bCustomCastConditionsMet; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool GetHasGlobalCooldown() const { return bOnGlobalCooldown; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetGlobalCooldownLength() const { return GlobalCooldownLength; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetMaxCharges() const { return MaxCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetCurrentCharges() const { return AbilityCooldown.CurrentCharges; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetCooldown() const; //TODO: Cooldown calculation.
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetChargeCost() const { return ChargesPerCast; }
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool CheckChargesMet() const { return AbilityCooldown.CurrentCharges >= ChargesPerCast; }
    void CommitCharges(int32 const CastID);
    
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetAbilityCost(FGameplayTag const& ResourceTag) const { return 0.0f; } //TODO: Implement Resource cost finding.
    UFUNCTION(BlueprintCallable, BlueprintPure)
    void GetAbilityCosts(TArray<FAbilityCost>& OutCosts) const;
    
    bool GetInitialized() const { return bInitialized; }

    //Internal ability functions, these adjust necessary properties then call the Blueprint implementations.
    
    void InitializeAbility(UAbilityComponent* AbilityComponent);
    void InitialTick();
    void NonInitialTick(int32 const TickNumber);
    void CompleteCast();
    void InterruptCast();
    void CancelCast();

protected:
    
    //Blueprint functions for defining the ability behavior.

    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialize();
    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialTick();
    UFUNCTION(BlueprintImplementableEvent)
    void OnNonInitialTick(int32 const TickNumber);
    UFUNCTION(BlueprintImplementableEvent)
    void OnCastComplete();
    UFUNCTION(BlueprintImplementableEvent)
    void OnCastInterrupted(); //TODO: Interrupt Event.
    UFUNCTION(BlueprintImplementableEvent)
    void OnCastCancelled();
};