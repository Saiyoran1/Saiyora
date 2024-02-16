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

#pragma region Init and Refs

public:
	
	UCrowdControlHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY()
	UBuffHandler* BuffHandlerRef = nullptr;
	UPROPERTY()
	UDamageHandler* DamageHandlerRef = nullptr;

#pragma endregion 
#pragma region State

public:

	//Gets whether a crowd control of the given type is active
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	bool IsCrowdControlActive(const FGameplayTag CcTag) const;
	//Get the crowd control status of a given type
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	FCrowdControlStatus GetCrowdControlStatus(const FGameplayTag CcTag) const { return *GetCcStructConst(CcTag); }
	//Get the tags of the different crowd controls that are currently active
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	void GetActiveCrowdControls(FGameplayTagContainer& OutCcs) const;

	//Called any time a crowd control becomes active or inactive, or changes its dominant buff
	UPROPERTY(BlueprintAssignable)
	FCrowdControlNotification OnCrowdControlChanged;
	
private:

	//Getter for a crowd control status struct, mutable
	FCrowdControlStatus* GetCcStruct(const FGameplayTag CcTag);
	//Getter for a crowd control status struct, const
	FCrowdControlStatus const* GetCcStructConst(const FGameplayTag CcTag) const;
	//Stun crowd control
	UPROPERTY(ReplicatedUsing = OnRep_StunStatus)
	FCrowdControlStatus StunStatus;
	UFUNCTION()
	void OnRep_StunStatus(const FCrowdControlStatus& Previous);
	//Incap crowd control
	UPROPERTY(ReplicatedUsing = OnRep_IncapStatus)
	FCrowdControlStatus IncapStatus;
	UFUNCTION()
	void OnRep_IncapStatus(const FCrowdControlStatus& Previous);
	//Root crowd control
	UPROPERTY(ReplicatedUsing = OnRep_RootStatus)
	FCrowdControlStatus RootStatus;
	UFUNCTION()
	void OnRep_RootStatus(const FCrowdControlStatus& Previous);
	//Silence crowd control
	UPROPERTY(ReplicatedUsing = OnRep_SilenceStatus)
	FCrowdControlStatus SilenceStatus;
	UFUNCTION()
	void OnRep_SilenceStatus(const FCrowdControlStatus& Previous);
	//Disarm crowd control
	UPROPERTY(ReplicatedUsing = OnRep_DisarmStatus)
	FCrowdControlStatus DisarmStatus;
	UFUNCTION()
	void OnRep_DisarmStatus(const FCrowdControlStatus& Previous);

	//Called when buffs are applied to the owning actor to check for crowd controls they are applying
	UFUNCTION()
	void CheckAppliedBuffForCc(const FBuffApplyEvent& BuffEvent);
	//Called when buffs are removed from the owning actor to check for crowd controls they were applying
	UFUNCTION()
	void CheckRemovedBuffForCc(const FBuffRemoveEvent& RemoveEvent);
	//Incapacitate breaks on taking any damage, so we handle that with this callback
	UFUNCTION()
	void RemoveIncapacitatesOnDamageTaken(const FHealthEvent& HealthEvent);

#pragma endregion 
#pragma region Immunities

public:

	//Gets whether this actor is currently immune to a given type of crowd control
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	bool IsImmuneToCrowdControl(const FGameplayTag CcTag) const { return DefaultCrowdControlImmunities.HasTagExact(CcTag); }
	//Gets the tags for all the crowd control types this actor is currently immune to
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	void GetImmunedCrowdControls(FGameplayTagContainer& OutImmunes) const { OutImmunes = DefaultCrowdControlImmunities; }

private:

	//Crowd control types this actor is immune to by default
	UPROPERTY(EditAnywhere, Category = "Crowd Control", meta = (Categories = "CrowdControl"))
	FGameplayTagContainer DefaultCrowdControlImmunities;

#pragma endregion 
};