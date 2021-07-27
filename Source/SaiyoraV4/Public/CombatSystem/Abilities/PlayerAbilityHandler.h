// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "CombatSystem/Abilities/AbilityHandler.h"
#include "PlayerAbilityHandler.generated.h"

class UPlayerCombatAbility;
class UPlayerAbilitySave;
class UPlayerSpecialization;

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API UPlayerAbilityHandler : public UAbilityHandler
{
	GENERATED_BODY()

//Setup
public:
	static const float MaxPingCompensation;
	static int32 GetAbilitiesPerBar() { return AbilitiesPerBar; }
	UPlayerAbilityHandler();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	EActorNetPermission GetAbilityPermission() const { return AbilityPermission; }
private:
	static const int32 AbilitiesPerBar = 6;
	static const float AbilityQueWindowSec;
	static int32 ClientPredictionID;
	int32 GenerateNewPredictionID();
	EActorNetPermission AbilityPermission = EActorNetPermission::None;
//Ability Management
public:
	UPlayerCombatAbility* FindActiveAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass);
	virtual UCombatAbility* AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass) override;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void LearnAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void UnlearnAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass);
	/*UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UpdateAbilityBind(TSubclassOf<UCombatAbility> const Ability, int32 const Bind, EActionBarType const Bar);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FPlayerAbilityLoadout GetPlayerLoadout() const { return CurrentLoadout; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	TSet<TSubclassOf<UCombatAbility>> GetUnlockedAbilities() const { return Spellbook; }*/
	UFUNCTION(BlueprintCallable)
	void SubscribeToAbilityBindUpdated(FAbilityBindingCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromAbilityBindUpdated(FAbilityBindingCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void SubscribeToBarSwap(FBarSwapCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromBarSwap(FBarSwapCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void SubscribeToSpellbookUpdated(FSpellbookCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromSpellbookUpdated(FSpellbookCallback const& Callback);
private:
	/*FPlayerAbilityLoadout CurrentLoadout;*/
	TSet<TSubclassOf<UCombatAbility>> Spellbook;
	EActionBarType CurrentBar = EActionBarType::None;
	FAbilityBindingNotification OnAbilityBindUpdated;
	FBarSwapNotification OnBarSwap;
	FSpellbookNotification OnSpellbookUpdated;
	virtual void SetupInitialAbilities() override;
	UFUNCTION()
	void SwapBarOnPlaneSwap(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source);
	UFUNCTION()
	bool CheckForAbilityBarRestricted(UCombatAbility* Ability);
	void RemoveAllAbilities();
//Ability Usage
public:
	/*UFUNCTION(BlueprintCallable, Category = "Abilities")
	FCastEvent AbilityInput(int32 const BindNumber, bool const bHidden);*/
	virtual FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass) override;
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToAbilityMispredicted(FAbilityMispredictionCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromAbilityMispredicted(FAbilityMispredictionCallback const& Callback);
private:
	TMap<int32, FClientAbilityPrediction> UnackedAbilityPredictions;
	FAbilityMispredictionNotification OnAbilityMispredicted;
	FCastEvent UsePlayerAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass, bool const bFromQueue = false);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPredictAbility(FAbilityRequest const& AbilityRequest);
	bool ServerPredictAbility_Validate(FAbilityRequest const& AbilityRequest) { return true; }
	UFUNCTION(Client, Reliable)
	void ClientSucceedPredictedAbility(FServerAbilityResult const& ServerResult);
	UFUNCTION(Client, Reliable)
	void ClientFailPredictedAbility(int32 const PredictionID, ECastFailReason const FailReason);
//Casting
private:
	TArray<FPredictedTick> TicksAwaitingParams;
	TMap<FPredictedTick, FCombatParameters> ParamsAwaitingTicks;
	void StartCast(UCombatAbility* Ability, int32 const PredictionID = 0);
	void PredictStartCast(UCombatAbility* Ability, int32 const PredictionID);
	void UpdatePredictedCastFromServer(FServerAbilityResult const& ServerResult);
	virtual void CompleteCast() override;
	UFUNCTION()
	void PredictAbilityTick();
	void HandleMissedPredictedTick(int32 const TickNumber);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerHandlePredictedTick(FAbilityRequest const& TickRequest);
	bool ServerHandlePredictedTick_Validate(FAbilityRequest const& TickRequest) { return true; }
	void PurgeExpiredPredictedTicks();
	UFUNCTION()
	void AuthTickPredictedCast();
//Cancelling
public:
	virtual FCancelEvent CancelCurrentCast() override;
private:
	FCancelEvent PredictCancelAbility();
	UFUNCTION(Server, WithValidation, Reliable)
	void ServerPredictCancelAbility(FCancelRequest const& CancelRequest);
	bool ServerPredictCancelAbility_Validate(FCancelRequest const& CancelRequest) { return true; }
//Interrupting
public:
	virtual FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions) override;
private:
	UFUNCTION(Client, Reliable)
	void ClientInterruptCast(FInterruptEvent const& InterruptEvent);
//Global Cooldown
private:
	void StartGlobal(UCombatAbility* Ability, bool const bPredicted = false);
	virtual void EndGlobalCooldown() override;
	void PredictStartGlobal(int32 const PredictionID);
	void UpdatePredictedGlobalFromServer(FServerAbilityResult const& ServerResult);
//Queueing
private:
	UPROPERTY()
	TSubclassOf<UPlayerCombatAbility> QueuedAbility;
	EQueueStatus QueueStatus = EQueueStatus::Empty;
	FTimerHandle QueueClearHandle;
	bool TryQueueAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass);
	UFUNCTION()
	void ClearQueue();
	void SetQueueExpirationTimer();
	void CheckForQueuedAbilityOnGlobalEnd();
	void CheckForQueuedAbilityOnCastEnd();
//Specialization
/*public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Abilities")
	void ChangeSpecialization(TSubclassOf<UPlayerSpecialization> const NewSpecialization);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SubscribeToSpecChanged(FSpecializationCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void UnsubscribeFromSpecChanged(FSpecializationCallback const& Callback);
	void NotifyOfNewSpecObject(UPlayerSpecialization* NewSpecialization);
private:
	UPROPERTY()
	UPlayerSpecialization* CurrentSpec;
	FSpecializationNotification OnSpecChanged;
	TMap<TSubclassOf<UPlayerSpecialization>, FPlayerAbilityLoadout> SpecLoadouts;
	TMap<TSubclassOf<UPlayerSpecialization>, FPlayerTalentSetup> SpecTalents;*/
};