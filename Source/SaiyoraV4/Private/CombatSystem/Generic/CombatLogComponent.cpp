#include "CombatLogComponent.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraGameState.h"
#include "UnrealNetwork.h"

int UCombatLogComponent::CombatEventIDCounter = 0;
const FString UCombatLogComponent::LogsFolderSubpath = "CombatLogs/";

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

void UCombatLogComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	//At EndPlay, bypass the write interval timer to try and flush anything left in the queue.
	//If we're already in a write, we will lose what's still queued, but that's probably okay.
	if (!bWriteInProgress && LogStringQueue.Num() > 0)
	{
		WriteToLogFile();
	}
}

void UCombatLogComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatLogComponent, Combatants);
}

void UCombatLogComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Send any pending combat log events from this frame to all clients.
	if (GetOwnerRole() == ROLE_Authority)
	{
		FlushPendingCombatLog();
	}
	//If new combatants were resolved this frame, try to resolve any log events that were waiting on combatant info.
	if (bTryResolveUnlogged)
	{
		bTryResolveUnlogged = false;
		//If we already know we don't have any unresolved events left, no need to iterate the whole log.
		if (UnresolvedEventsCounter > 0)
		{
			CombatLogEvents.ForEachCombatEvent([this](const auto& Event)
			{
				if (!Event.bLogged)
				{
					TryWriteEventToString(Event);
				}
			});
		}
	}
	//If we're not waiting on a previous write, and it's been some length of time and there's new events to write,
	//write to the actual combat log file on a background thread.
	if (!bWriteInProgress && LogStringQueue.Num() > 0
		&& GetWorld()->GetTimeSeconds() - LastLogTime > LogMinInterval)
	{
		WriteToLogFile();
	}
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

bool UCombatLogComponent::GetCombatantDisplayName(const FCombatantID& ID, FText& OutText) const
{
	for (const FCombatant& Combatant : Combatants.Items)
	{
		if (Combatant.ID == ID)
		{
			if (Combatant.NameNumber > 0)
			{
				OutText = FText::Format(NSLOCTEXT("CombatLog", "CombatantName", "{0} {1}"), Combatant.DisplayName, FText::AsNumber(Combatant.NameNumber));
			}
			else
			{
				OutText = Combatant.DisplayName;
			}
			return true;
		}
	}
	return false;
}

void UCombatLogComponent::FlushPendingCombatLog()
{
	//Check queued combat log events from this frame and send them to all players.
	if (PendingNetworkLogEvents.HasData())
	{
		Multicast_SendCombatLogEventBatch(PendingNetworkLogEvents);
		PendingNetworkLogEvents.Clear();
	}
}

void UCombatLogComponent::Multicast_SendCombatLogEventBatch_Implementation(const FCombatLogEventBatch& NewEvents)
{
	NewEvents.ForEachCombatEvent([this](const auto& Event)
	{
		const auto& AddedEvent = CombatLogEvents.AddEvent(Event);
		UnresolvedEventsCounter++;
		TryWriteEventToString(AddedEvent);
	});
}

void UCombatLogComponent::WriteToLogFile()
{
	bWriteInProgress = true;
	LastLogTime = GetWorld()->GetTimeSeconds();
	if (CombatLogFilePath.IsEmpty())
	{
		//Create the CombatLogs folder within the project log directory, if needed.
		const FString LogFolderPath = FPaths::ProjectLogDir() + LogsFolderSubpath;
		IFileManager::Get().MakeDirectory(*LogFolderPath);
		//File name is "CombatLog_DateTime_PIEWindowNumber.txt" The _PIEWindowNumber is compiled out outside the editor.
		CombatLogFilePath = LogFolderPath + "CombatLog";
		CombatLogFilePath += FString::Printf(TEXT("_%s"), *FDateTime::Now().ToFormattedString(TEXT("%y%m%d%H%M%S")));
#if WITH_EDITOR
		if (GetWorld()->IsPlayInEditor())
		{
			CombatLogFilePath += FString::Printf(TEXT("_%i"), int32(GPlayInEditorID));
		}
#endif
		CombatLogFilePath += ".txt";
	}
	const TWeakObjectPtr<UCombatLogComponent> LogCompPtr = MakeWeakObjectPtr(this);
	AsyncTask(ENamedThreads::Type::AnyBackgroundThreadNormalTask, [Path = CombatLogFilePath, Queue = MoveTemp(LogStringQueue), LogCompPtr]()
	{
		const bool bSuccess = FFileHelper::SaveStringArrayToFile(Queue, *Path,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
		//Callback to the game thread to clear the bWriteInProgress flag and allow subsequent writes.
		AsyncTask(ENamedThreads::Type::GameThread, [bSuccess, LogCompPtr]()
		{
			if (UCombatLogComponent* LogComp = LogCompPtr.Get())
			{
				LogComp->bWriteInProgress = false;
			}
		});
	});
}

void UCombatLogComponent::LogBuffApplyEvent(const FBuffApplyEvent& Event)
{
	//Queue this event up to be sent to clients at the next log update.
	PendingNetworkLogEvents.AddEvent(FLoggedBuffApplyEvent(Event,
		CombatEventIDCounter++, GameStateRef->GetServerWorldTimeSeconds()));
}

void UCombatLogComponent::LogBuffRemoveEvent(const FBuffRemoveEvent& Event)
{
	//Queue this event up to be sent to clients at the next log update.
	PendingNetworkLogEvents.AddEvent(FLoggedBuffRemoveEvent(Event,
		CombatEventIDCounter++, GameStateRef->GetServerWorldTimeSeconds()));
}
