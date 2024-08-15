#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "PlayerHUD.generated.h"

class UDungeonDisplay;
class UBuffContainer;
class UHealthBar;
class ASaiyoraPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInfoToggleNotification, const bool, bMoreInfo);

UCLASS()
class SAIYORAV4_API UPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	
	void InitializePlayerHUD(ASaiyoraPlayerCharacter* OwnerPlayer);

	void ToggleExtraInfo(const bool bShowExtraInfo);
	bool IsExtraInfoToggled() const { return bDisplayingExtraInfo; }
	FInfoToggleNotification OnExtraInfoToggled;

private:

	bool bDisplayingExtraInfo = false;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UHealthBar* HealthBar;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UBuffContainer* BuffContainer;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UBuffContainer* DebuffContainer;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UDungeonDisplay* DungeonDisplay;

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
};
