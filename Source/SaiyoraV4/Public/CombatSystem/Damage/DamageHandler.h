#pragma once
#include "CoreMinimal.h"
#include "DamageStructs.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BuffStructs.h"
#include "NPCEnums.h"
#include "StatStructs.h"
#include "DamageHandler.generated.h"

class UPendingResurrectionFunction;
class UStatHandler;
class UBuffHandler;
class UCombatStatusComponent;
class ADungeonGameState;
class UNPCAbilityComponent;

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
	UStatHandler* StatHandlerRef = nullptr;
	UPROPERTY()
	UCombatStatusComponent* CombatStatusComponentRef = nullptr;
	UPROPERTY()
	UNPCAbilityComponent* NPCComponentRef = nullptr;
	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;
	UPROPERTY()
	ADungeonGameState* GameStateRef = nullptr;

	UFUNCTION()
	void OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior, const ENPCCombatBehavior NewBehavior);
	FHealthEventRestriction DisableHealthEvents;
	UFUNCTION()
	bool DisableAllHealthEvents(const FHealthEventInfo& Event) { return true; }

//Health
	
public:

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentAbsorb() const { return CurrentAbsorb; }
	UFUNCTION(BlueprintPure, Category = "Health")
	bool HasHealth() const { return bHasHealth; }
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bHasHealth && LifeStatus != ELifeStatus::Alive; }

	UPROPERTY(BlueprintAssignable)
	FHealthChangeNotification OnHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FHealthChangeNotification OnMaxHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FHealthChangeNotification OnAbsorbChanged;

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
	void OnRep_CurrentHealth(const float PreviousValue);
	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth)
	float MaxHealth = 1.0f;
	UFUNCTION()
	void OnRep_MaxHealth(const float PreviousValue);
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAbsorb)
	float CurrentAbsorb = 0.0f;
	UFUNCTION()
	void OnRep_CurrentAbsorb(const float PreviousValue);
	
	void UpdateMaxHealth(const float NewMaxHealth);
	FStatCallback MaxHealthStatCallback;
	UFUNCTION()
	void ReactToMaxHealthStat(const FGameplayTag StatTag, const float NewValue);

#pragma region Death and Resurrection

public:

	UFUNCTION(BlueprintPure, Category = "Health")
	ELifeStatus GetLifeStatus() const { return LifeStatus; }
	UPROPERTY(BlueprintAssignable)
	FLifeStatusNotification OnLifeStatusChanged;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void KillActor(AActor* Attacker, UObject* Source, const bool bIgnoreDeathRestrictions);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health", meta = (AutoCreateRefTerm = "OverrideRespawnLocation"))
	void RespawnActor(const bool bForceRespawnLocation = false, const FVector& OverrideRespawnLocation = FVector::ZeroVector,
		const bool bForceHealthPercentage = false, const float OverrideHealthPercentage = 1.0f);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void UpdateRespawnPoint(const FVector& NewLocation);
	//Local-callable respawn function, for when a player requests to respawn at his default location.
	void RequestDefaultRespawn() { Server_RequestRespawn(); }

	int AddPendingResurrection(UPendingResurrectionFunction* PendingResFunction);
	void RemovePendingResurrection(const int ResID);
	const TArray<FPendingResurrection>& GetPendingResurrections() const { return PendingResurrections; }
	FPendingResNotification OnPendingResAdded;
	FPendingResNotification OnPendingResRemoved;
	void AcceptResurrection(const int ResID);
	void DeclineResurrection(const int ResID);

	void AddDeathRestriction(const FDeathRestriction& Restriction) { DeathRestrictions.Add(Restriction); }
	void RemoveDeathRestriction(const FDeathRestriction& Restriction);

private:

	UPROPERTY(EditAnywhere, Category = "Health")
	bool bCanBeResurrected = false;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bRespawnInPlace = false;

	UPROPERTY(ReplicatedUsing = OnRep_LifeStatus)
	ELifeStatus LifeStatus = ELifeStatus::Invalid;
	UFUNCTION()
	void OnRep_LifeStatus(const ELifeStatus PreviousValue) { OnLifeStatusChanged.Broadcast(GetOwner(), PreviousValue, LifeStatus); }

	FVector RespawnLocation;
	
	UPROPERTY(ReplicatedUsing = OnRep_PendingResurrections)
	TArray<FPendingResurrection> PendingResurrections;
	UFUNCTION()
	void OnRep_PendingResurrections(const TArray<FPendingResurrection>& PreviousResurrections);
	
	UFUNCTION(Server, Reliable)
	void Server_MakeResDecision(const int ResID, const bool bAccept);
	UFUNCTION(Server, Reliable)
	void Server_RequestRespawn();

	void Die();
	TConditionalRestrictionList<FDeathRestriction> DeathRestrictions;

	//Called when an NPC receives a resurrection.
	//This waits one frame then accepts the resurrection. Accepting on the same frame causes issues with the OnApply function of the resurrection buff function.
	UFUNCTION()
	void QueueResurrectionAutoAccept(const FPendingResurrection& PendingRes);
	//Called one frame after an NPC receives a resurrection.
	//Automatically takes the resurrection if the NPC is dead and the resurrection buff is still applied.
	UFUNCTION()
	void AutoAcceptResurrection(const int ResID);

#pragma endregion 

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
	UPROPERTY(EditAnywhere, Category = "Kill Count", meta = (Categories = "Boss"))
	FGameplayTag BossTag;

//Health Events

public:

	UFUNCTION(BlueprintPure, Category = "Health")
	bool CanEverReceiveDamage() const { return bCanEverReceiveDamage; }
	UFUNCTION(BlueprintPure, Category = "Health")
	bool CanEverReceiveHealing() const { return bCanEverReceiveHealing; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health", meta = (AutoCreateRefTerm = "SourceModifier, ThreatParams"))
	FHealthEvent ApplyHealthEvent(const EHealthEventType EventType, const float Amount, AActor* AppliedBy, UObject* Source, const EEventHitStyle HitStyle,
		const EElementalSchool School, const bool bBypassAbsorbs, const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions,
		const bool bFromSnapshot, const FHealthEventModCondition& SourceModifier, const FThreatFromDamage& ThreatParams);

	UPROPERTY(BlueprintAssignable)
	FHealthEventNotification OnIncomingHealthEvent;
	UPROPERTY(BlueprintAssignable)
	FHealthEventNotification OnOutgoingHealthEvent;
	UPROPERTY(BlueprintAssignable)
	FHealthEventNotification OnKillingBlow;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddIncomingHealthEventRestriction(const FHealthEventRestriction& Restriction) { IncomingHealthEventRestrictions.Add(Restriction); }
	UFUNCTION(BlueprintCallable, Category = "Health")
	void RemoveIncomingHealthEventRestriction(const FHealthEventRestriction& Restriction) { IncomingHealthEventRestrictions.Remove(Restriction); }
	
	UFUNCTION(BlueprintCallable, Category = "Health")
   	void AddOutgoingHealthEventRestriction(const FHealthEventRestriction& Restriction) { OutgoingHealthEventRestrictions.Add(Restriction); }
	UFUNCTION(BlueprintCallable, Category = "Health")
   	void RemoveOutgoingHealthEventRestriction(const FHealthEventRestriction& Restriction) { OutgoingHealthEventRestrictions.Remove(Restriction); }
	bool CheckOutgoingHealthEventRestricted(const FHealthEventInfo& EventInfo) { return OutgoingHealthEventRestrictions.IsRestricted(EventInfo); }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddIncomingHealthEventModifier(const FHealthEventModCondition& Modifier) { IncomingHealthEventModifiers.Add(Modifier); }
	UFUNCTION(BlueprintCallable, Category = "Health")
	void RemoveIncomingHealthEventModifier(const FHealthEventModCondition& Modifier) { IncomingHealthEventModifiers.Remove(Modifier); }
	float GetModifiedIncomingHealthEventValue(const FHealthEventInfo& EventInfo) const;

	UFUNCTION(BlueprintCallable, Category = "Health")
    void AddOutgoingHealthEventModifier(const FHealthEventModCondition& Modifier) { OutgoingHealthEventModifiers.Add(Modifier); }
	UFUNCTION(BlueprintCallable, Category = "Health")
   	void RemoveOutgoingHealthEventModifier(const FHealthEventModCondition& Modifier) { OutgoingHealthEventModifiers.Remove(Modifier); }
	float GetModifiedOutgoingHealthEventValue(const FHealthEventInfo& EventInfo, const FHealthEventModCondition& SourceMod) const;
	
	void NotifyOfOutgoingHealthEvent(const FHealthEvent& HealthEvent);
	
private:

	UPROPERTY(EditAnywhere, Category = "Health")
	bool bCanEverReceiveDamage = true;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bCanEverReceiveHealing = true;

	bool bHasPendingKillingBlow = false;
	FHealthEvent PendingKillingBlow;

	TConditionalRestrictionList<FHealthEventRestriction> IncomingHealthEventRestrictions;
	TConditionalRestrictionList<FHealthEventRestriction> OutgoingHealthEventRestrictions;

	TConditionalModifierList<FHealthEventModCondition> IncomingHealthEventModifiers;
	TConditionalModifierList<FHealthEventModCondition> OutgoingHealthEventModifiers;

	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfIncomingHealthEvent(const FHealthEvent& HealthEvent);
	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfOutgoingHealthEvent(const FHealthEvent& HealthEvent);
};