// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "BuffStructs.h"
#include "GameplayTagContainer.h"
#include "SaiyoraCombatLibrary.h"
#include "Buff.generated.h"

class UBuffFunction;

UCLASS(Blueprintable, Abstract)
class SAIYORAV4_API UBuff : public UObject
{
	GENERATED_BODY()

#pragma region Variables
	
private:

	//Display Info
	//Basic information needed for UI display.
	
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FName Name;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	UTexture2D* Icon;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FText Description;
	UPROPERTY(EditDefaultsOnly, Category = "Display Information")
	FLinearColor ProgressColor;
	
	//Pre-Application Info
	//Parameters that control how a buff can be applied and its behavior for subsequent applications.

	UPROPERTY(EditDefaultsOnly, Category = "Application Behavior")
	EBuffType BuffType = EBuffType::Buff;
	
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
	static const float MinimumBuffDuration;
	
	//Post-Application Info
	//The results of recent apply and remove events.

	//The OnRep from this property will be used to initialize client variables, rather than replicating everything twice.
	UPROPERTY(ReplicatedUsing = OnRep_CreationEvent)
	FBuffApplyEvent CreationEvent;
	//Do not use this property for the initial event. Otherwise you will get multiple OnRep calls.
	UPROPERTY(ReplicatedUsing = OnRep_LastApplyEvent)
	FBuffApplyEvent LastApplyEvent;
	UPROPERTY(ReplicatedUsing = OnRep_RemovingEvent)
	FBuffRemoveEvent RemovingEvent;
	
	//State Info
	//Current state of the buff.

	int32 CurrentStacks = 0;
	float LastApplyTime = 0.0f;
	float ExpireTime = 0.0f;
	FTimerHandle ExpireHandle;
	EBuffStatus Status = EBuffStatus::Spawning;
	
	//Functionality
	//Class references to the different functions this buff will instantiate and activate.

	UPROPERTY(EditDefaultsOnly, Category = "Functionality")
	TArray<TSubclassOf<UBuffFunction>> ServerFunctionClasses;
	UPROPERTY(EditDefaultsOnly, Category = "Functionality")
	TArray<TSubclassOf<UBuffFunction>> ClientFunctionClasses;
	UPROPERTY()
	TArray<UBuffFunction*> Functions;

#pragma endregion
	
#pragma region Getters

public:
	
	//Getters for BuffContainer to use during application.

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	EBuffType GetBuffType() const { return BuffType; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	bool GetDuplicable() const { return bDuplicable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	bool GetStackable() const { return bStackable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	bool GetRefreshable() const { return bRefreshable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	void GetServerFunctionTags(FGameplayTagContainer& OutContainer) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    void GetClientFunctionTags(FGameplayTagContainer& OutContainer) const;

	//Getters for basic information.
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	FName GetBuffName() const { return Name; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	UTexture2D* GetBuffIcon() const { return Icon; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	FText GetBuffDescription() const { return Description; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	FLinearColor GetBuffProgressColor() const { return ProgressColor; }
	
	//Getters for current state.
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	int32 GetCurrentStacks() const { return CurrentStacks; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	bool GetFiniteDuration() const { return bFiniteDuration; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	float GetRemainingTime() const { return bFiniteDuration ? FMath::Max(0.0f, ExpireTime - USaiyoraCombatLibrary::GetWorldTime(this)) : 0.0f; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	float GetApplicationTime() const { return LastApplyTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	float GetExpirationTime() const { return ExpireTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	EBuffStatus GetBuffStatus() const { return Status; }

	//Application Info Getters

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	FBuffApplyEvent GetCreationEvent() const { return CreationEvent; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	FBuffApplyEvent GetLastApplyEvent() const { return LastApplyEvent; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    int32 GetInitialStacks() const { return CreationEvent.Result.NewStacks; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    float GetInitialApplyTime() const { return CreationEvent.Result.NewApplyTime; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    float GetInitialExpireTime() const { return bFiniteDuration ? (GetInitialApplyTime() + CreationEvent.Result.NewDuration) : 0.0f; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    bool GetAppliedXPlane() const { return CreationEvent.AppliedXPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    ESaiyoraPlane GetAppliedByPlane() const { return CreationEvent.AppliedByPlane; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    ESaiyoraPlane GetAppliedToPlane() const { return CreationEvent.AppliedToPlane; }

	//Reference Getters
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	AActor* GetAppliedBy() const { return CreationEvent.AppliedBy; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    AActor* GetAppliedTo() const { return CreationEvent.AppliedTo; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    UObject* GetSource() const { return CreationEvent.Source; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
    TArray<TSubclassOf<UBuffFunction>> GetServerFunctionClasses() const { return ServerFunctionClasses; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	TArray<TSubclassOf<UBuffFunction>> GetClientFunctionClasses() const { return ClientFunctionClasses; }

#pragma endregion

#pragma region Application/Expiration

	//Application and Expiration Functions

	void InitializeBuff(FBuffApplyEvent& ApplicationEvent);
	void ApplyEvent(FBuffApplyEvent& ApplicationEvent);
	void ExpireBuff(FBuffRemoveEvent const & RemoveEvent);
	
private:

	//Stacking
	void SetInitialStacks(EBuffApplicationOverrideType const OverrideType, int32 const OverrideStacks);
	bool StackBuff(EBuffApplicationOverrideType const OverrideType, int32 const OverrideStacks);
	void UpdateStacksNoOverride();
	void UpdateStacksWithOverride(int32 const OverrideStacks);
	void UpdateStacksWithAdditiveOverride(int32 const OverrideStacks);
	void NotifyFunctionsOfStack(FBuffApplyEvent const & ApplyEvent);

	//Refreshing
	
	void SetInitialDuration(EBuffApplicationOverrideType const OverrideType, float const OverrideDuration);
	bool RefreshBuff(EBuffApplicationOverrideType const OverrideType, float const OverrideDuration);
	void UpdateDurationNoOverride();
	void UpdateDurationWithOverride(float const OverrideDuration);
	void UpdateDurationWithAdditiveOverride(float const OverrideDuration);
	void NotifyFunctionsOfRefresh(FBuffApplyEvent const & ApplyEvent);

	void ClearExpireTimer();
	void ResetExpireTimer(float const TimerLength);
	void CompleteExpireTimer();

#pragma endregion

#pragma region Replication

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CreationEvent();
	UFUNCTION()
	void OnRep_LastApplyEvent();
	UFUNCTION()
	void OnRep_RemovingEvent();

	void UpdateClientStateOnApply(FBuffApplyEvent const& ReplicatedEvent);
	void HandleBuffApplyEventReplication(FBuffApplyEvent const& ReplicatedEvent);
	void HandleBuffRemoveEventReplication(FBuffRemoveEvent const& ReplicatedEvent);

#pragma endregion 
	
};