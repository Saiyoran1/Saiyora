#pragma once
#include "CoreMinimal.h"
#include "CombatLogStructs.h"
#include "GameFramework/GameState.h"
#include "SaiyoraGameState.generated.h"

struct FBuffRemoveEvent;
struct FBuffApplyEvent;
class UCombatStatusComponent;
class ASaiyoraPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerAddRemoveNotification, ASaiyoraPlayerCharacter*, Player);

UCLASS()
class SAIYORAV4_API ASaiyoraGameState : public AGameState
{
	GENERATED_BODY()

public:

	ASaiyoraGameState();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma region Time

public:

	virtual double GetServerWorldTimeSeconds() const override { return WorldTime; }
	void UpdateClientWorldTime(const float ServerTime) { WorldTime = ServerTime; }

private:

	float WorldTime = 0.0f;

#pragma endregion 
#pragma region Player Handling

public:
	
	UFUNCTION(BlueprintPure, Category = "Players")
	void GetActivePlayers(TArray<ASaiyoraPlayerCharacter*>& OutActivePlayers) const { OutActivePlayers = ActivePlayers; }
	
	void InitPlayer(ASaiyoraPlayerCharacter* Player);
	void RemovePlayer(ASaiyoraPlayerCharacter* Player);

	UPROPERTY(BlueprintAssignable)
	FPlayerAddRemoveNotification OnPlayerAdded;
	UPROPERTY(BlueprintAssignable)
	FPlayerAddRemoveNotification OnPlayerRemoved;

private:

	UPROPERTY()
	TArray<ASaiyoraPlayerCharacter*> ActivePlayers;

#pragma endregion
#pragma region Combatants

public:

	FCombatantID RegisterCombatant(const UCombatStatusComponent* CombatComponent);
	FCombatantID GetCombatantID(const AActor* Combatant) const { return CombatantToIDMap.FindRef(Combatant); }
	AActor* GetCombatantByID(const FCombatantID& ID) const { return IDToCombatantMap.FindRef(ID); }
	void OnCombatantAdded(const FCombatant& NewCombatant) {}

	void LogBuffApplyEvent(const FBuffApplyEvent& Event);
	void LogBuffRemoveEvent(const FBuffRemoveEvent& Event);

	//Gets a unique ID for a combat event for logging.
	static int GetNextCombatEventID() { return CombatEventIDCounter++; }

private:

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

#pragma endregion 
};

