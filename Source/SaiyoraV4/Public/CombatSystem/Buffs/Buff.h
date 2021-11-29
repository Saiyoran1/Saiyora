#pragma once
#include "BuffHandler.h"
#include "BuffStructs.h"
#include "GameplayTagContainer.h"
#include "SaiyoraCombatLibrary.h"
#include "GameFramework/GameStateBase.h"
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UBuffHandler* GetHandler() const { return Handler; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	AActor* GetAppliedBy() const { return CreationEvent.AppliedBy; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	AActor* GetAppliedTo() const { return CreationEvent.AppliedTo; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UObject* GetSource() const { return CreationEvent.Source; }

private:

	UPROPERTY()
	AGameStateBase* GameStateRef;
	UPROPERTY()
	UBuffHandler* Handler;

//Info

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	FName GetBuffName() const { return Name; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UTexture2D* GetBuffIcon() const { return Icon; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	FText GetBuffDescription() const { return Description; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	FLinearColor GetBuffProgressColor() const { return ProgressColor; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
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
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool IsDuplicable() const { return bDuplicable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool IsStackable() const { return bStackable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool IsRefreshable() const { return bRefreshable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool HasFiniteDuration() const { return bFiniteDuration; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetBuffTags(FGameplayTagContainer& OutContainer) const { OutContainer.AppendTags(BuffTags); }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	int32 GetInitialStacks() const { return CreationEvent.NewStacks; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetInitialDuration() const { return bFiniteDuration ? CreationEvent.NewDuration : 0.0f; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetInitialApplyTime() const { return CreationEvent.NewApplyTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetInitialExpireTime() const { return bFiniteDuration ? (GetInitialApplyTime() + CreationEvent.NewDuration) : 0.0f; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool WasInitiallyAppliedXPlane() const { return CreationEvent.AppliedXPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetInitialOriginPlane() const { return CreationEvent.OriginPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetInitialTargetPlane() const { return CreationEvent.TargetPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetInitialParameters(TArray<FCombatParameter>& OutParams) const { OutParams.Append(CreationEvent.CombatParams); }
	void InitializeBuff(FBuffApplyEvent& Event, UBuffHandler* NewHandler, EBuffApplicationOverrideType const StackOverrideType,
		int32 const OverrideStacks, EBuffApplicationOverrideType const RefreshOverrideType, float const OverrideDuration);
	void ApplyEvent(FBuffApplyEvent& ApplicationEvent, EBuffApplicationOverrideType const StackOverrideType,
		int32 const OverrideStacks, EBuffApplicationOverrideType const RefreshOverrideType, float const OverrideDuration);

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
	UPROPERTY(ReplicatedUsing = OnRep_CreationEvent)
	FBuffApplyEvent CreationEvent;
	UFUNCTION()
	void OnRep_CreationEvent();

//Expiration

public:

	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToBuffRemoved(FBuffRemoveCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromBuffRemoved(FBuffRemoveCallback const& Callback);
	FBuffRemoveEvent TerminateBuff(EBuffExpireReason const TerminationReason);

private:

	UPROPERTY(ReplicatedUsing = OnRep_RemovalReason)
	EBuffExpireReason RemovalReason;
	UFUNCTION()
	void OnRep_RemovalReason();
	FTimerHandle ExpireHandle;
	void ResetExpireTimer();
	void CompleteExpireTimer();
	FBuffRemoveNotification OnRemoved;

//Status

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	EBuffStatus GetBuffStatus() const { return Status; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	int32 GetCurrentStacks() const { return CurrentStacks; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetRemainingTime() const { return bFiniteDuration ? FMath::Max(0.0f, ExpireTime - GameStateRef->GetServerWorldTimeSeconds()) : 0.0f; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetLastApplicationTime() const { return LastApplyTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetExpirationTime() const { return ExpireTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool WasLastAppliedXPlane() const { return LastApplyEvent.AppliedXPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetLastOriginPlane() const { return LastApplyEvent.OriginPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	ESaiyoraPlane GetLastTargetPlane() const { return LastApplyEvent.TargetPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetLastApplicationParameters(TArray<FCombatParameter>& OutParams) const { OutParams.Append(LastApplyEvent.CombatParams); }

	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void SubscribeToBuffUpdated(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void UnsubscribeFromBuffUpdated(FBuffEventCallback const& Callback);

private:

	EBuffStatus Status = EBuffStatus::Spawning;
	int32 CurrentStacks = 0;
	float LastApplyTime = 0.0f;
	float ExpireTime = 0.0f;
	UPROPERTY(ReplicatedUsing = OnRep_LastApplyEvent)
	FBuffApplyEvent LastApplyEvent;
	UFUNCTION()
	void OnRep_LastApplyEvent();
	//This delegate is for stacking and refreshing, for non-FCombatModifier functionality.
	FBuffEventNotification OnUpdated;

//Buff Functions

public:

	void RegisterBuffFunction(UBuffFunction* NewFunction) { if (Status == EBuffStatus::Spawning) { BuffFunctions.AddUnique(NewFunction); }}

protected:

	UFUNCTION(BlueprintNativeEvent)
	void SetupCommonBuffFunctions();
	virtual void SetupCommonBuffFunctions_Implementation() {}
	UFUNCTION(BlueprintNativeEvent)
	void OnApply(FBuffApplyEvent const& Event);
	virtual void OnApply_Implementation(FBuffApplyEvent const& Event) {}
	UFUNCTION(BlueprintNativeEvent)
	void OnRemove(FBuffRemoveEvent const& Event);
	virtual void OnRemove_Implementation(FBuffRemoveEvent const& Event) {}
	
private:
	
	UPROPERTY()
	TArray<UBuffFunction*> BuffFunctions;
	//Delegates here are only for FCombatModifiers that buff functions apply.
	FModifierNotification ModifierStack;
	FModifierNotification ModifierInvalidate;
};