#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "PartyFrame.generated.h"

class UPlayerHUD;
class UHealthBar;
class ASaiyoraPlayerCharacter;

//A single party frame for one member of the group.
UCLASS(Abstract)
class SAIYORAV4_API UPartyFrame : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitFrame(UPlayerHUD* OwningHUD, ASaiyoraPlayerCharacter* Player);
	ASaiyoraPlayerCharacter* GetAssignedPlayer() const { return PlayerCharacter; }

private:

	UPROPERTY(meta = (BindWidget))
	UHealthBar* HealthBar;
	
	UPROPERTY()
	ASaiyoraPlayerCharacter* PlayerCharacter;
	UPROPERTY()
	UPlayerHUD* OwnerHUD;
};
