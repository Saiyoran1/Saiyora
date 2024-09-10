#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "HorizontalBox.h"
#include "UserWidget.h"
#include "ActionBar.generated.h"

class UBuff;
class UCombatAbility;
class UBorder;
class ASaiyoraPlayerCharacter;
class UActionSlot;

UCLASS()
class SAIYORAV4_API UActionBar : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitActionBar(const ESaiyoraPlane Plane, const ASaiyoraPlayerCharacter* OwningPlayer);
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void AddAbilityProc(UBuff* SourceBuff, const TSubclassOf<UCombatAbility> AbilityClass);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UHorizontalBox* ActionBox;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UBorder* ActionBorder;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UActionSlot> ActionSlotWidget;

	UPROPERTY()
	TArray<UActionSlot*> ActionSlots;
	ESaiyoraPlane AssignedPlane = ESaiyoraPlane::Ancient;
	UPROPERTY()
	ASaiyoraPlayerCharacter* OwnerCharacter = nullptr;

	UFUNCTION()
	void OnPlaneSwapped(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source);
	bool bInCorrectPlane = false;
	float PlaneSwapAlpha = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	float PlaneSwapAnimDuration = 0.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	UCurveFloat* PlaneSwapAnimCurve = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	float MinScale = 0.7f;
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	float MaxScale = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	FLinearColor DesaturatedTint = FLinearColor::Gray;
};
