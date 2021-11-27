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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	UBuffHandler* GetHandler() const { return Handler; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	AActor* GetAppliedBy() const { return AppliedBy; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	AActor* GetAppliedTo() const { return Handler->GetOwner(); }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	UObject* GetSource() const { return Source; }

private:

	UPROPERTY()
	AGameStateBase* GameStateRef;
	UPROPERTY()
	UBuffHandler* Handler;
	UPROPERTY()
	AActor* AppliedBy;
	UPROPERTY()
	UObject* Source;

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
	bool GetDuplicable() const { return bDuplicable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool GetStackable() const { return bStackable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool GetRefreshable() const { return bRefreshable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	bool GetFiniteDuration() const { return bFiniteDuration; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetBuffTags(FGameplayTagContainer& OutContainer) const { OutContainer.AppendTags(BuffTags); }
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
	void UpdateClientStateOnApply(FBuffApplyEvent const& ReplicatedEvent);
	void HandleBuffApplyEventReplication(FBuffApplyEvent const& ReplicatedEvent);

//Expiration

public:

	void ExpireBuff(FBuffRemoveEvent const & RemoveEvent);

private:

	FTimerHandle ExpireHandle;
	void ResetExpireTimer();
	void CompleteExpireTimer();
	void HandleBuffRemoveEventReplication(FBuffRemoveEvent const& ReplicatedEvent);

//Status

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	int32 GetCurrentStacks() const { return CurrentStacks; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetRemainingTime() const { return bFiniteDuration ? FMath::Max(0.0f, ExpireTime - GameStateRef->GetServerWorldTimeSeconds()) : 0.0f; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetApplicationTime() const { return LastApplyTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	float GetExpirationTime() const { return ExpireTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	EBuffStatus GetBuffStatus() const { return Status; }

	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void SubscribeToBuffUpdated(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void UnsubscribeFromBuffUpdated(FBuffEventCallback const& Callback);

private:

	int32 CurrentStacks = 0;
	float LastApplyTime = 0.0f;
	float ExpireTime = 0.0f;
	
	EBuffStatus Status = EBuffStatus::Spawning;
	
	UPROPERTY(ReplicatedUsing = OnRep_CreationEvent)
	FBuffApplyEvent CreationEvent;
	UFUNCTION()
	void OnRep_CreationEvent();
	UPROPERTY(ReplicatedUsing = OnRep_LastApplyEvent)
	FBuffApplyEvent LastApplyEvent;
	UFUNCTION()
	void OnRep_LastApplyEvent();
	UPROPERTY(ReplicatedUsing = OnRep_RemovingEvent)
	FBuffRemoveEvent RemovingEvent;
	UFUNCTION()
	void OnRep_RemovingEvent();

	FBuffEventNotification OnUpdated;

//Buff Functions

public:

	void RegisterBuffFunction(UBuffFunction* NewFunction) { BuffFunctions.AddUnique(NewFunction); }

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void BuffFunctionality();
	
private:
	
	UPROPERTY()
	TArray<UBuffFunction*> BuffFunctions;
	//Delegates here are only for FCombatModifiers that buff functions apply.
	FModifierNotification OnStacked;
	FModifierNotification OnRemoved;

public:
/*
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	FBuffApplyEvent GetCreationEvent() const { return CreationEvent; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	FBuffApplyEvent GetLastApplyEvent() const { return LastApplyEvent; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    int32 GetInitialStacks() const { return CreationEvent.NewStacks; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    float GetInitialApplyTime() const { return CreationEvent.NewApplyTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    float GetInitialExpireTime() const { return bFiniteDuration ? (GetInitialApplyTime() + CreationEvent.NewDuration) : 0.0f; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    bool GetAppliedXPlane() const { return CreationEvent.AppliedXPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    ESaiyoraPlane GetAppliedByPlane() const { return CreationEvent.AppliedByPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    ESaiyoraPlane GetAppliedToPlane() const { return CreationEvent.AppliedToPlane; }
*/
};