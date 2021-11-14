// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageStructs.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BuffStructs.h"
#include "DamageHandler.generated.h"

class UStatHandler;
class UBuffHandler;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UDamageHandler : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UDamageHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	static FGameplayTag DamageDoneStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageDone")), false); }
	static FGameplayTag DamageTakenStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageTaken")), false); }
	static FGameplayTag HealingDoneStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingDone")), false); }
	static FGameplayTag HealingTakenStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingTaken")), false); }
	static FGameplayTag MaxHealthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxHealth")), false); }
	static FGameplayTag BuffFunctionDamageTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Damage")), false); }
	static FGameplayTag BuffFunctionHealingTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Healing")), false); }

private:
	
	UPROPERTY()
	UStatHandler* StatHandler;
	UPROPERTY()
	UBuffHandler* BuffHandler;

	//Health

private:
	
	UPROPERTY(EditAnywhere, Category = "Health")
	bool StaticMaxHealth = false;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float DefaultMaxHealth = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool HealthFollowsMaxHealth = true;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth = 0.0f;
	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth)
	float MaxHealth = 1.0f;
	UPROPERTY(ReplicatedUsing = OnRep_LifeStatus)
	ELifeStatus LifeStatus = ELifeStatus::Invalid;

	FHealthChangeNotification OnHealthChanged;
	FHealthChangeNotification OnMaxHealthChanged;
	FLifeStatusNotification OnLifeStatusChanged;
	TArray<FDamageRestriction> DeathConditions;

	void UpdateMaxHealth(float const NewMaxHealth);
	UFUNCTION()
	void ReactToMaxHealthStat(FGameplayTag const& StatTag, float const NewValue);
	bool CheckDeathRestricted(FDamageInfo const& DamageInfo);
	void Die();

	UFUNCTION()
	void OnRep_CurrentHealth(float PreviousValue);
	UFUNCTION()
	void OnRep_MaxHealth(float PreviousValue);
	UFUNCTION()
	void OnRep_LifeStatus(ELifeStatus PreviousValue);
	
public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	ELifeStatus GetLifeStatus() const { return LifeStatus; }

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

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void AddDeathRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void RemoveDeathRestriction(FDamageRestriction const& Restriction);

	//Outgoing Damage

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Damage")
	bool CanEverDealDamage() const { return bCanEverDealDamage; }
	
	void NotifyOfOutgoingDamageSuccess(FDamagingEvent const& DamageEvent);
    bool CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo);
    TArray<FCombatModifier> GetOutgoingDamageMods(FDamageInfo const& DamageInfo);

	UFUNCTION(BlueprintCallable, Category = "Damage")
    void SubscribeToOutgoingDamageSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
   	void UnsubscribeFromOutgoingDamageSuccess(FDamageEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
   	void AddOutgoingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
   	void RemoveOutgoingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
   	int32 AddOutgoingDamageModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
   	void RemoveOutgoingDamageModifier(int32 const ModifierID);
	
private:

	UFUNCTION()
	FCombatModifier ModifyDamageDoneFromStat(FDamageInfo const& DamageInfo);

	UPROPERTY(EditAnywhere, Category = "Damage")
	bool bCanEverDealDamage = true;
	
	FDamageEventNotification OnOutgoingDamage;
	TArray<FDamageRestriction> OutgoingDamageConditions;
	TMap<int32, FDamageModCondition> OutgoingDamageModifiers;

	UFUNCTION(NetMulticast, Unreliable)
    void MulticastNotifyOfOutgoingDamageSuccess(FDamagingEvent DamageEvent);

	UFUNCTION()
	bool RestrictDamageBuffs(FBuffApplyEvent const& BuffEvent);
    
    //Outgoing Healing

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Healing")
	bool CanEverDealHealing() const { return bCanEverDealHealing; }
	
	void NotifyOfOutgoingHealingSuccess(FHealingEvent const& HealingEvent);
    bool CheckOutgoingHealingRestricted(FHealingInfo const& HealingInfo);
   	TArray<FCombatModifier> GetOutgoingHealingMods(FHealingInfo const& HealingInfo);

	UFUNCTION(BlueprintCallable, Category = "Healing")
   	void SubscribeToOutgoingHealingSuccess(FHealingEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
   	void UnsubscribeFromOutgoingHealingSuccess(FHealingEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
   	void AddOutgoingHealingRestriction(FHealingRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
   	void RemoveOutgoingHealingRestriction(FHealingRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
   	int32 AddOutgoingHealingModifier(FHealingModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
   	void RemoveOutgoingHealingModifier(int32 const ModifierID);

private:

	UFUNCTION()
	FCombatModifier ModifyHealingDoneFromStat(FHealingInfo const& HealingInfo);
	
	UPROPERTY(EditAnywhere, Category = "Healing")
	bool bCanEverDealHealing = true;
	
    FHealingEventNotification OnOutgoingHealing;
    TArray<FHealingRestriction> OutgoingHealingConditions;
    TMap<int32, FHealingModCondition> OutgoingHealingModifiers;
    
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastNotifyOfOutgoingHealingSuccess(FHealingEvent HealingEvent);

	UFUNCTION()
	bool RestrictHealingBuffs(FBuffApplyEvent const& BuffEvent);

	//Incoming Damage

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Damage")
	bool CanEverReceiveDamage() const { return bCanEverReceiveDamage; }
	
	void ApplyDamage(FDamagingEvent& DamageEvent);
	bool CheckIncomingDamageRestricted(FDamageInfo const& DamageInfo);
	TArray<FCombatModifier> GetIncomingDamageMods(FDamageInfo const& DamageInfo);

	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SubscribeToIncomingDamageSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void UnsubscribeFromIncomingDamageSuccess(FDamageEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddIncomingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveIncomingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	int32 AddIncomingDamageModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveIncomingDamageModifier(int32 const ModifierID);

private:

	UFUNCTION()
	FCombatModifier ModifyDamageTakenFromStat(FDamageInfo const& DamageInfo);

	UPROPERTY(EditAnywhere, Category = "Damage")
	bool bCanEverReceiveDamage = true;
	
	FDamageEventNotification OnIncomingDamage;
	TArray<FDamageRestriction> IncomingDamageConditions;
	TMap<int32, FDamageModCondition> IncomingDamageModifiers;

	UFUNCTION(NetMulticast, Unreliable)
    void MulticastNotifyOfIncomingDamageSuccess(FDamagingEvent DamageEvent);

	//Incoming Healing

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Healing")
	bool CanEverReceiveHealing() const { return bCanEverReceiveHealing; }
	
	void ApplyHealing(FHealingEvent& HealingEvent);
	bool CheckIncomingHealingRestricted(FHealingInfo const& HealingInfo);
	TArray<FCombatModifier> GetIncomingHealingMods(FHealingInfo const& HealingInfo);

	UFUNCTION(BlueprintCallable, Category = "Healing")
	void SubscribeToIncomingHealingSuccess(FHealingEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void UnsubscribeFromIncomingHealingSuccess(FHealingEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddIncomingHealingRestriction(FHealingRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveIncomingHealingRestriction(FHealingRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	int32 AddIncomingHealingModifier(FHealingModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveIncomingHealingModifier(int32 const ModifierID);

private:

	UFUNCTION()
	FCombatModifier ModifyHealingTakenFromStat(FHealingInfo const& HealingInfo);

	UPROPERTY(EditAnywhere, Category = "Healing")
	bool bCanEverReceiveHealing = true;
	
	FHealingEventNotification OnIncomingHealing;
	TArray<FHealingRestriction> IncomingHealingConditions;
	TMap<int32, FHealingModCondition> IncomingHealingModifiers;

	UFUNCTION(NetMulticast, Unreliable)
    void MulticastNotifyOfIncomingHealingSuccess(FHealingEvent HealingEvent);

	//Replication

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		
};