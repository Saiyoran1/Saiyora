#pragma once
#include "CoreMinimal.h"
#include "CombatLogStructs.h"
#include "CombatStructs.h"
#include "Components/ActorComponent.h"
#include "CombatLogComponent.generated.h"

class ASaiyoraGameState;
class UCombatStatusComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UCombatLogComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatLogComponent();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	//Called on BeginPlay by the CombatStatusComponent to register an actor as a combatant for logging.
	FCombatantID RegisterCombatant(const UCombatStatusComponent* CombatComponent);
	//Get the combatant ID for an actor for the purposes of logging.
	static FCombatantID GetCombatantID(const AActor* Combatant);
	//Get the display name for the combat log for a given combatant. Returns false if the ID is unregistered or not associated with a name yet.
	bool GetCombatantDisplayName(const FCombatantID& ID, FText& OutText) const;
	//Called client-side when a combatant ID is replicated down, to set the flag for resolving any log events that had IDs with no name yet associated.
	void OnCombatantAdded(const FCombatant& NewCombatant) { bTryResolveUnlogged = true; }
	
	void LogBuffApplyEvent(const FBuffApplyEvent& Event);
	void LogBuffRemoveEvent(const FBuffRemoveEvent& Event);

private:

	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;
	
	UPROPERTY(Replicated)
	FCombatantArray Combatants;
	//Server-side map of how many combatants are using the same name. We just assign them a number to differentiate in the log.
	UPROPERTY()
	TMap<FString, int> NameUsageMap;

	//Combat log history of all log events.
	FCombatLogEventBatch CombatLogEvents;
	//Flag set when new combatants have resolved this frame.
	//Triggers an attempt to resolve log events that couldn't be logged because of missing combatant data when they were originally received.
	bool bTryResolveUnlogged = false;
	int UnresolvedEventsCounter = 0;
	
	//Combat log events queued up this frame to be sent to clients.
	FCombatLogEventBatch PendingNetworkLogEvents;
	//Sends all pending log events to all clients for addition to the log.
	void FlushPendingCombatLog();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SendCombatLogEventBatch(const FCombatLogEventBatch& NewEvents);

	//Templated function to write any combat log event to a string (which can then be written to the actual log file on disk).
	template <typename T>
	void TryWriteEventToString(const T& Event)
	{
		FString LogOutput = FString::Printf(TEXT("%.3f,%d,%s,"), Event.Timestamp, Event.EventID, Event.GetEventName());
		if (!Event.Write(this, LogOutput))
		{
			return;
		}
		LogStringQueue.Add(MoveTemp(LogOutput));
		Event.bLogged = true;
		UnresolvedEventsCounter--;
	}

	//Counter for generating unique IDs for logged combat events.
	static int CombatEventIDCounter;

	//Strings pending write to disk.
	TArray<FString> LogStringQueue;
	//Path to the CombatLog folder.
	static const FString LogsFolderSubpath;
	//Full file path to the current combat log file.
	FString CombatLogFilePath = "";
	//Launches an async task to write the log string queue to disk.
	void WriteToLogFile();
	//Flag to prevent simultaneous writes to the log.
	bool bWriteInProgress = false;
	//Min time between attempts to log to the text file.
	static constexpr float LogMinInterval = 3.0f;
	//Used to gate writes to only happen every LogMinInterval seconds.
	float LastLogTime = -1.0f;
};