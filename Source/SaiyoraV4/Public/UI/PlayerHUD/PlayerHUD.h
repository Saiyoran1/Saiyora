#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "PlayerHUD.generated.h"

class UCrosshair;
class UPartyFrames;
class UCombatAbility;
class UBuff;
class UActionBar;
class UResourceContainer;
class UCastBar;
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

	void AddAbilityProc(UBuff* SourceBuff, const TSubclassOf<UCombatAbility> AbilityClass);

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
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UCastBar* CastBar;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UResourceContainer* ResourceContainer;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UActionBar* AncientBar;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UActionBar* ModernBar;
	UPROPERTY(meta = (BindWidget))
	UPartyFrames* PartyFrames;
	UPROPERTY(meta = (BindWidget))
	UCrosshair* Crosshair;

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
};
