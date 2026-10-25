#include "CombatLogComponent.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraGameState.h"
#include "UnrealNetwork.h"

int UCombatLogComponent::CombatEventIDCounter = 0;

UCombatLogComponent::UCombatLogComponent()
{
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UCombatLogComponent::InitializeComponent()
{
	Super::InitializeComponent();

	GameStateRef = Cast<ASaiyoraGameState>(GetOwner());
	checkf(IsValid(GameStateRef), TEXT("CombatLogComponent attached to actor that isn't ASaiyoraGameState! This will crash! Owner: %s"), *GetOwner()->GetName());
}

void UCombatLogComponent::BeginPlay()
{
	Super::BeginPlay();

	Combatants.Init(this);
}

void UCombatLogComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FlushPendingCombatLog();
}

void UCombatLogComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatLogComponent, Combatants);
}

FCombatantID UCombatLogComponent::RegisterCombatant(const UCombatStatusComponent* CombatComponent)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(CombatComponent))
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

void UCombatLogComponent::FlushPendingCombatLog()
{
	//Check queued combat log events from this frame and send them to all players.
	if (PendingMultiplayerLogEvents.HasData())
	{
		Multicast_UpdateCombatLog(PendingMultiplayerLogEvents);
		PendingMultiplayerLogEvents.Clear();
	}
}

void UCombatLogComponent::Multicast_UpdateCombatLog_Implementation(const FCombatLog& NewLogEntries)
{
	//Add all new log entries from the last server tick to the combat log.
	CombatLog.Append(NewLogEntries);
}

void UCombatLogComponent::LogBuffApplyEvent(const FBuffApplyEvent& Event)
{
	//Queue this event up to be sent to clients at the next log update.
	PendingMultiplayerLogEvents.AddBuffApplyEvent(Event, GameStateRef);
}

void UCombatLogComponent::LogBuffRemoveEvent(const FBuffRemoveEvent& Event)
{
	//Queue this event up to be sent to clients at the next log update.
	PendingMultiplayerLogEvents.AddBuffRemoveEvent(Event, GameStateRef);
}
