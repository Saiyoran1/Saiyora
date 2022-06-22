#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "CrowdControlStructs.h"
#include "DamageStructs.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CrowdControlHandler.generated.h"

class UBuffHandler;
class UDamageHandler;

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
	UBuffHandler* BuffHandler = nullptr;
	UPROPERTY()
	UDamageHandler* DamageHandler = nullptr;

//Status

public:
	
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	bool IsCrowdControlActive(const FGameplayTag CcTag) const;
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	FCrowdControlStatus GetCrowdControlStatus(const FGameplayTag CcTag) const { return *GetCcStructConst(CcTag); }
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	void GetActiveCrowdControls(FGameplayTagContainer& OutCcs) const;

	UPROPERTY(BlueprintAssignable)
	FCrowdControlNotification OnCrowdControlChanged;
	
private:

	FCrowdControlStatus* GetCcStruct(const FGameplayTag CcTag);
	FCrowdControlStatus const* GetCcStructConst(const FGameplayTag CcTag) const;
	UPROPERTY(ReplicatedUsing = OnRep_StunStatus)
	FCrowdControlStatus StunStatus;
	UFUNCTION()
	void OnRep_StunStatus(const FCrowdControlStatus& Previous);
	UPROPERTY(ReplicatedUsing = OnRep_IncapStatus)
	FCrowdControlStatus IncapStatus;
	UFUNCTION()
	void OnRep_IncapStatus(const FCrowdControlStatus& Previous);
	UPROPERTY(ReplicatedUsing = OnRep_RootStatus)
	FCrowdControlStatus RootStatus;
	UFUNCTION()
	void OnRep_RootStatus(const FCrowdControlStatus& Previous);
	UPROPERTY(ReplicatedUsing = OnRep_SilenceStatus)
	FCrowdControlStatus SilenceStatus;
	UFUNCTION()
	void OnRep_SilenceStatus(const FCrowdControlStatus& Previous);
	UPROPERTY(ReplicatedUsing = OnRep_DisarmStatus)
	FCrowdControlStatus DisarmStatus;
	UFUNCTION()
	void OnRep_DisarmStatus(const FCrowdControlStatus& Previous);
	UFUNCTION()
	void CheckAppliedBuffForCc(const FBuffApplyEvent& BuffEvent);
	UFUNCTION()
	void CheckRemovedBuffForCc(const FBuffRemoveEvent& RemoveEvent);
	UFUNCTION()
	void RemoveIncapacitatesOnDamageTaken(const FDamagingEvent& DamageEvent);

//Immunities

public:

	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	bool IsImmuneToCrowdControl(const FGameplayTag CcTag) const { return DefaultCrowdControlImmunities.HasTagExact(CcTag); }
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	void GetImmunedCrowdControls(FGameplayTagContainer& OutImmunes) const { OutImmunes = DefaultCrowdControlImmunities; }

private:
	
	UPROPERTY(EditAnywhere, Category = "Crowd Control", meta = (Categories = "CrowdControl"))
	FGameplayTagContainer DefaultCrowdControlImmunities;
};