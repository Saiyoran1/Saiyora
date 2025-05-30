#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "VerticalBox.h"
#include "PartyFrames.generated.h"

class UPlayerHUD;
class UPartyFrame;
class ASaiyoraPlayerCharacter;

//The overall party frame widget that contains the individual party frames for each group member.
UCLASS(Abstract)
class SAIYORAV4_API UPartyFrames : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(UPlayerHUD* OwningHUD);

private:

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* PartyFramesVBox;

	UPROPERTY(EditDefaultsOnly, Category = "Party Frames")
	TSubclassOf<UPartyFrame> PartyFrameClass;

	UFUNCTION()
	void OnPlayerJoined(ASaiyoraPlayerCharacter* PlayerCharacter);
	UFUNCTION()
	void OnPlayerLeft(ASaiyoraPlayerCharacter* PlayerCharacter);

	UPROPERTY()
	TArray<UPartyFrame*> ActiveFrames;
	UPROPERTY()
	UPlayerHUD* OwnerHUD;
};
