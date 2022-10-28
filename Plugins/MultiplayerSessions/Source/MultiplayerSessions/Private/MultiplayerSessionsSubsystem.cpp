#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/PlayerState.h"

FSaiyoraSession::FSaiyoraSession()
{
	HostName = "";
	TotalSlots = 0;
	OpenSlots = 0;
	ServerName = "";
	MapName = "";
}

FSaiyoraSession::FSaiyoraSession(const FOnlineSessionSearchResult& SearchResult)
{
	NativeResult = SearchResult;
	HostName = SearchResult.Session.OwningUserName;
	TotalSlots = SearchResult.Session.SessionSettings.NumPublicConnections;
	OpenSlots = SearchResult.Session.NumOpenPublicConnections;
	SearchResult.Session.SessionSettings.Get(FName(TEXT("ServerName")), ServerName);
	SearchResult.Session.SessionSettings.Get(SETTING_MAPNAME, MapName);
}

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSession)),
	FindSessionsDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessions)),
	JoinSessionDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSession)),
	DestroySessionDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySession))
{
	
}

void UMultiplayerSessionsSubsystem::CreateSession(const bool bPrivate, const int32 NumPlayers, const FString& ServerName, const FString& MapName, const FOnCreateSessionCallback& Callback)
{
	if (!Callback.IsBound())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid create session callback."));
		}
		return;
	}
	if (bCreatingSession)
	{
		Callback.Execute(false, TEXT("Currently creating a session, cannot create session again."));
		return;
	}
	if (bJoiningSession)
	{
		Callback.Execute(false, TEXT("Currently joining a session, cannot create session."));
		return;
	}
	if (bFindingSessions)
	{
		Callback.Execute(false, TEXT("Currently finding sessions, cannot create session."));
		return;
	}
	if (bDestroyingSession)
	{
		Callback.Execute(false, TEXT("Currently destroying session, cannot create session"));
		return;
	}
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		Callback.Execute(false, TEXT("Invalid online subsystem, cannot create session."));
		return;
	}
	const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		Callback.Execute(false, TEXT("Invalid session interface, cannot create session."));
		return;
	}
	bCreatingSession = true;
	CreateSessionHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegate);
	CreateSessionCallback = Callback;
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bWaitingOnDestroyForRemake = true;
		CreateSessionConnectionNumber = NumPlayers;
		CreateSessionServerName = ServerName;
		CreateSessionMapName = MapName;
		bCreateSessionPrivate = bPrivate;
		FOnDestroySessionCallback PostDestroy;
		PostDestroy.BindDynamic(this, &ThisClass::PostDestroyCreateSession);
		DestroySession(PostDestroy);
	}
	else
	{
		LastSessionSettings = FOnlineSessionSettings();
		LastSessionSettings.bIsLANMatch = Subsystem->GetSubsystemName() == FName(TEXT("NULL"));
		LastSessionSettings.NumPublicConnections = bPrivate ? 0 : NumPlayers;
		LastSessionSettings.NumPrivateConnections = bPrivate ? NumPlayers : 0;
		LastSessionSettings.bAllowJoinInProgress = true;
		LastSessionSettings.bAllowJoinViaPresence = true;
		LastSessionSettings.bShouldAdvertise = !bPrivate;
		LastSessionSettings.bUsesPresence = true;
		LastSessionSettings.bUseLobbiesIfAvailable = true;
		LastSessionSettings.Set(SETTING_GAMEMODE, FString(TEXT("Saiyora")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		LastSessionSettings.Set(FName(TEXT("ServerName")), ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		LastSessionSettings.Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		const FUniqueNetIdPtr NetID = GetWorld()->GetFirstPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();
		SessionInterface->CreateSession(*NetID, NAME_GameSession, LastSessionSettings);
	}
}

void UMultiplayerSessionsSubsystem::OnCreateSession(FName SessionName, bool bWasSuccessful)
{
	bCreatingSession = false;
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
		}
	}
	if (CreateSessionCallback.IsBound())
	{
		CreateSessionCallback.Execute(bWasSuccessful, bWasSuccessful ? TEXT("Successfully created a session.") : TEXT("Session interface returned false when creating session."));
		CreateSessionCallback.Clear();
	}
}

void UMultiplayerSessionsSubsystem::FindSessions(const int32 MaxSearchResults, const FOnFindSessionsCallback& Callback)
{
	if (!Callback.IsBound())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid find sessions callback."));
		}
		return;
	}
	if (MaxSearchResults <= 0)
	{
		Callback.Execute(false, TEXT("Max search results at or below 0, cannot find sessions."), TArray<FSaiyoraSession>());
		return;
	}
	if (bCreatingSession)
	{
		Callback.Execute(false, TEXT("Currently creating session, cannot find sessions."), TArray<FSaiyoraSession>());
		return;
	}
	if (bJoiningSession)
	{
		Callback.Execute(false, TEXT("Currently joining session, cannot find sessions."), TArray<FSaiyoraSession>());
		return;
	}
	if (bFindingSessions)
	{
		Callback.Execute(false, TEXT("Currently finding sessions, cannot find sessions again."), TArray<FSaiyoraSession>());
		return;
	}
	if (bDestroyingSession)
	{
		Callback.Execute(false, TEXT("Currently destroying session, cannot find sessions."), TArray<FSaiyoraSession>());
		return;
	}
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		Callback.Execute(false, TEXT("Invalid online subsystem, cannot find sessions."), TArray<FSaiyoraSession>());
		return;
	}
	const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		Callback.Execute(false, TEXT("Invalid session interface, cannot find sessions."), TArray<FSaiyoraSession>());
		return;
	}
	bFindingSessions = true;
	FindSessionsCallback = Callback;
	FindSessionsHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegate);
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == FName(TEXT("NULL"));
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	const FUniqueNetIdPtr NetID = GetWorld()->GetFirstPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();
	SessionInterface->FindSessions(*NetID, LastSessionSearch.ToSharedRef());
}

void UMultiplayerSessionsSubsystem::OnFindSessions(bool bWasSuccessful)
{
	bFindingSessions = false;
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
		}
	}
	if (FindSessionsCallback.IsBound())
	{
		TArray<FSaiyoraSession> SessionResults;
		if (bWasSuccessful)
		{
			for (const FOnlineSessionSearchResult& Result : LastSessionSearch->SearchResults)
			{
				if (Result.IsValid())
				{
					SessionResults.Add(FSaiyoraSession(Result));
				}
			}
		}
		FindSessionsCallback.Execute(bWasSuccessful, bWasSuccessful ? TEXT("Successfully found sessions.") : TEXT("Session interface returned false when finding sessions."), SessionResults);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FSaiyoraSession& Session, const FOnJoinSessionCallback& Callback)
{
	if (!Callback.IsBound())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid join session callback."));
		}
		return;
	}
	if (bCreatingSession)
	{
		Callback.Execute(false, TEXT("Currently creating session, cannot join session."));
		return;
	}
	if (bJoiningSession)
	{
		Callback.Execute(false, TEXT("Currently joining session, cannot join session again."));
		return;
	}
	if (bFindingSessions)
	{
		Callback.Execute(false, TEXT("Currently finding sessions, cannot join session."));
		return;
	}
	if (bDestroyingSession)
	{
		Callback.Execute(false, TEXT("Currently destroying session, cannot join session."));
		return;
	}
	if (!Session.NativeResult.IsValid())
	{
		Callback.Execute(false, TEXT("Invalid session result, cannot join session."));
		return;
	}
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		Callback.Execute(false, TEXT("Invalid online subsystem, cannot join session."));
		return;
	}
	const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		Callback.Execute(false, TEXT("Invalid session interface, cannot join session."));
		return;
	}
	bJoiningSession = true;
	JoinSessionCallback = Callback;
	JoinSessionHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegate);
	const FUniqueNetIdPtr NetID = GetWorld()->GetFirstPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();
	SessionInterface->JoinSession(*NetID, NAME_GameSession, Session.NativeResult);
}

void UMultiplayerSessionsSubsystem::OnJoinSession(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bJoiningSession = false;
	IOnlineSessionPtr SessionInterface = nullptr;
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
	}
	const bool bSuccess = Result == EOnJoinSessionCompleteResult::Success;
	if (JoinSessionCallback.IsBound())
	{
		JoinSessionCallback.Execute(bSuccess, bSuccess ? TEXT("Successfully joined session.") : TEXT("Session interface returned false when joining session."));
	}
	if (bSuccess && SessionInterface.IsValid())
	{
		FString ConnectString;
		SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString);
		GetWorld()->GetFirstPlayerController()->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	}
}

void UMultiplayerSessionsSubsystem::DestroySession(const FOnDestroySessionCallback& Callback)
{
	if (!Callback.IsBound())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,TEXT("Invalid callback for destroy session."));
		}
		return;
	}
	if (bCreatingSession && !bWaitingOnDestroyForRemake)
	{
		Callback.Execute(false, TEXT("Currently creating session (not waiting on destroy), cannot destroy session."));
		return;
	}
	if (bJoiningSession)
	{
		Callback.Execute(false, TEXT("Currently joining session, cannot destroy session."));
		return;
	}
	if (bFindingSessions)
	{
		Callback.Execute(false, TEXT("Currently finding sessions, cannot destroy session."));
		return;
	}
	if (bDestroyingSession)
	{
		Callback.Execute(false, TEXT("Currently destroying session, cannot destroy session again."));
		return;
	}
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		Callback.Execute(false, TEXT("Invalid online subsystem, cannot destroy session."));
		return;
	}
	const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		Callback.Execute(false, TEXT("Invalid session interface, cannot destroy session."));
		return;
	}
	bDestroyingSession = true;
	DestroySessionCallback = Callback;
	DestroySessionHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegate);
	SessionInterface->DestroySession(NAME_GameSession);
}

void UMultiplayerSessionsSubsystem::OnDestroySession(FName SessionName, bool bWasSuccessful)
{
	bDestroyingSession = false;
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
		}
	}
	if (DestroySessionCallback.IsBound())
	{
		DestroySessionCallback.Execute(bWasSuccessful, bWasSuccessful ? TEXT("Successfully destroyed session.") : TEXT("Session interface returned false from destroying session."));
	}
}

void UMultiplayerSessionsSubsystem::PostDestroyCreateSession(const bool bWasSuccessful, const FString& Error)
{
	if (bWaitingOnDestroyForRemake)
	{
		bWaitingOnDestroyForRemake = false;
		if (bWasSuccessful)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Destroyed current session to create new session."));
			}
			CreateSession(bCreateSessionPrivate, CreateSessionConnectionNumber, CreateSessionServerName, CreateSessionMapName, CreateSessionCallback);
		}
		else
		{
			bCreatingSession = false;
			if (CreateSessionCallback.IsBound())
			{
				CreateSessionCallback.Execute(false, TEXT("Session interface returned false from destroying existing session, cannot create session."));
			}
		}
	}
}

bool UMultiplayerSessionsSubsystem::StartSession()
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return false;
	}
	const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return false;
	}
	return SessionInterface->StartSession(NAME_GameSession);
}

void UMultiplayerSessionsSubsystem::UpdateSession(const FName SessionName, FOnlineSessionSettings& NewSettings) const
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return;
	}
	const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return;
	}
	SessionInterface->UpdateSession(SessionName, NewSettings);
}

FOnlineSessionSettings UMultiplayerSessionsSubsystem::GetSessionSettings(const FName SessionName) const
{
	if (SessionName == NAME_None)
	{
		return FOnlineSessionSettings();
	}
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return FOnlineSessionSettings();
	}
	const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return FOnlineSessionSettings();
	}
	const FNamedOnlineSession* Session = SessionInterface->GetNamedSession(SessionName);
	if (!Session)
	{
		return FOnlineSessionSettings();
	}
	return Session->SessionSettings;
}

FString UMultiplayerSessionsSubsystem::GetSessionState() const
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return FString(TEXT("Invalid subsystem."));
	}
	const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return FString(TEXT("Invalid session interface"));
	}
	const FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	if (!Session)
	{
		return FString(TEXT("No valid session."));
	}
	switch (Session->SessionState)
	{
	case EOnlineSessionState::NoSession :
		return FString(TEXT("NoSession"));
	case EOnlineSessionState::Creating :
		return FString(TEXT("Creating"));
	case EOnlineSessionState::Pending :
		return FString(TEXT("Pending"));
	case EOnlineSessionState::Starting :
		return FString(TEXT("Starting"));
	case EOnlineSessionState::InProgress :
		return FString(TEXT("InProgress"));
	case EOnlineSessionState::Ending :
		return FString(TEXT("Ending"));
	case EOnlineSessionState::Ended :
		return FString(TEXT("Ended"));
	case EOnlineSessionState::Destroying :
		return FString(TEXT("Destroying"));
	default:
		return FString(TEXT("defaulted"));
	}
}
