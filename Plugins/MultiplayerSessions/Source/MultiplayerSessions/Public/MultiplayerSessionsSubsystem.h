#pragma once
#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Engine/DataTable.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FMapInformation : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSoftObjectPtr<UWorld> Level;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FName DisplayName;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FString Description;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UTexture2D* Image = nullptr;
};

UCLASS(Blueprintable)
class UMapPool : public UDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maps")
	TArray<FMapInformation> Maps;
};

USTRUCT(BlueprintType)
struct FSaiyoraSession
{
	GENERATED_BODY()

	FSaiyoraSession();
	FSaiyoraSession(const FOnlineSessionSearchResult& SearchResult);

	FOnlineSessionSearchResult NativeResult;
	UPROPERTY(BlueprintReadOnly)
	FString HostName;
	UPROPERTY(BlueprintReadOnly)
	FString ServerName;
	UPROPERTY(BlueprintReadOnly)
	FString MapName;
	UPROPERTY(BlueprintReadOnly)
	int32 TotalSlots;
	UPROPERTY(BlueprintReadOnly)
	int32 OpenSlots;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnCreateSessionCallback, const bool, bWasSuccessful, const FString&, Error);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnFindSessionsCallback, const bool, bWasSuccessful, const FString&, Error, const TArray<FSaiyoraSession>&, Results);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnJoinSessionCallback, const bool, bWasSuccessful, const FString&, Error);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnDestroySessionCallback, const bool, bWasSuccessful, const FString&, Error);

UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UMultiplayerSessionsSubsystem();

	UFUNCTION(BlueprintCallable)
	void CreateSession(const bool bPrivate, const int32 NumPlayers, const FString& ServerName, const FString& MapName, const FOnCreateSessionCallback& Callback);
	UFUNCTION(BlueprintCallable)
	void FindSessions(const int32 MaxSearchResults, const FOnFindSessionsCallback& Callback);
	UFUNCTION(BlueprintCallable)
	void JoinSession(const FSaiyoraSession& Session, const FOnJoinSessionCallback& Callback);
	UFUNCTION(BlueprintCallable)
	void DestroySession(const FOnDestroySessionCallback& Callback);
	UFUNCTION(BlueprintCallable)
	bool StartSession();
	void UpdateSession(const FName SessionName, FOnlineSessionSettings& NewSettings) const;
	FOnlineSessionSettings GetSessionSettings(const FName SessionName) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetSessionState() const;
	
private:

	FOnlineSessionSettings LastSessionSettings;
	FOnCreateSessionCompleteDelegate CreateSessionDelegate;
	FDelegateHandle CreateSessionHandle;
	void OnCreateSession(FName SessionName, bool bWasSuccessful);
	UFUNCTION()
	void PostDestroyCreateSession(const bool bWasSuccessful, const FString& Error);
	FOnCreateSessionCallback CreateSessionCallback;
	int32 CreateSessionConnectionNumber = 1;
	FString CreateSessionServerName;
	FString CreateSessionMapName;
	bool bCreateSessionPrivate = false;
	bool bCreatingSession = false;
	bool bWaitingOnDestroyForRemake = false;

	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	FOnFindSessionsCompleteDelegate FindSessionsDelegate;
	FDelegateHandle FindSessionsHandle;
	void OnFindSessions(bool bWasSuccessful);
	FOnFindSessionsCallback FindSessionsCallback;
	bool bFindingSessions = false;
	
	FOnJoinSessionCompleteDelegate JoinSessionDelegate;
	FDelegateHandle JoinSessionHandle;
	void OnJoinSession(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	FOnJoinSessionCallback JoinSessionCallback;
	bool bJoiningSession = false;
	
	FOnDestroySessionCompleteDelegate DestroySessionDelegate;
	FDelegateHandle DestroySessionHandle;
	void OnDestroySession(FName SessionName, bool bWasSuccessful);
	FOnDestroySessionCallback DestroySessionCallback;
	bool bDestroyingSession = false;
};
