#pragma once
#include "CoreMinimal.h"
#include "CombatLogStructs.h"
#include "CombatStructs.h"
#include "Components/ActorComponent.h"
#include "CombatLogComponent.generated.h"

class UCombatStatusComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UCombatLogComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatLogComponent();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	//Called on BeginPlay by the CombatStatusComponent to register an actor as a combatant for logging.
	FCombatantID RegisterCombatant(const UCombatStatusComponent* CombatComponent);
	//Get the combatant ID for an actor for the purposes of logging.
	static FCombatantID GetCombatantID(const AActor* Combatant);
	//Get the display name for the combat log for a given combatant. Returns false if the ID is unregistered or not associated with a name yet.
	bool GetCombatantDisplayName(const FCombatantID& ID, FText& OutText) const;
	//Called client-side when a combatant ID is replicated down, to resolve any log events that had IDs with no name yet associated.
	void OnCombatantAdded(const FCombatant& NewCombatant);
	
	void LogBuffApplyEvent(const FBuffApplyEvent& Event);
	void LogBuffRemoveEvent(const FBuffRemoveEvent& Event);

	//Gets a unique ID for a combat event for logging.
	static int GetNextCombatEventID() { return CombatEventIDCounter++; }

private:

	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;
	
	UPROPERTY(Replicated)
	FCombatantArray Combatants;
	//Server-side map of how many combatants are using the same name. We just assign them a number to differentiate in the log.
	UPROPERTY()
	TMap<FString, int> NameUsageMap;

	UPROPERTY()
	FCombatLog CombatLog;
	//Combat log events queued up this frame to be sent to clients.
	FCombatLog PendingMultiplayerLogEvents;
	//Sends all pending log events to all clients for addition to the log.
	void FlushPendingCombatLog();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_UpdateCombatLog(const FCombatLog& NewLogEntries);

	static int CombatEventIDCounter;
};