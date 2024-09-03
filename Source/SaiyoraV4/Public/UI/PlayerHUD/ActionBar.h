#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "HorizontalBox.h"
#include "UserWidget.h"
#include "ActionBar.generated.h"

class ASaiyoraPlayerCharacter;
class UActionSlot;

UCLASS()
class SAIYORAV4_API UActionBar : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitActionBar(const ESaiyoraPlane Plane, const ASaiyoraPlayerCharacter* OwningPlayer);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UHorizontalBox* ActionBox;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UActionSlot> ActionSlotWidget;

	UPROPERTY()
	TArray<UActionSlot*> ActionSlots;
	ESaiyoraPlane AssignedPlane = ESaiyoraPlane::Ancient;
	UPROPERTY()
	ASaiyoraPlayerCharacter* OwnerCharacter = nullptr;
};
