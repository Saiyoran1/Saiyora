#pragma once
#include "CoreMinimal.h"
#include "Button.h"
#include "DamageStructs.h"
#include "Image.h"
#include "Overlay.h"
#include "TextBlock.h"
#include "UserWidget.h"
#include "WidgetSwitcher.h"
#include "DeathOverlay.generated.h"

class UDamageHandler;
struct FPendingResurrection;
class ASaiyoraPlayerCharacter;

UCLASS()
class SAIYORAV4_API UDeathOverlay : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(ASaiyoraPlayerCharacter* OwningChar);

private:
	
	UPROPERTY(meta = (BindWidget))
	UOverlay* DefaultRespawnOverlay;
	UPROPERTY(meta = (BindWidget))
	UButton* DefaultRespawnButton;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefaultRespawnText;

	UPROPERTY(EditInstanceOnly, Category = "Death")
	FText RespawnText;
	UPROPERTY(EditInstanceOnly, Category = "Death")
	FText RespawnUnavailableText;
	
	UPROPERTY(meta = (BindWidget))
	UOverlay* PendingResOverlay;
	UPROPERTY(meta = (BindWidget))
	UButton* AcceptPendingResButton;
	UPROPERTY(meta = (BindWidget))
	UButton* DeclinePendingResButton;
	UPROPERTY(meta = (BindWidget))
	UImage* PendingResIcon;

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* RespawnResurrectSwitcher;
	
	UPROPERTY()
	UDamageHandler* OwnerDamageHandler;

	UFUNCTION()
	void OnLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	UFUNCTION()
	void OnPendingResUpdated(const FPendingResurrection& PendingRes);

	bool bWasDisplayed = false;
	bool bDoingInit = false;

	int PendingResID = -1;

	UFUNCTION()
	void AcceptPendingRes();
	UFUNCTION()
	void DeclinePendingRes();
	UFUNCTION()
	void DefaultRespawn();
};
