#pragma once
#include "CoreMinimal.h"
#include "DamageStructs.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BuffStructs.h"
#include "StatStructs.h"

#include "DamageHandler.generated.h"

class UStatHandler;
class UBuffHandler;
class UPlaneComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UDamageHandler : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UDamageHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static FGameplayTag DamageDoneStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageDone")), false); }
	static FGameplayTag DamageTakenStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.DamageTaken")), false); }
	static FGameplayTag HealingDoneStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingDone")), false); }
	static FGameplayTag HealingTakenStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.HealingTaken")), false); }
	static FGameplayTag MaxHealthStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.MaxHealth")), false); }
	static FGameplayTag BuffFunctionDamageTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Damage")), false); }
	static FGameplayTag BuffFunctionHealingTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Healing")), false); }

private:
	
	UPROPERTY()
	UStatHandler* StatHandler = nullptr;
	UPROPERTY()
	UBuffHandler* BuffHandler = nullptr;
	UPROPERTY()
	UPlaneComponent* PlaneComponent = nullptr;
	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;

//Health
	
public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	ELifeStatus GetLifeStatus() const { return LifeStatus; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool HasHealth() const { return bHasHealth; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool IsDead() const { return bHasHealth && LifeStatus != ELifeStatus::Alive; }

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
	void AddDeathRestriction(FDeathRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void RemoveDeathRestriction(FDeathRestriction const& Restriction);

private:

	UPROPERTY(EditAnywhere, Category = "Health")
	bool bHasHealth = true;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bStaticMaxHealth = false;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float DefaultMaxHealth = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth = 0.0f;
	UFUNCTION()
	void OnRep_CurrentHealth(float PreviousValue);
	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth)
	float MaxHealth = 1.0f;
	UFUNCTION()
	void OnRep_MaxHealth(float PreviousValue);
	UPROPERTY(ReplicatedUsing = OnRep_LifeStatus)
	ELifeStatus LifeStatus = ELifeStatus::Invalid;
	UFUNCTION()
	void OnRep_LifeStatus(ELifeStatus PreviousValue);

	FHealthChangeNotification OnHealthChanged;
	FHealthChangeNotification OnMaxHealthChanged;
	FLifeStatusNotification OnLifeStatusChanged;
	TArray<FDeathRestriction> DeathRestrictions;

	void UpdateMaxHealth(float const NewMaxHealth);
	FStatCallback MaxHealthStatCallback;
	UFUNCTION()
	void ReactToMaxHealthStat(FGameplayTag const& StatTag, float const NewValue, int32 const PredictionID);
	bool CheckDeathRestricted(FDamagingEvent const& DamageEvent);
	void Die();

//Outgoing Damage

public:

	UFUNCTION(BlueprintCallable, Category = "Damage")
    void SubscribeToOutgoingDamage(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
   	void UnsubscribeFromOutgoingDamage(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SubscribeToKillingBlow(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void UnsubscribeFromKillingBlow(FDamageEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
   	void AddOutgoingDamageRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
   	void RemoveOutgoingDamageRestriction(FDamageRestriction const& Restriction);
	bool CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
    void AddOutgoingDamageModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage")
   	void RemoveOutgoingDamageModifier(FDamageModCondition const& Modifier);
	float GetModifiedOutgoingDamage(FDamageInfo const& DamageInfo, FDamageModCondition const& SourceMod) const;

	void NotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent);
	
private:
	
	FDamageEventNotification OnOutgoingDamage;
	FDamageEventNotification OnKillingBlow;
	TArray<FDamageRestriction> OutgoingDamageRestrictions;
	TArray<FDamageModCondition> OutgoingDamageModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent);

	FDamageModCondition DamageDoneModFromStat;
	UFUNCTION()
	FCombatModifier ModifyDamageDoneFromStat(FDamageInfo const& DamageInfo);

//Outgoing Healing

public:

	UFUNCTION(BlueprintCallable, Category = "Healing")
   	void SubscribeToOutgoingHealing(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
   	void UnsubscribeFromOutgoingHealing(FDamageEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
   	void AddOutgoingHealingRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
   	void RemoveOutgoingHealingRestriction(FDamageRestriction const& Restriction);
	bool CheckOutgoingHealingRestricted(FDamageInfo const& HealingInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
   	void AddOutgoingHealingModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
   	void RemoveOutgoingHealingModifier(FDamageModCondition const& Modifier);
	float GetModifiedOutgoingHealing(FDamageInfo const& HealingInfo, FDamageModCondition const& SourceMod) const;
	
	void NotifyOfOutgoingHealing(FDamagingEvent const& HealingEvent);
	
private:
	
    FDamageEventNotification OnOutgoingHealing;
    TArray<FDamageRestriction> OutgoingHealingRestrictions;
    TArray<FDamageModCondition> OutgoingHealingModifiers;
    
    UFUNCTION(Client, Unreliable)
    void ClientNotifyOfOutgoingHealing(FDamagingEvent const& HealingEvent);

	FDamageModCondition HealingDoneModFromStat;
	UFUNCTION()
	FCombatModifier ModifyHealingDoneFromStat(FDamageInfo const& HealingInfo);

//Incoming Damage

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Damage")
	bool CanEverReceiveDamage() const { return bCanEverReceiveDamage; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage", meta = (AutoCreateRefTerm = "SourceModifier, ThreatParams"))
	FDamagingEvent ApplyDamage(float const Amount, AActor* AppliedBy, UObject* Source,
		EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions,
		bool const bIgnoreModifiers, bool const bFromSnapshot, FDamageModCondition const& SourceModifier, FThreatFromDamage const& ThreatParams);

	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SubscribeToIncomingDamage(FDamageEventCallback const& Callback);
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
	float GetModifiedIncomingDamage(FDamageInfo const& DamageInfo) const;

private:

	UPROPERTY(EditAnywhere, Category = "Damage")
	bool bCanEverReceiveDamage = true;

	bool bHasPendingKillingBlow = false;
	FDamagingEvent PendingKillingBlow;
	
	FDamageEventNotification OnIncomingDamage;
	TArray<FDamageRestriction> IncomingDamageRestrictions;
	TArray<FDamageModCondition> IncomingDamageModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfIncomingDamage(FDamagingEvent const& DamageEvent);

	FDamageModCondition DamageTakenModFromStat;
	UFUNCTION()
	FCombatModifier ModifyDamageTakenFromStat(FDamageInfo const& DamageInfo);
	FBuffRestriction DamageBuffRestriction;
	UFUNCTION()
	bool RestrictDamageBuffs(FBuffApplyEvent const& BuffEvent);

	//Incoming Healing

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Healing")
	bool CanEverReceiveHealing() const { return bCanEverReceiveHealing; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing", meta = (AutoCreateRefTerm = "SourceModifier, ThreatParams"))
	FDamagingEvent ApplyHealing(float const Amount, AActor* AppliedBy, UObject* Source,
		EDamageHitStyle const HitStyle, EDamageSchool const School, bool const bIgnoreRestrictions,
		bool const bIgnoreModifiers, bool const bFromSnapshot, FDamageModCondition const& SourceModifier, FThreatFromDamage const& ThreatParams);

	UFUNCTION(BlueprintCallable, Category = "Healing")
	void SubscribeToIncomingHealing(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
	void UnsubscribeFromIncomingHealingSuccess(FDamageEventCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddIncomingHealingRestriction(FDamageRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveIncomingHealingRestriction(FDamageRestriction const& Restriction);
	bool CheckIncomingHealingRestricted(FDamageInfo const& HealingInfo);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void AddIncomingHealingModifier(FDamageModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing")
	void RemoveIncomingHealingModifier(FDamageModCondition const& Modifier);
	float GetModifiedIncomingHealing(FDamageInfo const& HealingInfo) const;

private:

	UPROPERTY(EditAnywhere, Category = "Healing")
	bool bCanEverReceiveHealing = true;
	
	FDamageEventNotification OnIncomingHealing;
	TArray<FDamageRestriction> IncomingHealingRestrictions;
	TArray<FDamageModCondition> IncomingHealingModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfIncomingHealing(FDamagingEvent const& HealingEvent);

	FDamageModCondition HealingTakenModFromStat;
	UFUNCTION()
	FCombatModifier ModifyHealingTakenFromStat(FDamageInfo const& HealingInfo);
	FBuffRestriction HealingBuffRestriction;
	UFUNCTION()
	bool RestrictHealingBuffs(FBuffApplyEvent const& BuffEvent);
};