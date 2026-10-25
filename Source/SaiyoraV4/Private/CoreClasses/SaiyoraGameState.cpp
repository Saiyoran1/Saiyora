#include "CoreClasses/SaiyoraGameState.h"
#include "CombatStatusComponent.h"
#include "SaiyoraPlayerCharacter.h"
#include "UnrealNetwork.h"

int ASaiyoraGameState::CombatEventIDCounter = 0;

ASaiyoraGameState::ASaiyoraGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASaiyoraGameState::BeginPlay()
{
	Super::BeginPlay();

	Combatants.Init(this);
}

void ASaiyoraGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	WorldTime += DeltaSeconds;

	FlushPendingCombatLog();
}

void ASaiyoraGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASaiyoraGameState, Combatants);
}

void ASaiyoraGameState::InitPlayer(ASaiyoraPlayerCharacter* Player)
{
	if (!IsValid(Player) || ActivePlayers.Contains(Player))
	{
		return;
	}
	ActivePlayers.Add(Player);
	OnPlayerAdded.Broadcast(Player);
}

void ASaiyoraGameState::RemovePlayer(ASaiyoraPlayerCharacter* Player)
{
	ActivePlayers.Remove(Player);
	OnPlayerRemoved.Broadcast(Player);
}

FCombatantID ASaiyoraGameState::RegisterCombatant(const UCombatStatusComponent* CombatComponent)
{
	if (!HasAuthority() || !IsValid(CombatComponent))
	{
		return FCombatantID();
	}
	AActor* Combatant = CombatComponent->GetOwner();
	if (!IsValid(Combatant) || !Combatant->Implements<USaiyoraCombatInterface>())
	{
		return FCombatantID();
	}
	//Check if this combatant is already registered.
	FCombatantID NewID = GetCombatantID(Combatant);
	//If not, create a new entry for this combatant.
	if (NewID == FCombatantID())
	{
		NewID = FCombatantID::GenerateCombatantID();
		const FName CombatantName = CombatComponent->GetCombatName();
		//Since names aren't unique, we assign a number to actors of the same name.
		const int NameNumber = NameUsageMap.FindRef(CombatantName);
		NameUsageMap.Add(CombatantName, NameNumber + 1);
		//This is the actual replicated array of combatants.
		Combatants.MarkItemDirty(Combatants.Items.Emplace_GetRef(NewID, CombatantName, NameNumber));
		//These maps are just used for lookups in each direction (ID -> actor, actor -> ID).
		IDToCombatantMap.Add(NewID, Combatant);
		CombatantToIDMap.Add(Combatant, NewID);
	}
	return NewID;
}

void ASaiyoraGameState::FlushPendingCombatLog()
{
	if (PendingMultiplayerLogEvents.HasData())
	{
		Multicast_UpdateCombatLog(PendingMultiplayerLogEvents);
		PendingMultiplayerLogEvents.Clear();
	}
}

void ASaiyoraGameState::Multicast_UpdateCombatLog_Implementation(const FCombatLog& NewLogEntries)
{
	CombatLog.Append(NewLogEntries);
}

void ASaiyoraGameState::LogBuffApplyEvent(const FBuffApplyEvent& Event)
{
	//Queue this event up to be sent to clients at the next log update.
	PendingMultiplayerLogEvents.BuffApplications.Emplace(Event, this);
}

void ASaiyoraGameState::LogBuffRemoveEvent(const FBuffRemoveEvent& Event)
{
	//Queue this event up to be sent to clients at the next log update.
	PendingMultiplayerLogEvents.BuffRemovals.Emplace(Event, this);
}