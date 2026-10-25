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
	//Get a combatant ID for an actor for the purposes of logging.
	FCombatantID GetCombatantID(const AActor* Combatant) const { return CombatantToIDMap.FindRef(Combatant); }
	//Gets a pointer to an actor using its combat log ID. Returns nullptr for unregistered combatants.
	AActor* GetCombatantByID(const FCombatantID& ID) const { return IDToCombatantMap.FindRef(ID); }
	void OnCombatantAdded(const FCombatant& NewCombatant) {}
	
	void LogBuffApplyEvent(const FBuffApplyEvent& Event);
	void LogBuffRemoveEvent(const FBuffRemoveEvent& Event);

	//Gets a unique ID for a combat event for logging.
	static int GetNextCombatEventID() { return CombatEventIDCounter++; }

private:

	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;
	
	UPROPERTY(Replicated)
	FCombatantArray Combatants;
	UPROPERTY()
	TMap<FCombatantID, AActor*> IDToCombatantMap;
	UPROPERTY()
	TMap<AActor*, FCombatantID> CombatantToIDMap;
	UPROPERTY()
	TMap<FName, int> NameUsageMap;

	UPROPERTY()
	FCombatLog CombatLog;
	FCombatLog PendingMultiplayerLogEvents;
	void FlushPendingCombatLog();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_UpdateCombatLog(const FCombatLog& NewLogEntries);

	static int CombatEventIDCounter;
};