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
class UCombatStatusComponent;
class ADungeonGameState;
class UNPCAbilityComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UDamageHandler : public UActorComponent
{
	GENERATED_BODY()

#pragma region Core

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

#pragma endregion 
#pragma region Health
	
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

#pragma endregion 
#pragma region Death and Resurrection

public:

	UFUNCTION(BlueprintPure, Category = "Health")
	ELifeStatus GetLifeStatus() const { return LifeStatus; }
	UPROPERTY(BlueprintAssignable)
	FLifeStatusNotification OnLifeStatusChanged;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void KillActor(AActor* Attacker, UObject* Source, const bool bIgnoreDeathRestrictions);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health", meta = (AutoCreateRefTerm = "OverrideRespawnLocation"))
	bool RespawnActor(const bool bForceRespawnLocation = false, const FVector& OverrideRespawnLocation = FVector::ZeroVector,
		const bool bForceHealthPercentage = false, const float OverrideHealthPercentage = 1.0f);
	//Overload of RespawnActor that just takes a pending resurrection.
	bool RespawnActor(const FPendingResurrection& Resurrection);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void UpdateRespawnPoint(const FVector& NewLocation);
	//Local-callable respawn function, for when a player requests to respawn at his default location.
	void RequestDefaultRespawn() { Server_RequestRespawn(); }

	//Called by UPendingResurrectionFunction to add a pending resurrection from a buff.
	bool AddPendingResurrection(UPendingResurrectionFunction* Source);
	//Called when a buff is removed to withdraw a pending resurrection it applied.
	void RemovePendingResurrection(UPendingResurrectionFunction* Source);
	const TArray<FPendingResurrection>& GetPendingResurrections() const { return PendingResurrections; }
	//Called when accepting or declining a pending resurrection. If called locally, redirects to the server.
	void MakeResurrectionDecision(const int ResID, const bool bAccept);
	FPendingResNotification OnPendingResListChanged;

	bool AddDeathRestriction(const FDeathRestriction& Restriction);
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
	void OnRep_PendingResurrections() { OnPendingResListChanged.Broadcast(PendingResurrections); }
	
	UFUNCTION(Server, Reliable)
	void Server_MakeResDecision(const int ResID, const bool bAccept);
	void Server_MakeResDecision_Implementation(const int ResID, const bool bAccept) { MakeResurrectionDecision(ResID, bAccept); }
	UFUNCTION(Server, Reliable)
	void Server_RequestRespawn();
	UFUNCTION(Client, Reliable)
	void Client_ResurrectionRequestFailed();

	bool RespawnActor_Internal(const bool bForceRespawnLocation = false, const FVector& OverrideRespawnLocation = FVector::ZeroVector,
		const bool bForceHealthPercentage = false, const float OverrideHealthPercentage = 1.0f, const FPendingResurrection& Resurrection = FPendingResurrection());

	void Die();
	TConditionalRestrictionList<FDeathRestriction> DeathRestrictions;

	//Called when an NPC receives a resurrection.
	//This waits one frame then accepts the resurrection. Accepting on the same frame causes issues with the OnApply function of the resurrection buff function.
	UFUNCTION()
	void QueueResurrectionAutoAccept(const TArray<FPendingResurrection>& NewResurrections);
	//Called one frame after an NPC receives a resurrection.
	//Automatically takes the resurrection if the NPC is dead and the resurrection buff is still applied.
	UFUNCTION()
	void AutoAcceptResurrection(const int ResID);

#pragma endregion 
#pragma region Kill Count
	
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

#pragma endregion
#pragma region Health Events

public:

	UFUNCTION(BlueprintPure, Category = "Health")
	bool CanEverReceiveDamage() const { return bCanEverReceiveDamage; }
	UFUNCTION(BlueprintPure, Category = "Health")
	bool CanEverReceiveHealing() const { return bCanEverReceiveHealing; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health", meta = (AutoCreateRefTerm = "SourceModifier, ThreatParams"))
	FHealthEvent ApplyHealthEvent(const EHealthEventType EventType, const float Amount, AActor* AppliedBy, UObject* Source, const EEventHitStyle HitStyle,
		const EElementalSchool School, const bool bBypassAbsorbs, const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions,
		const FHealthEventModCondition& SourceModifier, const FThreatFromDamage& ThreatParams);
	//Apply a health event using a previously snapshotted value. The Snapshot struct comes from a previous ApplyHealthEvent call's result, and includes the original modifiers applied to the event.
	//Snapshotted health events ignore outgoing modifiers, under the assumption they were already applied to the passed in value in a previous health event and should not be overwritten.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health", meta = (AutoCreateRefTerm = "Snapshot, SourceModifier, ThreatParams"))
	FHealthEvent ApplySnapshottedHealthEvent(const EHealthEventType EventType, const FHealthEventSnapshot& Snapshot, AActor* AppliedBy, UObject* Source, const EEventHitStyle HitStyle,
		const EElementalSchool School, const bool bBypassAbsorbs, const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions, const FThreatFromDamage& ThreatParams);

	UPROPERTY(BlueprintAssignable)
	FHealthEventNotification OnIncomingHealthEvent;
	UPROPERTY(BlueprintAssignable)
	FHealthEventNotification OnOutgoingHealthEvent;
	UPROPERTY(BlueprintAssignable)
	FHealthEventNotification OnKillingBlow;
	
	bool AddHealthEventRestriction(const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction);
	void RemoveHealthEventRestriction(const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction);

	bool AddHealthEventModifier(const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier);
	void RemoveHealthEventModifier(const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier);
	float ApplyHealthEventModifiers(const ECombatEventDirection Direction, const FHealthEventInfo& EventInfo, TArray<FModifierProxy>& OutModConditions,
		const FHealthEventModCondition& SourceMod = FHealthEventModCondition()) const;
	
	void NotifyOfOutgoingHealthEvent(const FHealthEvent& HealthEvent);
	
private:

	FHealthEvent ApplyHealthEvent_Internal(const EHealthEventType EventType, const float Amount, AActor* AppliedBy, UObject* Source, const EEventHitStyle HitStyle,
		const EElementalSchool School, const bool bBypassAbsorbs, const bool bIgnoreModifiers, const bool bIgnoreRestrictions, const bool bIgnoreDeathRestrictions,
		const bool bFromSnapshot, const FHealthEventSnapshot& Snapshot, const FHealthEventModCondition& SourceModifier, const FThreatFromDamage& ThreatParams);

	UPROPERTY(EditAnywhere, Category = "Health")
	bool bCanEverReceiveDamage = true;
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bCanEverReceiveHealing = true;

	bool bHasPendingKillingBlow = false;
	FHealthEvent PendingKillingBlow;

	TConditionalRestrictionList<FHealthEventRestriction> IncomingHealthEventRestrictions;
	TConditionalRestrictionList<FHealthEventRestriction> OutgoingHealthEventRestrictions;
	bool CheckEventRestricted(const ECombatEventDirection Direction, const FHealthEventInfo& Event, TArray<EHealthEventFailReason>& OutFailReasons, FHealthEventRestriction& OutRestriction);

	TConditionalModifierList<FHealthEventModCondition> IncomingHealthEventModifiers;
	TConditionalModifierList<FHealthEventModCondition> OutgoingHealthEventModifiers;

	UFUNCTION(Client, Unreliable)
	void ClientNotifyOfIncomingHealthEvent(const FHealthEvent& HealthEvent);
	UFUNCTION(Client, Unreliable)
    void ClientNotifyOfOutgoingHealthEvent(const FHealthEvent& HealthEvent);

#pragma endregion
#pragma region DEBUG
#if !UE_BUILD_SHIPPING

public:

	void DEBUG_DumpState(const ECombatEventDirection Direction, TArray<FString>& OutStrings) const;
	void DEBUG_DumpRestrictions(const ECombatEventDirection Direction, TArray<FString>& OutStrings) const;
	void DEBUG_DumpModifiers(const ECombatEventDirection Direction, TArray<FString>& OutStrings) const;
	void DEBUG_DumpDeathRestrictions(TArray<FString>& OutStrings) const;
	
#endif
#pragma endregion 
};