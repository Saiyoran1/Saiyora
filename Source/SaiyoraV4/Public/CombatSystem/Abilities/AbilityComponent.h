#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "AbilityStructs.h"
#include "CrowdControlStructs.h"
#include "DamageStructs.h"
#include "CombatAbility.h"
#include "AbilityComponent.generated.h"

class UCrowdControlHandler;
class UDamageHandler;
class UStatHandler;
class UResourceHandler;

USTRUCT()
struct FAbilityRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY()
	int32 PredictionID = 0;
	UPROPERTY()
	int32 Tick = 0;
	UPROPERTY()
	float ClientStartTime = 0.0f;
	UPROPERTY()
	FAbilityOrigin Origin;
	UPROPERTY()
	TArray<FAbilityTargetSet> Targets;

	friend FArchive& operator<<(FArchive& Ar, FAbilityRequest& Request)
	{
		Ar << Request.AbilityClass;
		Ar << Request.PredictionID;
		Ar << Request.Tick;
		Ar << Request.ClientStartTime;
		Ar << Request.Origin;
		Ar << Request.Targets;
		return Ar;
	}

	void Clear() { AbilityClass = nullptr; PredictionID = 0; Tick = 0; ClientStartTime = 0.0f; Targets.Empty(); }
};

USTRUCT()
struct FServerAbilityResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bSuccess = false;
	UPROPERTY()
	ECastFailReason FailReason = ECastFailReason::None;
	UPROPERTY()
	int32 PredictionID = 0;
	UPROPERTY()
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY()
	float ClientStartTime = 0.0f;
	UPROPERTY()
	bool bActivatedGlobal = false;
	UPROPERTY()
	float GlobalLength = 0.0f;
	UPROPERTY()
	int32 ChargesSpent = 0;
	UPROPERTY()
	bool bActivatedCastBar = false;
	UPROPERTY()
	float CastLength = 0.0f;
	UPROPERTY()
	bool bInterruptible = false;
	UPROPERTY()
	TArray<FAbilityCost> AbilityCosts;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

//Setup

public:

	static const float MAXPINGCOMPENSATION;
	static const float MINCASTLENGTH;
	static const float MINGCDLENGTH;
	static const float MINCDLENGTH;
	static const float ABILITYQUEWINDOW;

	UAbilityComponent();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	AGameState* GetGameStateRef() const { return GameStateRef; }
	UResourceHandler* GetResourceHandlerRef() const { return ResourceHandlerRef; }

private:

	bool IsLocallyControlled() const;
	UPROPERTY()
	AGameState* GameStateRef = nullptr;
	UPROPERTY()
	UResourceHandler* ResourceHandlerRef = nullptr;
	UPROPERTY()
	UStatHandler* StatHandlerRef = nullptr;
	UPROPERTY()
	UDamageHandler* DamageHandlerRef = nullptr;
	UPROPERTY()
	UCrowdControlHandler* CrowdControlHandlerRef = nullptr;

//Ability Management

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	UCombatAbility* AddNewAbility(const TSubclassOf<UCombatAbility> AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveAbility(const TSubclassOf<UCombatAbility> AbilityClass);
	UFUNCTION(BlueprintPure, Category = "Abilities")
	UCombatAbility* FindActiveAbility(const TSubclassOf<UCombatAbility> AbilityClass) const { return ActiveAbilities.FindRef(AbilityClass); }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	void GetActiveAbilities(TArray<UCombatAbility*>& OutAbilities) const { ActiveAbilities.GenerateValueArray(OutAbilities); }
	UPROPERTY(BlueprintAssignable)
	FAbilityInstanceNotification OnAbilityAdded;
	UPROPERTY(BlueprintAssignable)
	FAbilityInstanceNotification OnAbilityRemoved;

	void NotifyOfReplicatedAbility(UCombatAbility* NewAbility);
	void NotifyOfReplicatedAbilityRemoval(UCombatAbility* RemovedAbility);

private:

	UPROPERTY(EditAnywhere, Category = "Abilities")
	TSet<TSubclassOf<UCombatAbility>> DefaultAbilities;
	UPROPERTY()
	TMap<TSubclassOf<UCombatAbility>, UCombatAbility*> ActiveAbilities;
	UPROPERTY()
	TArray<UCombatAbility*> RecentlyRemovedAbilities;
	UFUNCTION()
	void CleanupRemovedAbility(UCombatAbility* Ability) { RecentlyRemovedAbilities.Remove(Ability); }
	

//Ability Usage

public:

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FAbilityEvent UseAbility(const TSubclassOf<UCombatAbility> AbilityClass);
	bool UseAbilityFromPredictedMovement(const FAbilityRequest& Request);
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool CanUseAbility(const UCombatAbility* Ability, ECastFailReason& OutFailReason) const;
	UPROPERTY(BlueprintAssignable)
	FAbilityNotification OnAbilityTick;
	UPROPERTY(BlueprintAssignable)
	FAbilityMispredictionNotification OnAbilityMispredicted;
	
	void AddAbilityTagRestriction(UBuff* Source, const FGameplayTag Tag);
	void RemoveAbilityTagRestriction(UBuff* Source, const FGameplayTag Tag);
	void AddAbilityClassRestriction(UBuff* Source, const TSubclassOf<UCombatAbility> Class);
	void RemoveAbilityClassRestriction(UBuff* Source, const TSubclassOf<UCombatAbility> Class);

private:

	TMultiMap<FGameplayTag, UBuff*> AbilityUsageTagRestrictions;
	TMultiMap<TSubclassOf<UCombatAbility>, UBuff*> AbilityUsageClassRestrictions;
	int32 LastPredictionID = 0;
	int32 GenerateNewPredictionID();
	TMap<int32, FClientAbilityPrediction> UnackedAbilityPredictions;
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPredictAbility(const FAbilityRequest& Request);
	bool ServerPredictAbility_Validate(const FAbilityRequest& Request) { return true; }
	UFUNCTION(Client, Reliable)
	void ClientPredictionResult(const FServerAbilityResult& Result);
	FTimerHandle TickHandle;
	UFUNCTION()
	void TickCurrentCast();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityTick(const FAbilityEvent& Event);
	TMap<FPredictedTick, bool> PredictedTickRecord;
	TArray<FAbilityEvent> TicksAwaitingParams;
	TMap<FPredictedTick, FAbilityParams> ParamsAwaitingTicks;
	void RemoveExpiredTicks();

//Cancelling

public:

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FCancelEvent CancelCurrentCast();
	UPROPERTY(BlueprintAssignable)
	FAbilityCancelNotification OnAbilityCancelled;
		
private:

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCancelAbility(const FCancelRequest& Request);
	bool ServerCancelAbility_Validate(const FCancelRequest& Request) { return true; }
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityCancel(const FCancelEvent& Event);

//Interrupting

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, const bool bIgnoreRestrictions);
	UPROPERTY(BlueprintAssignable)
	FInterruptNotification OnAbilityInterrupted;
	
	void AddInterruptRestriction(UBuff* Source, const FInterruptRestriction& Restriction);
	void RemoveInterruptRestriction(const UBuff* Source);

private:

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityInterrupt(const FInterruptEvent& InterruptEvent);
	UFUNCTION(Client, Reliable)
	void ClientAbilityInterrupt(const FInterruptEvent& InterruptEvent);
	TMap<UBuff*, FInterruptRestriction> InterruptRestrictions;
	FCrowdControlCallback CcCallback;
	UFUNCTION()
	void InterruptCastOnCrowdControl(const FCrowdControlStatus& PreviousStatus, const FCrowdControlStatus& NewStatus);
	FLifeStatusCallback DeathCallback;
	UFUNCTION()
	void InterruptCastOnDeath(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);

//Casting

public:

	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsCasting() const { return CastingState.bIsCasting; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	UCombatAbility* GetCurrentCast() const { return CastingState.CurrentCast; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsInterruptible() const { return CastingState.bIsCasting && CastingState.bInterruptible; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetCurrentCastLength() const { return CastingState.bIsCasting && CastingState.CastEndTime != -1.0f ? FMath::Max(0.0f, CastingState.CastEndTime - CastingState.CastStartTime) : -1.0f; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetCastTimeRemaining() const { return CastingState.bIsCasting && CastingState.CastEndTime != -1.0f ? FMath::Max(0.0f, CastingState.CastEndTime - GameStateRef->GetServerWorldTimeSeconds()) : -1.0f; }
	UPROPERTY(BlueprintAssignable)
	FCastingStateNotification OnCastStateChanged;
	
	void AddCastLengthModifier(UBuff* Source, const FAbilityModCondition& Modifier);
	void RemoveCastLengthModifier(const UBuff* Source);
	float CalculateCastLength(UCombatAbility* Ability, const bool bWithPingComp) const;

private:

	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	UFUNCTION()
	void OnRep_CastingState(const FCastingState& Previous) { OnCastStateChanged.Broadcast(Previous, CastingState); }
	void StartCast(UCombatAbility* Ability, const int32 PredictionID = 0);
	void EndCast();
	TMap<UBuff*, FAbilityModCondition> CastLengthMods;
	

//Global Cooldown

public:

	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsGlobalCooldownActive() const { return GlobalCooldownState.bGlobalCooldownActive; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float GetCurrentGlobalCooldownLength() const { return GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime != -1.0f ? FMath::Max(0.0f, GlobalCooldownState.EndTime - GlobalCooldownState.StartTime) : -1.0f; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float GetGlobalCooldownTimeRemaining() const { return GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime != -1.0f ? FMath::Max(0.0f, GlobalCooldownState.EndTime - GameStateRef->GetServerWorldTimeSeconds()) : -1.0f; }
	UPROPERTY(BlueprintAssignable)
	FGlobalCooldownNotification OnGlobalCooldownChanged;
	
	void AddGlobalCooldownModifier(UBuff* Source, const FAbilityModCondition& Modifier);
	void RemoveGlobalCooldownModifier(const UBuff* Source);
	float CalculateGlobalCooldownLength(UCombatAbility* Ability, const bool bWithPingComp) const;

private:

	FGlobalCooldown GlobalCooldownState;
	void StartGlobalCooldown(UCombatAbility* Ability, const int32 PredictionID = 0);
	FTimerHandle GlobalCooldownHandle;
	UFUNCTION()
	void EndGlobalCooldown();
	TMap<UBuff*, FAbilityModCondition> GlobalCooldownMods;
	

//Cooldown

public:

	void AddCooldownModifier(UBuff* Source, const FAbilityModCondition& Modifier);
	void RemoveCooldownModifier(const UBuff* Source);
	float CalculateCooldownLength(UCombatAbility* Ability, const bool bWithPingComp) const;

private:

	TMap<UBuff*, FAbilityModCondition> CooldownMods;

//Cost

public:

	void AddGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifier& Modifier);
	void RemoveGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, UBuff* Source);

//Queueing

private:

	EQueueStatus QueueStatus = EQueueStatus::Empty;
	TSubclassOf<UCombatAbility> QueuedAbility;
	bool bUsingAbilityFromQueue = false;
	bool TryQueueAbility(const TSubclassOf<UCombatAbility> AbilityClass);
	FTimerHandle QueueExpirationHandle;
	UFUNCTION()
	void ExpireQueue();
	
};