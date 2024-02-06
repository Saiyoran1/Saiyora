#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "AbilityStructs.h"
#include "CrowdControlStructs.h"
#include "DamageStructs.h"
#include "CombatAbility.h"
#include "GameplayTasksComponent.h"
#include "AbilityComponent.generated.h"

class UCombatDebugOptions;
class UCombatStatusComponent;
class USaiyoraMovementComponent;
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
	TArray<ECastFailReason> FailReasons;
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
	TArray<FSimpleAbilityCost> AbilityCosts;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UAbilityComponent : public UGameplayTasksComponent
{
	GENERATED_BODY()

//Setup

public:

	//This ratio is used to reduce server cast and GCD lengths by a fraction of a player's ping, to prevent ping fluctuation from preventing otherwise valid cast predictions. Should always be less than 1.
	static constexpr float LagCompensationRatio = 0.2f;
	//This is the max lag compensation a player is allowed. For cooldowns, this is the max reduction. For GCDs and casts, this * the ratio above is used to determine max reduction.
	static constexpr float MaxLagCompensation = 0.2f;
	//These 3 values must always be higher than MaxLagCompensation, so we don't get negative or 0 length timers.
	static constexpr float MinCastLength = 0.5f;
	static constexpr float MinGcdLength = 0.5f;
	static constexpr float MinCooldownLength = 0.5f;
	static constexpr float AbilityQueueWindow = 0.2f;

	UAbilityComponent(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	AGameState* GetGameStateRef() const { return GameStateRef; }
	UResourceHandler* GetResourceHandlerRef() const { return ResourceHandlerRef; }

protected:

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
	USaiyoraMovementComponent* MovementComponentRef = nullptr;
	UPROPERTY()
	UCrowdControlHandler* CrowdControlHandlerRef = nullptr;
	UPROPERTY()
	UCombatStatusComponent* CombatStatusComponentRef = nullptr;

//Ability Management

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	UCombatAbility* AddNewAbility(const TSubclassOf<UCombatAbility> AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	bool RemoveAbility(const TSubclassOf<UCombatAbility> AbilityClass);
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
	void CleanupRemovedAbility(UCombatAbility* Ability);
	

//Ability Usage

public:

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FAbilityEvent UseAbility(const TSubclassOf<UCombatAbility> AbilityClass);
	bool UseAbilityFromPredictedMovement(const FAbilityRequest& Request);
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
	void ClearOldPredictions(const int32 AckedPredictionID);
	FTimerHandle TickHandle;
	UFUNCTION()
	void TickCurrentCast();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityTick(const FAbilityEvent& Event);
	TMap<FPredictedTick, bool> PredictedTickRecord;
	TArray<FAbilityEvent> TicksAwaitingParams;
	TMap<FPredictedTick, FAbilityParams> ParamsAwaitingTicks;
	void RemoveExpiredTicks();
	TMap<FPredictedTick, float> PredictedTickHistory;

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

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddInterruptRestriction(const FInterruptRestriction& Restriction) { InterruptRestrictions.Add(Restriction); }
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveInterruptRestriction(const FInterruptRestriction& Restriction) { InterruptRestrictions.Remove(Restriction); }

private:

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityInterrupt(const FInterruptEvent& InterruptEvent);
	UFUNCTION(Client, Reliable)
	void ClientAbilityInterrupt(const FInterruptEvent& InterruptEvent);
	UFUNCTION()
	void InterruptCastOnCrowdControl(const FCrowdControlStatus& PreviousStatus, const FCrowdControlStatus& NewStatus);
	UFUNCTION()
	void InterruptCastOnDeath(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	UFUNCTION()
	void InterruptCastOnMovement(AActor* Actor, const bool bNewMovement);
	UFUNCTION()
	void InterruptCastOnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* SwapSource);

	TConditionalRestrictionList<FInterruptRestriction> InterruptRestrictions;

//Casting

public:

	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsCasting() const { return CastingState.bIsCasting; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	UCombatAbility* GetCurrentCast() const { return CastingState.CurrentCast; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsInterruptible() const { return CastingState.bIsCasting && CastingState.bInterruptible; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetCurrentCastLength() const { return CastingState.bIsCasting ? CastingState.CastLength : -1.0f; }
    UFUNCTION(BlueprintPure, Category = "Abilities")
    float GetCastTimeRemaining() const { return CastingState.bIsCasting ? FMath::Max(0.0f, CastingState.CastStartTime + CastingState.CastLength - GameStateRef->GetServerWorldTimeSeconds()) : -1.0f; }
	UPROPERTY(BlueprintAssignable)
	FCastingStateNotification OnCastStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddCastLengthModifier(const FAbilityModCondition& Modifier) { CastLengthMods.Add(Modifier); }
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveCastLengthModifier(const FAbilityModCondition& Modifier) { CastLengthMods.Remove(Modifier); }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float CalculateCastLength(UCombatAbility* Ability) const;

private:

	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	UFUNCTION()
	void OnRep_CastingState(const FCastingState& Previous) { OnCastStateChanged.Broadcast(Previous, CastingState); }

	float StartCast(UCombatAbility* Ability, const int32 PredictionID = 0);
	void EndCast();
	void UpdateCastFromServerResult(const float PredictionTime, const FServerAbilityResult& Result);
	
	TConditionalModifierList<FAbilityModCondition> CastLengthMods;

//Global Cooldown

public:

	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsGlobalCooldownActive() const { return GlobalCooldownState.bActive; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float GetCurrentGlobalCooldownLength() const { return GlobalCooldownState.bActive ? GlobalCooldownState.Length : -1.0f; }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float GetGlobalCooldownTimeRemaining() const { return GlobalCooldownState.bActive ? FMath::Max(0.0f, (GlobalCooldownState.StartTime + GlobalCooldownState.Length) - GameStateRef->GetServerWorldTimeSeconds()) : -1.0f; }
	UPROPERTY(BlueprintAssignable)
	FGlobalCooldownNotification OnGlobalCooldownChanged;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddGlobalCooldownModifier(const FAbilityModCondition& Modifier) { GlobalCooldownMods.Add(Modifier); }
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveGlobalCooldownModifier(const FAbilityModCondition& Modifier) { GlobalCooldownMods.Remove(Modifier); }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float CalculateGlobalCooldownLength(UCombatAbility* Ability) const;

private:
	
	FGlobalCooldown GlobalCooldownState;
	float StartGlobalCooldown(UCombatAbility* Ability, const int32 PredictionID = 0);
	FTimerHandle GlobalCooldownHandle;
	UFUNCTION()
	void EndGlobalCooldown();
	void UpdateGlobalCooldownFromServerResult(const float PredictionTime, const FServerAbilityResult& Result);
	
	TConditionalModifierList<FAbilityModCondition> GlobalCooldownMods;
	
//Cooldown

public:

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddCooldownModifier(const FAbilityModCondition& Modifier) { CooldownMods.Add(Modifier); }
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveCooldownModifier(const FAbilityModCondition& Modifier) { CooldownMods.Remove(Modifier); }
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float CalculateCooldownLength(UCombatAbility* Ability, const bool bIgnoreGlobalMin = false) const;

private:
	
	TConditionalModifierList<FAbilityModCondition> CooldownMods;

//Cost

public:
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	FCombatModifierHandle AddGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifier& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void RemoveGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifierHandle& ModifierHandle);
	void UpdateGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifierHandle& ModifierHandle, const FCombatModifier& Modifier);

private:

	TMap<FCombatModifierHandle, FResourceCostModMap> GenericResourceCostMods;
	void ApplyGenericCostModsToNewAbility(UCombatAbility* NewAbility);

#pragma region Debug

private:

	UPROPERTY()
	UCombatDebugOptions* CombatDebugOptions = nullptr;

#pragma endregion 
};