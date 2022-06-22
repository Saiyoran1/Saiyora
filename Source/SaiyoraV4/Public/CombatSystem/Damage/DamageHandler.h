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
class ADungeonGameState;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UDamageHandler : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UDamageHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	
	UPROPERTY()
	UStatHandler* StatHandler = nullptr;
	UPROPERTY()
	UPlaneComponent* PlaneComponent = nullptr;
	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;
	UPROPERTY()
	ADungeonGameState* GameStateRef = nullptr;

//Health
	
public:

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Health")
	ELifeStatus GetLifeStatus() const { return LifeStatus; }
	UFUNCTION(BlueprintPure, Category = "Health")
	bool HasHealth() const { return bHasHealth; }
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bHasHealth && LifeStatus != ELifeStatus::Alive; }

	UFUNCTION(BlueprintAuthorityOnly, Category = "Health")
	void KillActor(AActor* Attacker, UObject* Source, const bool bIgnoreDeathRestrictions);
	UFUNCTION(BlueprintAuthorityOnly, Category = "Health")
	void RespawnActor(const bool bForceRespawnLocation, const FVector& OverrideRespawnLocation, const bool bForceHealthPercentage, const float OverrideHealthPercentage);
	UFUNCTION(BlueprintAuthorityOnly, Category = "Health")
	void UpdateRespawnPoint(const FVector& NewLocation);

	UPROPERTY(BlueprintAssignable)
	FHealthChangeNotification OnHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FHealthChangeNotification OnMaxHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FLifeStatusNotification OnLifeStatusChanged;

	void AddDeathRestriction(UBuff* Source, const FDeathRestriction& Restriction);
	void RemoveDeathRestriction(const UBuff* Source);

private:

	UPROPERTY(EditAnywhere, Category = "Health")
	bool bHasHealth = true;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bStaticMaxHealth = false;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float DefaultMaxHealth = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bCanRespawn = false;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bRespawnInPlace = false;

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
	FVector RespawnLocation;
	
	UPROPERTY()
	TMap<UBuff*, FDeathRestriction> DeathRestrictions;

	void UpdateMaxHealth(const float NewMaxHealth);
	FStatCallback MaxHealthStatCallback;
	UFUNCTION()
	void ReactToMaxHealthStat(const FGameplayTag StatTag, const float NewValue);
	bool CheckDeathRestricted(const FDamagingEvent& DamageEvent);
	void Die();

//Kill Count

public:

	UFUNCTION(BlueprintPure, Category = "Kill Count")
	EKillCountType GetKillCountType() const { return KillCountType; }
	UFUNCTION(BlueprintPure, Category = "Kill Count")
	int32 GetKillCountValue() const { return KillCountValue; }
	UFUNCTION(BlueprintPure, Category = "Kill Count")
	FGameplayTag GetBossTag() const { return BossTag; }

private:

	UPROPERTY(EditAnywhere, Category = "Kill Count")
	EKillCountType KillCountType = EKillCountType::None;
	UPROPERTY(EditAnywhere, Category = "Kill Count")
	int32 KillCountValue = 0;
	UPROPERTY(EditAnywhere, Category = "Kill Count", meta = (GameplayTagFilter = "Boss"))
	FGameplayTag BossTag;

//Outgoing Damage

public:

	UPROPERTY(BlueprintAssignable)
	FDamageEventNotification OnOutgoingDamage;
	UPROPERTY(BlueprintAssignable)
	FDamageEventNotification OnKillingBlow;

   	void AddOutgoingDamageRestriction(UBuff* Source, const FDamageRestriction& Restriction);
   	void RemoveOutgoingDamageRestriction(const UBuff* Source);
	bool CheckOutgoingDamageRestricted(const FDamageInfo& DamageInfo);
    void AddOutgoingDamageModifier(UBuff* Source, const FDamageModCondition& Modifier);
   	void RemoveOutgoingDamageModifier(const UBuff* Source);
	float GetModifiedOutgoingDamage(const FDamageInfo& DamageInfo, const FDamageModCondition& SourceMod) const;

	void NotifyOfOutgoingDamage(const FDamagingEvent& DamageEvent);
	
private:
	
	UPROPERTY()
	TMap<UBuff*, FDamageRestriction> OutgoingDamageRestrictions;
	UPROPERTY()
	TMap<UBuff*, FDamageModCondition> OutgoingDamageModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfOutgoingDamage(const FDamagingEvent& DamageEvent);

//Outgoing Healing

public:

	UPROPERTY(BlueprintAssignable)
	FDamageEventNotification OnOutgoingHealing;

   	void AddOutgoingHealingRestriction(UBuff* Source, const FDamageRestriction& Restriction);
   	void RemoveOutgoingHealingRestriction(const UBuff* Source);
	bool CheckOutgoingHealingRestricted(const FDamageInfo& HealingInfo);
   	void AddOutgoingHealingModifier(UBuff* Source, const FDamageModCondition& Modifier);
   	void RemoveOutgoingHealingModifier(const UBuff* Source);
	float GetModifiedOutgoingHealing(const FDamageInfo& HealingInfo, const FDamageModCondition& SourceMod) const;
	
	void NotifyOfOutgoingHealing(const FDamagingEvent& HealingEvent);
	
private:
	
	UPROPERTY()
    TMap<UBuff*, FDamageRestriction> OutgoingHealingRestrictions;
	UPROPERTY()
    TMap<UBuff*, FDamageModCondition> OutgoingHealingModifiers;
    
    UFUNCTION(Client, Unreliable)
    void ClientNotifyOfOutgoingHealing(const FDamagingEvent& HealingEvent);

//Incoming Damage

public:

	UFUNCTION(BlueprintPure, Category = "Damage")
	bool CanEverReceiveDamage() const { return bCanEverReceiveDamage; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Damage", meta = (AutoCreateRefTerm = "SourceModifier, ThreatParams"))
	FDamagingEvent ApplyDamage(const float Amount, AActor* AppliedBy, UObject* Source, const EDamageHitStyle HitStyle,
		const EDamageSchool School, const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions,
		const bool bFromSnapshot, const FDamageModCondition& SourceModifier, const FThreatFromDamage& ThreatParams);

	UPROPERTY(BlueprintAssignable)
	FDamageEventNotification OnIncomingDamage;

	void AddIncomingDamageRestriction(UBuff* Source, const FDamageRestriction& Restriction);
	void RemoveIncomingDamageRestriction(const UBuff* Source);
	bool CheckIncomingDamageRestricted(const FDamageInfo& DamageInfo);
	void AddIncomingDamageModifier(UBuff* Source, const FDamageModCondition& Modifier);
	void RemoveIncomingDamageModifier(const UBuff* Source);
	float GetModifiedIncomingDamage(const FDamageInfo& DamageInfo) const;

private:

	UPROPERTY(EditAnywhere, Category = "Damage")
	bool bCanEverReceiveDamage = true;

	bool bHasPendingKillingBlow = false;
	FDamagingEvent PendingKillingBlow;
	
	UPROPERTY()
	TMap<UBuff*, FDamageRestriction> IncomingDamageRestrictions;
	UPROPERTY()
	TMap<UBuff*, FDamageModCondition> IncomingDamageModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfIncomingDamage(const FDamagingEvent& DamageEvent);

//Incoming Healing

public:

	UFUNCTION(BlueprintPure, Category = "Healing")
	bool CanEverReceiveHealing() const { return bCanEverReceiveHealing; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Healing", meta = (AutoCreateRefTerm = "SourceModifier, ThreatParams"))
	FDamagingEvent ApplyHealing(const float Amount, AActor* AppliedBy, UObject* Source,
		const EDamageHitStyle HitStyle, const EDamageSchool School, const bool bIgnoreRestrictions,
		const bool bIgnoreModifiers, const bool bFromSnapshot, const FDamageModCondition& SourceModifier, const FThreatFromDamage& ThreatParams);

	UPROPERTY(BlueprintAssignable)
	FDamageEventNotification OnIncomingHealing;

	void AddIncomingHealingRestriction(UBuff* Source, const FDamageRestriction& Restriction);
	void RemoveIncomingHealingRestriction(const UBuff* Source);
	bool CheckIncomingHealingRestricted(const FDamageInfo& HealingInfo);
	void AddIncomingHealingModifier(UBuff* Source, const FDamageModCondition& Modifier);
	void RemoveIncomingHealingModifier(const UBuff* Source);
	float GetModifiedIncomingHealing(const FDamageInfo& HealingInfo) const;

private:

	UPROPERTY(EditAnywhere, Category = "Healing")
	bool bCanEverReceiveHealing = true;
	
	UPROPERTY()
	TMap<UBuff*, FDamageRestriction> IncomingHealingRestrictions;
	UPROPERTY()
	TMap<UBuff*, FDamageModCondition> IncomingHealingModifiers;

	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfIncomingHealing(const FDamagingEvent& HealingEvent);
};