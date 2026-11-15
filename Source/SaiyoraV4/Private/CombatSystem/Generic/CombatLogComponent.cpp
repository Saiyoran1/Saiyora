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
	CombatLog.Init(this);
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
	//If this combatant is already registered, just return the ID already assigned.
	if (CombatComponent->GetCombatantID().IsValid())
	{
		return CombatComponent->GetCombatantID();
	}
	const AActor* Combatant = CombatComponent->GetOwner();
	if (!IsValid(Combatant) || !Combatant->Implements<USaiyoraCombatInterface>())
	{
		return FCombatantID();
	}
	//Generate a new ID for this combatant.
	const FCombatantID NewID = FCombatantID::GenerateCombatantID();
	//Since combatant names aren't unique, we assign a number to actors of the same name.
	const FText CombatantName = CombatComponent->GetCombatName();
	//Have to use FString because FText is not a valid map key with hashing or == implemented.
	//Logging will still use the localized FText, this is purely for lookup.
	const FString NameAsString = CombatantName.ToString();
	const int NameNumber = NameUsageMap.FindRef(NameAsString);
	NameUsageMap.Add(NameAsString, NameNumber + 1);
	//This is the actual replicated array of combatants.
	Combatants.MarkItemDirty(Combatants.Items.Emplace_GetRef(NewID, CombatantName, NameNumber));
	return NewID;
}

FCombatantID UCombatLogComponent::GetCombatantID(const AActor* Combatant)
{
	if (!IsValid(Combatant) || !Combatant->Implements<USaiyoraCombatInterface>())
	{
		return FCombatantID();
	}
	const UCombatStatusComponent* CombatComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Combatant);
	if (!IsValid(CombatComp))
	{
		return FCombatantID();
	}
	return CombatComp->GetCombatantID();
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

bool UCombatLogComponent::GetCombatantDisplayName(const FCombatantID& ID, FText& OutText) const
{
	for (const FCombatant& Combatant : Combatants.Items)
	{
		if (Combatant.ID == ID)
		{
			OutText = FText::Format(NSLOCTEXT("CombatLog", "CombatantName", "{0} {1}"), Combatant.DisplayName, FText::AsNumber(Combatant.NameNumber));
			return true;
		}
	}
	return false;
}

void UCombatLogComponent::OnCombatantAdded(const FCombatant& NewCombatant)
{
	CombatLog.TryResolveEvents();
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
