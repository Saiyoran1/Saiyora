#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "PartyFrame.generated.h"

class ASaiyoraPlayerCharacter;

//A single party frame for one member of the group.
UCLASS()
class SAIYORAV4_API UPartyFrame : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitFrame(ASaiyoraPlayerCharacter* Player);
	ASaiyoraPlayerCharacter* GetAssignedPlayer() const { return PlayerCharacter; }

private:

	UPROPERTY()
	ASaiyoraPlayerCharacter* PlayerCharacter;
};
