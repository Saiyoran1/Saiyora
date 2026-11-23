#pragma once
#include "CoreMinimal.h"
#include "Border.h"
#include "Button.h"
#include "DamageStructs.h"
#include "Image.h"
#include "TextBlock.h"
#include "UserWidget.h"
#include "DeathOverlay.generated.h"

class UDamageHandler;
struct FPendingResurrection;
class ASaiyoraPlayerCharacter;

UCLASS(Abstract)
class SAIYORAV4_API UDeathOverlay : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(ASaiyoraPlayerCharacter* OwningChar);
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//Called by the server when failing to respawn/resurrect.
	//Re-enables the respawn, accept, and decline buttons.
	void OnServerFailedToRespawn() { SetWaitingForServer(false); }

private:
	
	UPROPERTY(meta = (BindWidget))
	UButton* DefaultRespawnButton;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefaultRespawnText;

	UPROPERTY(EditInstanceOnly, Category = "Death")
	FText RespawnText;
	UPROPERTY(EditInstanceOnly, Category = "Death")
	FText RespawnUnavailableText;
	
	UPROPERTY(meta = (BindWidget))
	UBorder* RezBorder;
	UPROPERTY(meta = (BindWidget))
	UButton* AcceptPendingResButton;
	UPROPERTY(meta = (BindWidget))
	UButton* DeclinePendingResButton;
	UPROPERTY(meta = (BindWidget))
	UImage* PendingResIcon;
	
	UPROPERTY()
	UDamageHandler* OwnerDamageHandler;

	//Class of the actor used to display the current pending resurrection location.
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<AActor> ResurrectionLocationIndicatorClass;
	//The actor that shows a particle effect at the location of the current pending resurrection, if one is being offered.
	UPROPERTY()
	AActor* ResurrectionLocationIndicator;

	UFUNCTION()
	void OnLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	UFUNCTION()
	void OnResurrectionListChanged(const TArray<FPendingResurrection>& PendingResurrections) { bQueuedNeedsUpdate = true; }

	//Flag to trigger UpdatePendingResurrection on the next tick.
	bool bQueuedNeedsUpdate = false;
	//Updates the death overlay to show or hide the next pending resurrection.
	void UpdatePendingResurrection();
	//List of pending resurrections we've declined. This hides them without waiting for the server to clear the buff and replicate it back down.
	TArray<int> AlreadyDeclinedResIDs;
	//Called when a respawn or resurrection is accepted to lock the accept/decline/respawn buttons until the server can either replicate state or RPC failure.
	void SetWaitingForServer(const bool bWaiting);
	//Flag set by SetWaitingForServer that prevents updating the current pending resurrection while we're waiting on the server to accept or deny a requested res/respawn.
	bool bAwaitingServerResDecision = false;
	//ID of the current pending resurrection the player can accept or decline. -1 means no pending res is being displayed.
	int PendingResID = -1;

	UFUNCTION()
	void AcceptPendingRes();
	UFUNCTION()
	void DeclinePendingRes();
	UFUNCTION()
	void DefaultRespawn();
};
