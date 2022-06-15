#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "CrowdControlStructs.h"
#include "DamageStructs.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CrowdControlHandler.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UCrowdControlHandler : public UActorComponent
{
	GENERATED_BODY()

//Setup

public:
	
	UCrowdControlHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY()
	class UBuffHandler* BuffHandler;
	UPROPERTY()
	class UDamageHandler* DamageHandler;

//Status

public:
	
	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void SubscribeToCrowdControlChanged(FCrowdControlCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void UnsubscribeFromCrowdControlChanged(FCrowdControlCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crowd Control")
	bool IsCrowdControlActive(FGameplayTag const CcTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crowd Control")
	FCrowdControlStatus GetCrowdControlStatus(FGameplayTag const CcTag) const { return *GetCcStructConst(CcTag); }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crowd Control")
	void GetActiveCrowdControls(FGameplayTagContainer& OutCcs) const;
	
private:

	FCrowdControlStatus* GetCcStruct(FGameplayTag const CcTag);
	FCrowdControlStatus const* GetCcStructConst(FGameplayTag const CcTag) const;
	UPROPERTY(ReplicatedUsing=OnRep_StunStatus)
	FCrowdControlStatus StunStatus;
	UFUNCTION()
	void OnRep_StunStatus(FCrowdControlStatus const& Previous);
	UPROPERTY(ReplicatedUsing=OnRep_IncapStatus)
	FCrowdControlStatus IncapStatus;
	UFUNCTION()
	void OnRep_IncapStatus(FCrowdControlStatus const& Previous);
	UPROPERTY(ReplicatedUsing=OnRep_RootStatus)
	FCrowdControlStatus RootStatus;
	UFUNCTION()
	void OnRep_RootStatus(FCrowdControlStatus const& Previous);
	UPROPERTY(ReplicatedUsing=OnRep_SilenceStatus)
	FCrowdControlStatus SilenceStatus;
	UFUNCTION()
	void OnRep_SilenceStatus(FCrowdControlStatus const& Previous);
	UPROPERTY(ReplicatedUsing=OnRep_DisarmStatus)
	FCrowdControlStatus DisarmStatus;
	UFUNCTION()
	void OnRep_DisarmStatus(FCrowdControlStatus const& Previous);
	FCrowdControlNotification OnCrowdControlChanged;
	FBuffEventCallback OnBuffApplied;
	UFUNCTION()
	void CheckAppliedBuffForCc(FBuffApplyEvent const& BuffEvent);
	FBuffRemoveCallback OnBuffRemoved;
	UFUNCTION()
	void CheckRemovedBuffForCc(FBuffRemoveEvent const& RemoveEvent);
	FDamageEventCallback OnDamageTaken;
	UFUNCTION()
	void RemoveIncapacitatesOnDamageTaken(FDamagingEvent const& DamageEvent);

//Immunities

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crowd Control")
	bool IsImmuneToCrowdControl(FGameplayTag const CcTag) const { return DefaultCrowdControlImmunities.HasTagExact(CcTag); }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Crowd Control")
	void GetImmunedCrowdControls(FGameplayTagContainer& OutImmunes) const { OutImmunes = DefaultCrowdControlImmunities; }

private:
	
	UPROPERTY(EditAnywhere, Category = "Crowd Control", meta = (AllowPrivateAccess = true, Categories = "CrowdControl"))
	FGameplayTagContainer DefaultCrowdControlImmunities;
};