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

	//Called by buff functions to apply a crowd control.
	void AddCrowdControl(UBuff* Source, const ECrowdControl CcType);
	//Called by buff functions to update the duration of a crowd control.
	//This really just serves to potentially update which buff providing a given CC type will last the longest, for display.
	void RefreshCrowdControl(UBuff* Source);
	//Called on buff removal to remove any CCs provided by a buff.
	void RemoveCrowdControl(UBuff* Source);

	//Gets whether a crowd control of the given type is active
	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	bool IsCrowdControlActive(const ECrowdControl CcType) const;

	//Called any time a crowd control becomes active or inactive, or changes its dominant buff
	UPROPERTY(BlueprintAssignable)
	FCrowdControlNotification OnCrowdControlChanged;

	//Called when crowd control status changes replicate down to clients.
	void OnReplicatedCrowdControlUpdated(const FCrowdControlStatus& NewStatus)
	{
		OnCrowdControlChanged.Broadcast(NewStatus);
	}
	
private:
	
	//Getter for a crowd control status struct, mutable
	FCrowdControlStatus* GetCcStruct(const ECrowdControl CcType);
	//Getter for a crowd control status struct, const
	FCrowdControlStatus const* GetCcStructConst(const ECrowdControl CcType) const;

	UPROPERTY(Replicated)
	FCrowdControlStatusArray CurrentCrowdControls;
	
	//Incapacitate breaks on taking any damage, so we handle that with this callback
	UFUNCTION()
	void RemoveIncapacitatesOnDamageTaken(const FHealthEvent& HealthEvent);

#pragma endregion 
#pragma region Immunities

public:

	//Called by buff functions to add a temporary immunity to some CC types.
	void AddImmunities(UBuff* Source, const TSet<ECrowdControl>& Immunities);
	//Called on buff removal to remove any CC immunities provided by that buff.
	void RemoveImmunities(UBuff* Source);
	//Checks whether we are currently immune to a given CC type, either via default immunity or a buff.
	bool IsImmuneToCc(const ECrowdControl CcType) const;

private:

	//Crowd control types this actor is immune to by default
	UPROPERTY(EditAnywhere, Category = "Crowd Control")
	TSet<ECrowdControl> DefaultCrowdControlImmunities;
	
	//Removes all buffs that are applying a given type of CC.
	void DispelCcsOfType(const ECrowdControl CcType);

#pragma endregion 
};