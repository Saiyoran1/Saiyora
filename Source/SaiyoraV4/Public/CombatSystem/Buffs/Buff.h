#pragma once
#include "AbilityFunctionLibrary.h"
#include "BuffStructs.h"
#include "InstancedStruct.h"
#include "GameFramework/GameState.h"
#include "Buff.generated.h"

class UBuffFunction;
class UBuffHandler;

//Represents any kind of persistent effect applied to an actor
UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UBuff : public UObject
{
	GENERATED_BODY()

#pragma region Init and Refs

public:
	
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintPure, Category = "Buff")
	UBuffHandler* GetHandler() const { return Handler; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	AActor* GetAppliedBy() const { return CreationEvent.AppliedBy; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	AActor* GetAppliedTo() const { return CreationEvent.AppliedTo; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	UObject* GetSource() const { return CreationEvent.Source; }

private:

	UPROPERTY()
	AGameState* GameStateRef;
	UPROPERTY()
	UBuffHandler* Handler;

#pragma endregion 
#pragma region Display Info

public:

	UFUNCTION(BlueprintPure, Category = "Buff")
	FName GetBuffName() const { return Name; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	UTexture2D* GetBuffIcon() const { return Icon; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	FText GetBuffDescription() const { return Description; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	FLinearColor GetBuffProgressColor() const { return ProgressColor; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	EBuffType GetBuffType() const { return BuffType; }

	bool ShouldDisplayOnPlayerHUD() const;
	bool ShouldDisplayOnNameplate() const;
	bool ShouldDisplayOnPartyFrame() const;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FName Name;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	UTexture2D* Icon = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FText Description;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FLinearColor ProgressColor = FColor::White;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	EBuffType BuffType = EBuffType::Buff;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	EBuffDisplayOption PlayerHUDDisplay = EBuffDisplayOption::Display;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	EBuffDisplayOption NameplateDisplay = EBuffDisplayOption::DisplayOnlyFromSelf;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	EBuffDisplayOption PartyFrameDisplay = EBuffDisplayOption::Display;

#pragma endregion
#pragma region Application

public:

	//Gets whether multiple buffs of this class can be applied by the same actor as separate instances
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool IsDuplicable() const { return bDuplicable; }
	//Gets whether this buff can stack on itself with subsequent applications
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool IsStackable() const { return bStackable; }
	//Gets the max number of stacks this buff can have
	UFUNCTION(BlueprintPure, Category = "Buff")
	int32 GetMaxStacks() const { return MaximumStacks; }
	//Gets whether this buff's duration can be extended by subsequent applications
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool IsRefreshable() const { return bRefreshable; }
	//Gets whether this buff expires after a finite duration, or lasts until it is manually removed
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool HasFiniteDuration() const { return bFiniteDuration; }
	//Gets the max duration this buff can have at any given time
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetMaxDuration() const { return MaximumDuration; }
	//Gets the tags that describe this buff
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffTags(FGameplayTagContainer& OutContainer) const { OutContainer = BuffTags; }
	//Gets whether this buff can be applied to dead targets, and whether it will remain on targets that die
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool CanBeAppliedWhileDead() const { return bIgnoreDeath; }
	//Gets whether this buff will remain on NPC targets that leave combat
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool IsRemovedOnCombatEnd() const { return bRemoveOnCombatEnd; }
	//Gets the stack count this buff had on its first application
	UFUNCTION(BlueprintPure, Category = "Buff")
	int32 GetInitialStacks() const { return CreationEvent.NewStacks; }
	//Gets the duration of the buff when it was first applied, or 0.0f if infinite duration
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetInitialDuration() const { return bFiniteDuration ? CreationEvent.NewDuration : 0.0f; }
	//Gets the timestamp of the buff's initial application
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetInitialApplyTime() const { return CreationEvent.NewApplyTime; }
	//Gets the timestamp of when the buff would have expired after its first application, or 0.0f if inifinite duration
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetInitialExpireTime() const { return bFiniteDuration ? (GetInitialApplyTime() + CreationEvent.NewDuration) : 0.0f; }
	//Gets whether the buff's initial application was XPlane
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool WasInitiallyAppliedXPlane() const { return UAbilityFunctionLibrary::IsXPlane(CreationEvent.OriginPlane, CreationEvent.TargetPlane); }
	//Gets the plane the actor who applied this buff originally was in when applying the buff
	UFUNCTION(BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetInitialOriginPlane() const { return CreationEvent.OriginPlane; }
	//Gets the plane the target this buff was applied to was in when applying the buff
	UFUNCTION(BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetInitialTargetPlane() const { return CreationEvent.TargetPlane; }
	//Gets the array of parameters passed to the buff on its first application
	UFUNCTION(BlueprintPure, Category = "Buff", meta = (BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
	void GetInitialParameters(TArray<FInstancedStruct>& OutParams) const { OutParams = CreationEvent.CombatParams; }

	//Called by the handler after creating a new buff to set it up on the server
	void InitializeBuff(FBuffApplyEvent& Event, UBuffHandler* NewHandler, const bool bIgnoreRestrictions, const EBuffApplicationOverrideType StackOverrideType,
		const int32 OverrideStacks, const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration);
	//Called by the handler on subsequent applications to try and stack or refresh this buff on the server
	void ApplyEvent(FBuffApplyEvent& ApplicationEvent, EBuffApplicationOverrideType const StackOverrideType,
		const int32 OverrideStacks, const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration);

private:

	//Global minimum buff duration allowed during application
	static constexpr float MinimumBuffDuration = 0.1f;

	//Whether buffs of the same class from the same owner spawn multiple instances
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bDuplicable = false;
	//Whether subsequent applications change the stack count of this buff
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bStackable = false;
	//The default number of stacks this buff has on its initial application
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	int32 DefaultInitialStacks = 1;
	//The default number of stacks added by each subsequent application
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	int32 DefaultStacksPerApply = 1;
	//The max stacks this buff can have
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	int32 MaximumStacks = 1;
	//Whether subsequent applications can affect this buff's duration
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bRefreshable = false;
	//Whether this buff has a finite duration, or lasts until manually removed
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bFiniteDuration = true;
	//The default duration of the buff on its initial application
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	float DefaultInitialDuration = 1.0f;
	//The default amount of time to add to the duration on each subsequent application
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	float DefaultDurationPerApply = 1.0f;
	//The maximum duration the buff can have at any given time (NOT the max time the buff can exist total)
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	float MaximumDuration = 1.0f;
	//Tags to describe the buff
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	FGameplayTagContainer BuffTags;
	//Whether the buff can be applied to dead targets and remains on targets that die
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bIgnoreDeath = false;
	//Whether the buff is removed from NPCs when they leave combat
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bRemoveOnCombatEnd = true;
	//The application event for the initial application of this buff instance
	UPROPERTY(ReplicatedUsing = OnRep_CreationEvent)
	FBuffApplyEvent CreationEvent;
	//Used on clients to initialize the buff
	UFUNCTION()
	void OnRep_CreationEvent();
	//Whether this buff ignores buff restrictions that would remove it after its initial application
	bool bIgnoringRestrictions = false;

#pragma endregion
#pragma region Removal

public:

	//Multicast delegate fired when the buff is being removed
	UPROPERTY(BlueprintAssignable)
	FBuffRemoveNotification OnRemoved;
	//Called by the handler when removing the buff, or by the buff itself on expiration, or on clients when the removal reason has replicated down
	//Handles alerting handlers of buff removal, cleaning up buff functions, and firing delegates
	//Can fail if ignoring restrictions and the termination reason is something ignorable (like a dispel)
	FBuffRemoveEvent TerminateBuff(const EBuffExpireReason TerminationReason);

private:

	//The reason this buff was removed
	UPROPERTY(ReplicatedUsing = OnRep_RemovalReason)
	EBuffExpireReason RemovalReason;
	//Used for clients to call TerminateBuff locally and clean themselves up
	UFUNCTION()
	void OnRep_RemovalReason();
	//Timer handle for expiration from the buff's duration
	FTimerHandle ExpireHandle;
	//When the buff's duration changes, this is called to reset the timer
	void ResetExpireTimer();
	//Called when the buff times out on the server to terminate itself
	void CompleteExpireTimer();

#pragma endregion
#pragma region State

public:

	//Get the current stacks of the buff
	UFUNCTION(BlueprintPure, Category = "Buff")
	int32 GetCurrentStacks() const { return CurrentStacks; }
	//Get the current duration of the buff, or 0.0f if infinite
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetRemainingTime() const { return bFiniteDuration ? FMath::Max(0.0f, ExpireTime - GameStateRef->GetServerWorldTimeSeconds()) : 0.0f; }
	//Get the timestamp of the last refresh of the buff's duration
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetLastRefreshTime() const { return LastRefreshTime; }
	//Get the timestamp this buff will expire at
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetExpirationTime() const { return ExpireTime; }
	//Get whether the last application event for this buff was XPlane
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool WasLastAppliedXPlane() const { return LastApplyEvent.ActionTaken == EBuffApplyAction::Failed
		? UAbilityFunctionLibrary::IsXPlane(CreationEvent.OriginPlane, CreationEvent.TargetPlane)
		: UAbilityFunctionLibrary::IsXPlane(LastApplyEvent.OriginPlane, LastApplyEvent.TargetPlane); }
	//Get the plane the actor who applied this buff was in during the last application event
	UFUNCTION(BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetLastOriginPlane() const { return LastApplyEvent.ActionTaken == EBuffApplyAction::Failed ? CreationEvent.OriginPlane : LastApplyEvent.OriginPlane; }
	//Get the plane the actor this buff is applied to was in during the last application event
	UFUNCTION(BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetLastTargetPlane() const { return LastApplyEvent.ActionTaken == EBuffApplyAction::Failed ? CreationEvent.TargetPlane : LastApplyEvent.TargetPlane; }
	//Get the custom parameters passed in to the buff in the last application event
	UFUNCTION(BlueprintPure, Category = "Buff", meta = (BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
	void GetLastApplicationParameters(TArray<FInstancedStruct>& OutParams) const { LastApplyEvent.ActionTaken == EBuffApplyAction::Failed ? OutParams = CreationEvent.CombatParams : OutParams = LastApplyEvent.CombatParams; }

	//Multicast delegate called during each non-initial application event
	UPROPERTY(BlueprintAssignable)
	FBuffEventNotification OnUpdated;

private:

	//State machine control for the buff, might be wholly unnecessary. Used to gate client init and guard against double init
	EBuffStatus Status = EBuffStatus::Spawning;
	int32 CurrentStacks = 0;
	float LastRefreshTime = 0.0f;
	float ExpireTime = 0.0f;
	//Application event of the most recent application
	UPROPERTY(ReplicatedUsing = OnRep_LastApplyEvent)
	FBuffApplyEvent LastApplyEvent;
	//Used by clients to update the buff locally after application events
	UFUNCTION()
	void OnRep_LastApplyEvent();

#pragma endregion
#pragma region Buff Functions

protected:

	//Blueprint-exposed event to setup and pass parameters to any common buff functions this buff should use
	UFUNCTION(BlueprintNativeEvent)
	void SetupCommonBuffFunctions();
	virtual void SetupCommonBuffFunctions_Implementation() {}
	//Blueprint-exposed event for performing logic on each application of this buff
	UFUNCTION(BlueprintNativeEvent)
	void OnApply(const FBuffApplyEvent& Event);
	virtual void OnApply_Implementation(const FBuffApplyEvent& Event) {}
	//Blueprint-exposed event for performing logic when this buff is removed
	UFUNCTION(BlueprintNativeEvent)
	void OnRemove(const FBuffRemoveEvent& Event);
	virtual void OnRemove_Implementation(const FBuffRemoveEvent& Event) {}
	
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Buff Functions", Instanced)
	TArray<UBuffFunction*> BuffFunctionObjects;

#pragma endregion 
};