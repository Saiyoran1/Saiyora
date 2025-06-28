#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "HorizontalBox.h"
#include "Image.h"
#include "TextBlock.h"
#include "UserWidget.h"
#include "PartyFrame.generated.h"

class UCombatStatusComponent;
class UBuffIcon;
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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	UPROPERTY(meta = (BindWidget))
	UHealthBar* HealthBar;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NameText;
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* BuffBox;
	UPROPERTY(meta = (BindWidget))
	UImage* XPlaneOverlayImage;

	UPROPERTY(EditAnywhere, Category = "Party Frame")
	int PlayerNameCharLimit = 12;
	UPROPERTY(EditAnywhere, Category = "Party Frame")
	TSubclassOf<UBuffIcon> BuffIconClass;

	UPROPERTY(EditDefaultsOnly, Category = "Party Frame")
	UMaterialInstance* XPlaneOverlayMaterial;
	UPROPERTY(EditDefaultsOnly, Category = "Party Frame")
	float XPlaneAnimationLength = 0.5f;
	
	UPROPERTY()
	ASaiyoraPlayerCharacter* PlayerCharacter;
	UPROPERTY()
	UPlayerHUD* OwnerHUD;

	UFUNCTION()
	void OnBuffApplied(const FBuffApplyEvent& ApplyEvent) { OnBuffApplied(ApplyEvent.AffectedBuff); }
	void OnBuffApplied(UBuff* Buff);
	UFUNCTION()
	void OnBuffRemoved(const FBuffRemoveEvent& RemoveEvent);
	UPROPERTY()
	TMap<UBuff*, UBuffIcon*> BuffIcons;

	float XPlaneAlpha = 0.0f;
	bool bIsXPlaneFromLocalPlayer = false;
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicXPlaneOverlayMat;
	UPROPERTY()
	UCombatStatusComponent* AssignedPlayerCombatComp;
	UPROPERTY()
	UCombatStatusComponent* LocalPlayerCombatComp;
	UFUNCTION()
	void OnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source);
};
