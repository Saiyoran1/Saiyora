#pragma once
#include "CoreMinimal.h"

#include "BuffStructs.h"
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool HasHealth() const { return bHasHealth; }
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

private:

	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float DefaultMaxHealth = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool StaticMaxHealth = false;

	bool bHasHealth = true; //False if bCanEverReceiveDamage and bCanEverReceiveHealing are both false.
	UPROPERTY(ReplicatedUsing=OnRep_CurrentHealth)
	float CurrentHealth = 0.0f;
	UFUNCTION()
	void OnRep_CurrentHealth(float const PreviousHealth);
	FHealthChangeNotification OnHealthChanged;
	float MaxHealth = 1.0f;
	FHealthChangeNotification OnMaxHealthChanged;
	UPROPERTY(ReplicatedUsing=OnRep_LifeStatus)
	ELifeStatus LifeStatus = ELifeStatus::Invalid;
	UFUNCTION()
	void OnRep_LifeStatus(ELifeStatus const PreviousStatus);
	FLifeStatusNotification OnLifeStatusChanged;
	TArray<FDamageRestriction> DeathRestrictions;

	/*UFUNCTION()
	void OnMaxHealthStatChanged(FGameplayTag const& StatTag, float const NewValue);*/
	bool CheckDeathRestricted(FDamageInfo const& DamageInfo);
	void Die();

	//Incoming Damage

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Damage")
	bool CanEverReceiveDamage() const { return bCanEverReceiveDamage; }

	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SubscribeToIncomingDamageSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void UnsubscribeFromIncomingDamageSuccess(FDamageEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddIncomingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveIncomingDamageRestriction(FDamageRestriction const& Restriction);
	bool CheckIncomingDamageRestricted(FDamageInfo const& DamageInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddIncomingDamageModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveIncomingDamageModifier(FDamageModCondition const& Modifier);
	void GetIncomingDamageMods(FDamageInfo const& DamageInfo, TArray<FCombatModifier>& OutMods);

	void ApplyDamage(FDamagingEvent& DamageEvent);

private:

	UPROPERTY(EditAnywhere, Category = "Damage")
	bool bCanEverReceiveDamage = true;
	
	FDamageEventNotification OnIncomingDamage;
	TArray<FDamageRestriction> IncomingDamageRestrictions;
	TArray<FDamageModCondition> IncomingDamageModifiers;

	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfIncomingDamage(FDamagingEvent const& DamageEvent);

	//Incoming Healing

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Healing")
	bool CanEverReceiveHealing() const { return bCanEverReceiveHealing; }
	
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void SubscribeToIncomingHealingSuccess(FHealingEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void UnsubscribeFromIncomingHealingSuccess(FHealingEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddIncomingHealingRestriction(FHealingRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveIncomingHealingRestriction(FHealingRestriction const& Restriction);
	bool CheckIncomingHealingRestricted(FHealingInfo const& HealingInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddIncomingHealingModifier(FHealingModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveIncomingHealingModifier(FHealingModCondition const& Modifier);
	void GetIncomingHealingMods(FHealingInfo const& HealingInfo, TArray<FCombatModifier>& OutMods);

	void ApplyHealing(FHealingEvent& HealingEvent);

private:

	UPROPERTY(EditAnywhere, Category = "Healing")
	bool bCanEverReceiveHealing = true;
	
	FHealingEventNotification OnIncomingHealing;
	TArray<FHealingRestriction> IncomingHealingRestrictions;
	TArray<FHealingModCondition> IncomingHealingModifiers;

	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfIncomingHealing(FHealingEvent const& HealingEvent);

	//Outgoing Damage

public:

	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SubscribeToOutgoingDamageSuccess(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void UnsubscribeFromOutgoingDamageSuccess(FDamageEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddOutgoingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveOutgoingDamageRestriction(FDamageRestriction const& Restriction);
	bool CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void AddOutgoingDamageModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
	void RemoveOutgoingDamageModifier(FDamageModCondition const& Modifier);
	void GetOutgoingDamageMods(FDamageInfo const& DamageInfo, TArray<FCombatModifier>& OutMods);

	void NotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent);
	
private:
	
	FDamageEventNotification OnOutgoingDamage;
	TArray<FDamageRestriction> OutgoingDamageRestrictions;
	TArray<FDamageModCondition> OutgoingDamageModifiers;

	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent);

	/*UFUNCTION()
	bool RestrictDamageBuffs(FBuffApplyEvent const& BuffEvent);*/

	//Outgoing Healing

public:

	UFUNCTION(BlueprintCallable, Category = "Healing")
	void SubscribeToOutgoingHealingSuccess(FHealingEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void UnsubscribeFromOutgoingHealingSuccess(FHealingEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddOutgoingHealingRestriction(FHealingRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveOutgoingHealingRestriction(FHealingRestriction const& Restriction);
	bool CheckOutgoingHealingRestricted(FHealingInfo const& HealingInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddOutgoingHealingModifier(FHealingModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveOutgoingHealingModifier(FHealingModCondition const& Modifier);
	void GetOutgoingHealingMods(FHealingInfo const& HealingInfo, TArray<FCombatModifier>& OutMods);

	void NotifyOfOutgoingHealing(FHealingEvent const& HealingEvent);

private:
	
	FHealingEventNotification OnOutgoingHealing;
	TArray<FHealingRestriction> OutgoingHealingRestrictions;
	TArray<FHealingModCondition> OutgoingHealingModifiers;
    
	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfOutgoingHealing(FHealingEvent const& HealingEvent);

	/*UFUNCTION()
	bool RestrictHealingBuffs(FBuffApplyEvent const& BuffEvent);*/

};
