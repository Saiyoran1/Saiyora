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

	void AddCrowdControl(UBuff* Source, const ECrowdControl CcType);
	void RefreshCrowdControl(UBuff* Source);
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

	void AddImmunities(UBuff* Source, const TSet<ECrowdControl>& Immunities);
	void RemoveImmunities(UBuff* Source);

private:

	//Crowd control types this actor is immune to by default
	UPROPERTY(EditAnywhere, Category = "Crowd Control")
	TSet<ECrowdControl> DefaultCrowdControlImmunities;
	
	FBuffRestriction CrowdControlRestriction;
	//Restriction that prevents application of buffs that will apply a certain Cc, when we are immune to that Cc.
	UFUNCTION()
	bool CheckBuffForImmunedCc(const FBuffApplyEvent& Event);

	//Checks whether we are currently immune to a given Cc type, either via default immunity or a buff.
	bool IsImmuneToCc(const ECrowdControl CcType) const;
	//Removes all buffs that are applying a given type of Cc.
	void DispelCcsOfType(const ECrowdControl CcType);

#pragma endregion 
};