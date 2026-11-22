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

	UFUNCTION()
	void OnLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	UFUNCTION()
	void OnResurrectionListChanged(const TArray<FPendingResurrection>& PendingResurrections) { bQueuedNeedsUpdate = true; }
	
	TArray<int> AlreadyDeclinedResIDs;
	bool bAwaitingServerResDecision = false;
	bool bQueuedNeedsUpdate = false;
	void UpdatePendingResurrection();
	void SetWaitingForServer(const bool bWaiting);

	int PendingResID = -1;

	UFUNCTION()
	void AcceptPendingRes();
	UFUNCTION()
	void DeclinePendingRes();
	UFUNCTION()
	void DefaultRespawn();
};
