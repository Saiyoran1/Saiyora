#include "EscapeMenu.h"
#include "MapsDataAsset.h"
#include "MultiplayerSessionsSubsystem.h"
#include "SaiyoraGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UEscapeMenu::Init()
{
	LeaveGameButton->OnClicked.AddDynamic(this, &UEscapeMenu::OnLeaveGamePressed);
	AddToViewport();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UEscapeMenu::OnLeaveGamePressed()
{
	USaiyoraGameInstance* GameInstanceRef = GetGameInstance<USaiyoraGameInstance>();
	if (!IsValid(GameInstanceRef))
	{
		return;
	}

	UMultiplayerSessionsSubsystem* MultiplayerSubsystem = GameInstanceRef->GetSubsystem<UMultiplayerSessionsSubsystem>();
	if (!IsValid(MultiplayerSubsystem) || MultiplayerSubsystem->GetSessionState() == EOnlineSessionState::NoSession)
	{
		OnSessionDestroyed(true, FString());
		return;
	}
	FOnDestroySessionCallback Callback;
	Callback.BindDynamic(this, &UEscapeMenu::OnSessionDestroyed);
	MultiplayerSubsystem->DestroySession(Callback);
}

void UEscapeMenu::OnSessionDestroyed(const bool bSuccess, const FString& Error)
{
	//Once we've successfully destroyed the session, quit the game.
	if (bSuccess)
	{
		const USaiyoraGameInstance* GameInstanceRef = GetGameInstance<USaiyoraGameInstance>();
		//If we have a valid MainMenu level reference, open that.
		if (IsValid(GameInstanceRef) && IsValid(GameInstanceRef->MapsDataAsset) && !GameInstanceRef->MapsDataAsset->MainMenuLevel.IsNull())
		{
			UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), GameInstanceRef->MapsDataAsset->MainMenuLevel, true);
			return;
		}
		//If we didn't have a valid MainMenu level set up, we will just close the game.
		UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
	}
	else
	{
		//TODO: Spawn an error widget?
	}
}