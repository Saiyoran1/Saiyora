#pragma once
#include "AbilityFunctionLibrary.h"
#include "BuffStructs.h"
#include "InstancedStruct.h"
#include "GameFramework/GameState.h"
#include "Buff.generated.h"

class UBuffFunction;
class UBuffHandler;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UBuff : public UObject
{
	GENERATED_BODY()

public:

	static const float MinimumBuffDuration;
	
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

//Info

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

private:

	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FName Name;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	UTexture2D* Icon;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FText Description;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FLinearColor ProgressColor;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	EBuffType BuffType = EBuffType::Buff;	

//Application

public:
	
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool IsDuplicable() const { return bDuplicable; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool IsStackable() const { return bStackable; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	int32 GetMaxStacks() const { return MaximumStacks; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool IsRefreshable() const { return bRefreshable; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool HasFiniteDuration() const { return bFiniteDuration; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetMaxDuration() const { return MaximumDuration; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffTags(FGameplayTagContainer& OutContainer) const { OutContainer = BuffTags; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool CanBeAppliedWhileDead() const { return bIgnoreDeath; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool IsRemovedOnCombatEnd() const { return bRemoveOnCombatEnd; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	int32 GetInitialStacks() const { return CreationEvent.NewStacks; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetInitialDuration() const { return bFiniteDuration ? CreationEvent.NewDuration : 0.0f; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetInitialApplyTime() const { return CreationEvent.NewApplyTime; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetInitialExpireTime() const { return bFiniteDuration ? (GetInitialApplyTime() + CreationEvent.NewDuration) : 0.0f; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool WasInitiallyAppliedXPlane() const { return UAbilityFunctionLibrary::IsXPlane(CreationEvent.OriginPlane, CreationEvent.TargetPlane); }
	UFUNCTION(BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetInitialOriginPlane() const { return CreationEvent.OriginPlane; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetInitialTargetPlane() const { return CreationEvent.TargetPlane; }
	UFUNCTION(BlueprintPure, Category = "Buff", meta = (BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
	void GetInitialParameters(TArray<FInstancedStruct>& OutParams) const { OutParams = CreationEvent.CombatParams; }
	void InitializeBuff(FBuffApplyEvent& Event, UBuffHandler* NewHandler, const bool bIgnoreRestrictions, const EBuffApplicationOverrideType StackOverrideType,
		const int32 OverrideStacks, const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration);
	void ApplyEvent(FBuffApplyEvent& ApplicationEvent, EBuffApplicationOverrideType const StackOverrideType,
		const int32 OverrideStacks, const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration);

private:

	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bDuplicable = false;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bStackable = false;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	int32 DefaultInitialStacks = 1;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	int32 DefaultStacksPerApply = 1;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	int32 MaximumStacks = 1;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bRefreshable = false;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bFiniteDuration = true;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	float DefaultInitialDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	float DefaultDurationPerApply = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	float MaximumDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	FGameplayTagContainer BuffTags;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bIgnoreDeath = false;
	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	bool bRemoveOnCombatEnd = true;
	UPROPERTY(ReplicatedUsing = OnRep_CreationEvent)
	FBuffApplyEvent CreationEvent;
	UFUNCTION()
	void OnRep_CreationEvent();
	bool bIgnoringRestrictions = false;

//Expiration

public:

	UPROPERTY(BlueprintAssignable)
	FBuffRemoveNotification OnRemoved;
	FBuffRemoveEvent TerminateBuff(const EBuffExpireReason TerminationReason);

private:

	UPROPERTY(ReplicatedUsing = OnRep_RemovalReason)
	EBuffExpireReason RemovalReason;
	UFUNCTION()
	void OnRep_RemovalReason();
	FTimerHandle ExpireHandle;
	void ResetExpireTimer();
	void CompleteExpireTimer();

//Status

public:
	
	UFUNCTION(BlueprintPure, Category = "Buff")
	int32 GetCurrentStacks() const { return CurrentStacks; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetRemainingTime() const { return bFiniteDuration ? FMath::Max(0.0f, ExpireTime - GameStateRef->GetServerWorldTimeSeconds()) : 0.0f; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetLastRefreshTime() const { return LastRefreshTime; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	float GetExpirationTime() const { return ExpireTime; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	bool WasLastAppliedXPlane() const { return LastApplyEvent.ActionTaken == EBuffApplyAction::Failed
		? UAbilityFunctionLibrary::IsXPlane(CreationEvent.OriginPlane, CreationEvent.TargetPlane)
		: UAbilityFunctionLibrary::IsXPlane(LastApplyEvent.OriginPlane, LastApplyEvent.TargetPlane); }
	UFUNCTION(BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetLastOriginPlane() const { return LastApplyEvent.ActionTaken == EBuffApplyAction::Failed ? CreationEvent.OriginPlane : LastApplyEvent.OriginPlane; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetLastTargetPlane() const { return LastApplyEvent.ActionTaken == EBuffApplyAction::Failed ? CreationEvent.TargetPlane : LastApplyEvent.TargetPlane; }
	UFUNCTION(BlueprintPure, Category = "Buff", meta = (BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
	void GetLastApplicationParameters(TArray<FInstancedStruct>& OutParams) const { LastApplyEvent.ActionTaken == EBuffApplyAction::Failed ? OutParams = CreationEvent.CombatParams : OutParams = LastApplyEvent.CombatParams; }

	UPROPERTY(BlueprintAssignable)
	FBuffEventNotification OnUpdated;

private:

	EBuffStatus Status = EBuffStatus::Spawning;
	int32 CurrentStacks = 0;
	float LastRefreshTime = 0.0f;
	float ExpireTime = 0.0f;
	UPROPERTY(ReplicatedUsing = OnRep_LastApplyEvent)
	FBuffApplyEvent LastApplyEvent;
	UFUNCTION()
	void OnRep_LastApplyEvent();

//Buff Functions

public:

	void RegisterBuffFunction(UBuffFunction* NewFunction) { if (Status == EBuffStatus::Spawning) { BuffFunctions.AddUnique(NewFunction); }}

protected:

	UFUNCTION(BlueprintNativeEvent)
	void SetupCommonBuffFunctions();
	virtual void SetupCommonBuffFunctions_Implementation() {}
	UFUNCTION(BlueprintNativeEvent)
	void OnApply(const FBuffApplyEvent& Event);
	virtual void OnApply_Implementation(const FBuffApplyEvent& Event) {}
	UFUNCTION(BlueprintNativeEvent)
	void OnRemove(const FBuffRemoveEvent& Event);
	virtual void OnRemove_Implementation(const FBuffRemoveEvent& Event) {}
	
private:
	
	UPROPERTY()
	TArray<UBuffFunction*> BuffFunctions;
};