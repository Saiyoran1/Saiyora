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
	static FGameplayTag GenericDamageTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Damage")), false); }
	static FGameplayTag GenericHealingTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Healing")), false); }

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

	void AddDeathRestriction(UBuff* Source, FDeathRestriction const& Restriction);
	void RemoveDeathRestriction(UBuff* Source);

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
	TMap<UBuff*, FDeathRestriction> DeathRestrictions;

	void UpdateMaxHealth(float const NewMaxHealth);
	FStatCallback MaxHealthStatCallback;
	UFUNCTION()
	void ReactToMaxHealthStat(FGameplayTag const& StatTag, float const NewValue);
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

   	void AddOutgoingDamageRestriction(UBuff* Source, FDamageRestriction const& Restriction);
   	void RemoveOutgoingDamageRestriction(UBuff* Source);
	bool CheckOutgoingDamageRestricted(FDamageInfo const& DamageInfo);
    void AddOutgoingDamageModifier(UBuff* Source, FDamageModCondition const& Modifier);
   	void RemoveOutgoingDamageModifier(UBuff* Source);
	float GetModifiedOutgoingDamage(FDamageInfo const& DamageInfo, FDamageModCondition const& SourceMod) const;

	void NotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent);
	
private:
	
	FDamageEventNotification OnOutgoingDamage;
	FDamageEventNotification OnKillingBlow;
	TMap<UBuff*, FDamageRestriction> OutgoingDamageRestrictions;
	TMap<UBuff*, FDamageModCondition> OutgoingDamageModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfOutgoingDamage(FDamagingEvent const& DamageEvent);

//Outgoing Healing

public:

	UFUNCTION(BlueprintCallable, Category = "Healing")
   	void SubscribeToOutgoingHealing(FDamageEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Healing")
   	void UnsubscribeFromOutgoingHealing(FDamageEventCallback const& Callback);

   	void AddOutgoingHealingRestriction(UBuff* Source, FDamageRestriction const& Restriction);
   	void RemoveOutgoingHealingRestriction(UBuff* Source);
	bool CheckOutgoingHealingRestricted(FDamageInfo const& HealingInfo);
   	void AddOutgoingHealingModifier(UBuff* Source, FDamageModCondition const& Modifier);
   	void RemoveOutgoingHealingModifier(UBuff* Source);
	float GetModifiedOutgoingHealing(FDamageInfo const& HealingInfo, FDamageModCondition const& SourceMod) const;
	
	void NotifyOfOutgoingHealing(FDamagingEvent const& HealingEvent);
	
private:
	
    FDamageEventNotification OnOutgoingHealing;
    TMap<UBuff*, FDamageRestriction> OutgoingHealingRestrictions;
    TMap<UBuff*, FDamageModCondition> OutgoingHealingModifiers;
    
    UFUNCTION(Client, Unreliable)
    void ClientNotifyOfOutgoingHealing(FDamagingEvent const& HealingEvent);

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
	void UnsubscribeFromIncomingDamage(FDamageEventCallback const& Callback);

	void AddIncomingDamageRestriction(UBuff* Source, FDamageRestriction const& Restriction);
	void RemoveIncomingDamageRestriction(UBuff* Source);
	bool CheckIncomingDamageRestricted(FDamageInfo const& DamageInfo);
	void AddIncomingDamageModifier(UBuff* Source, FDamageModCondition const& Modifier);
	void RemoveIncomingDamageModifier(UBuff* Source);
	float GetModifiedIncomingDamage(FDamageInfo const& DamageInfo) const;

private:

	UPROPERTY(EditAnywhere, Category = "Damage")
	bool bCanEverReceiveDamage = true;

	bool bHasPendingKillingBlow = false;
	FDamagingEvent PendingKillingBlow;
	
	FDamageEventNotification OnIncomingDamage;
	TMap<UBuff*, FDamageRestriction> IncomingDamageRestrictions;
	TMap<UBuff*, FDamageModCondition> IncomingDamageModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfIncomingDamage(FDamagingEvent const& DamageEvent);

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

	void AddIncomingHealingRestriction(UBuff* Source, FDamageRestriction const& Restriction);
	void RemoveIncomingHealingRestriction(UBuff* Source);
	bool CheckIncomingHealingRestricted(FDamageInfo const& HealingInfo);
	void AddIncomingHealingModifier(UBuff* Source, FDamageModCondition const& Modifier);
	void RemoveIncomingHealingModifier(UBuff* Source);
	float GetModifiedIncomingHealing(FDamageInfo const& HealingInfo) const;

private:

	UPROPERTY(EditAnywhere, Category = "Healing")
	bool bCanEverReceiveHealing = true;
	
	FDamageEventNotification OnIncomingHealing;
	TMap<UBuff*, FDamageRestriction> IncomingHealingRestrictions;
	TMap<UBuff*, FDamageModCondition> IncomingHealingModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfIncomingHealing(FDamagingEvent const& HealingEvent);

	FBuffRestriction HealingBuffRestriction;
	UFUNCTION()
	bool RestrictHealingBuffs(FBuffApplyEvent const& BuffEvent);
};