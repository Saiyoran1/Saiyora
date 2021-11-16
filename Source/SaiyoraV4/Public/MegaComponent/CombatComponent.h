﻿#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "DamageStructs.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
private:
	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;

#pragma region Damage and Healing
	
	/*
	 *
	 * DAMAGE AND HEALING
	 *
	 */
	
public:
	static FGameplayTag DamageDoneStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageDone")), false); }
	static FGameplayTag DamageTakenStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageTaken")), false); }
	static FGameplayTag HealingDoneStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingDone")), false); }
	static FGameplayTag HealingTakenStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingTaken")), false); }
	static FGameplayTag MaxHealthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxHealth")), false); }
	static FGameplayTag BuffFunctionDamageTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Damage")), false); }
	static FGameplayTag BuffFunctionHealingTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Healing")), false); }

	void ApplyDamage(FDamagingEvent& DamageEvent);
	void NotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent);
	void ApplyHealing(FDamagingEvent& HealingEvent);
	void NotifyOfOutgoingHealing(FDamagingEvent const& HealingEvent);
private:
	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfIncomingDamage(FDamagingEvent const& DamageEvent);
	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent);
	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfIncomingHealing(FDamagingEvent const& HealingEvent);
	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfOutgoingHealing(FDamagingEvent const& HealingEvent);
	/*UFUNCTION()
	bool RestrictDamageBuffs(FBuffApplyEvent const& BuffEvent);*/
	/*UFUNCTION()
	bool RestrictHealingBuffs(FBuffApplyEvent const& BuffEvent);*/

#pragma region Subscriptions
	
public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SubscribeToHealthChanged(FHealthChangeCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UnsubscribeFromHealthChanged(FHealthChangeCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SubscribeToMaxHealthChanged(FHealthChangeCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UnsubscribeFromMaxHealthChanged(FHealthChangeCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SubscribeToLifeStatusChanged(FLifeStatusCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UnsubscribeFromLifeStatusChanged(FLifeStatusCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SubscribeToIncomingDamageSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void UnsubscribeFromIncomingDamageSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void SubscribeToIncomingHealingSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void UnsubscribeFromIncomingHealingSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SubscribeToOutgoingDamageSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void UnsubscribeFromOutgoingDamageSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void SubscribeToOutgoingHealingSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void UnsubscribeFromOutgoingHealingSuccess(FDamageEventCallback const& Callback);
private:
	FHealthChangeNotification OnHealthChanged;
	FHealthChangeNotification OnMaxHealthChanged;
	FLifeStatusNotification OnLifeStatusChanged;
	FDamageEventNotification OnIncomingDamage;
	FDamageEventNotification OnIncomingHealing;
	FDamageEventNotification OnOutgoingDamage;
	FDamageEventNotification OnOutgoingHealing;
	
#pragma endregion
#pragma region Restrictions

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Damage")
	bool CanEverReceiveDamage() const { return bCanEverReceiveDamage; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Healing")
	bool CanEverReceiveHealing() const { return bCanEverReceiveHealing; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void AddDeathRestriction(FDeathRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void RemoveDeathRestriction(FDeathRestriction const& Restriction);
	bool CheckDeathRestricted(FDamagingEvent const& DamageEvent);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddIncomingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveIncomingDamageRestriction(FDamageRestriction const& Restriction);
	bool CheckIncomingDamageRestricted(FDamageInfo const& DamageInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddIncomingHealingRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveIncomingHealingRestriction(FDamageRestriction const& Restriction);
	bool CheckIncomingHealingRestricted(FDamageInfo const& HealingInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddOutgoingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveOutgoingDamageRestriction(FDamageRestriction const& Restriction);
	bool CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddOutgoingHealingRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveOutgoingHealingRestriction(FDamageRestriction const& Restriction);
	bool CheckOutgoingHealingRestricted(FDamageInfo const& HealingInfo);
private:
	bool bHasHealth = true;
	UPROPERTY(EditAnywhere, Category = "Damage")
	bool bCanEverReceiveDamage = true;
	UPROPERTY(EditAnywhere, Category = "Healing")
	bool bCanEverReceiveHealing = true;
	TArray<FDeathRestriction> DeathRestrictions;
	TArray<FDamageRestriction> IncomingDamageRestrictions;
	TArray<FDamageRestriction> IncomingHealingRestrictions;
	TArray<FDamageRestriction> OutgoingDamageRestrictions;	
	TArray<FDamageRestriction> OutgoingHealingRestrictions;
	
#pragma endregion
#pragma region Modifiers

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddIncomingDamageModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveIncomingDamageModifier(FDamageModCondition const& Modifier);
	void GetIncomingDamageMods(FDamageInfo const& DamageInfo, TArray<FCombatModifier>& OutMods);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddIncomingHealingModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveIncomingHealingModifier(FDamageModCondition const& Modifier);
	void GetIncomingHealingMods(FDamageInfo const& HealingInfo, TArray<FCombatModifier>& OutMods);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddOutgoingDamageModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveOutgoingDamageModifier(FDamageModCondition const& Modifier);
	void GetOutgoingDamageMods(FDamageInfo const& DamageInfo, TArray<FCombatModifier>& OutMods);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddOutgoingHealingModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveOutgoingHealingModifier(FDamageModCondition const& Modifier);
	void GetOutgoingHealingMods(FDamageInfo const& HealingInfo, TArray<FCombatModifier>& OutMods);
private:
	TArray<FDamageModCondition> IncomingDamageModifiers;
	TArray<FDamageModCondition> IncomingHealingModifiers;
	TArray<FDamageModCondition> OutgoingDamageModifiers;
	TArray<FDamageModCondition> OutgoingHealingModifiers;
	
#pragma endregion
#pragma region Health

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool HasHealth() const { return bHasHealth; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	ELifeStatus GetLifeStatus() const { return LifeStatus; }
private:
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float DefaultMaxHealth = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool StaticMaxHealth = false;
	UPROPERTY(ReplicatedUsing=OnRep_CurrentHealth)
	float CurrentHealth = 0.0f;
	UFUNCTION()
	void OnRep_CurrentHealth(float const PreviousHealth);
	float MaxHealth = 1.0f;
	/*UFUNCTION()
	void OnMaxHealthStatChanged(FGameplayTag const& StatTag, float const NewValue);*/
	UPROPERTY(ReplicatedUsing=OnRep_LifeStatus)
	ELifeStatus LifeStatus = ELifeStatus::Invalid;
	UFUNCTION()
	void OnRep_LifeStatus(ELifeStatus const PreviousStatus);
	void Die();
	
#pragma endregion 
#pragma endregion
	
#pragma region Threat
	
	/*
	 *
	 * THREAT
	 *
	 */
	
public:
	static FGameplayTag GenericThreatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat")), false); }
	static float GlobalHealingThreatModifier;
	static float GlobalTauntThreatPercentage;
	static float GlobalThreatDecayPercentage;
	static float GlobalThreatDecayInterval;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool IsInCombat() const { return bInCombat; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	AActor* GetCurrentTarget() const { return CurrentTarget; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	FThreatEvent AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
			UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier);
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	float GetThreatLevel(AActor* Target) const;
	void NotifyAddedToThreatTable(AActor* Actor);
	void NotifyRemovedFromThreatTable(AActor* Actor);
private:
	UPROPERTY(ReplicatedUsing=OnRep_bInCombat)
	bool bInCombat = false;
	UFUNCTION()
	void OnRep_bInCombat();
	void UpdateCombatStatus();
	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget)
    AActor* CurrentTarget = nullptr;
    UFUNCTION()
    void OnRep_CurrentTarget(AActor* PreviousTarget);
	void UpdateTarget();
	UPROPERTY()
	TArray<FThreatTarget> ThreatTable;
	void AddToThreatTable(AActor* Target, float const InitialThreat = 0.0f, bool const bFaded = false, UBuff* InitialFixate = nullptr, UBuff* InitialBlind = nullptr);
	void RemoveFromThreatTable(AActor* Target);
	void RemoveThreat(AActor* Target, float const Amount);
	FTimerDelegate DecayDelegate;
	UFUNCTION()
	void DecayThreat();
	FTimerHandle DecayHandle;
	UPROPERTY()
	TArray<AActor*> TargetedBy;
	/*UFUNCTION()
	bool CheckBuffForThreat(FBuffApplyEvent const& BuffEvent);
	FBuffEventCondition ThreatBuffRestriction;*/

#pragma region Threat Special Events

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Taunt(AActor* AppliedBy);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void DropThreat(AActor* Target, float const Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void TransferThreat(AActor* FromActor, AActor* ToActor, float const Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Vanish();
	void NotifyOfTargetVanish(AActor* Actor) { RemoveFromThreatTable(Actor); }
	void AddFixate(AActor* Target, UBuff* Source);
    void RemoveFixate(AActor* Target, UBuff* Source);
    void AddBlind(AActor* Target, UBuff* Source);
    void RemoveBlind(AActor* Target, UBuff* Source);
    void AddFade(UBuff* Source);
    void RemoveFade(UBuff* Source);
	bool HasActiveFade() const { return Fades.Num() != 0; }
	void NotifyOfTargetFadeStatusChange(AActor* Actor, bool const bFaded);
    void AddMisdirect(UBuff* Source, AActor* Target);
    void RemoveMisdirect(UBuff* Source);
	AActor* GetMisdirectTarget() const { return Misdirects.Num() == 0 ? nullptr : Misdirects[Misdirects.Num() - 1].TargetActor; }
private:
	UPROPERTY()
	TArray<UBuff*> Fades;
	UPROPERTY()
    TArray<FMisdirect> Misdirects;

#pragma endregion
#pragma region Subscriptions

public:
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void SubscribeToTargetChanged(FTargetCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void UnsubscribeFromTargetChanged(FTargetCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void SubscribeToCombatStatusChanged(FCombatStatusCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void UnsubscribeFromCombatStatusChanged(FCombatStatusCallback const& Callback);
private:
	FTargetNotification OnTargetChanged;
	FCombatStatusNotification OnCombatStatusChanged;
	
#pragma endregion
#pragma region Restrictions

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool CanEverReceiveThreat() const { return bCanEverReceiveThreat; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddIncomingThreatRestriction(FThreatRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveIncomingThreatRestriction(FThreatRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddOutgoingThreatRestriction(FThreatRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveOutgoingThreatRestriction(FThreatRestriction const& Restriction);
private:
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanEverReceiveThreat = true;
	TArray<FThreatRestriction> IncomingThreatRestrictions;
	bool CheckIncomingThreatRestricted(FThreatEvent const& ThreatEvent);
	TArray<FThreatRestriction> OutgoingThreatRestrictions;
	bool CheckOutgoingThreatRestricted(FThreatEvent const& ThreatEvent);
	
#pragma endregion 
#pragma region Modifiers

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddIncomingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveIncomingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddOutgoingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveOutgoingThreatModifier(FThreatModCondition const& Modifier);
private:
	TArray<FThreatModCondition> IncomingThreatMods;
	void GetIncomingThreatMods(FThreatEvent const& ThreatEvent, TArray<FCombatModifier>& OutMods);
	TArray<FThreatModCondition> OutgoingThreatMods;
	void GetOutgoingThreatMods(FThreatEvent const& ThreatEvent, TArray<FCombatModifier>& OutMods);
	
#pragma endregion 
	
#pragma endregion
	
};


