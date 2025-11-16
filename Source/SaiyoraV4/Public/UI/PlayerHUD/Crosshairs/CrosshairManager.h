#pragma once
#include "CoreMinimal.h"
#include "CanvasPanel.h"
#include "CombatEnums.h"
#include "Image.h"
#include "UserWidget.h"
#include "CrosshairManager.generated.h"

class UCombatAbility;
class UFireWeapon;
class UModernCrosshair;
class ASaiyoraPlayerCharacter;

UCLASS(Abstract)
class SAIYORAV4_API UCrosshairManager : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(ASaiyoraPlayerCharacter* PlayerRef);
	
private:

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* CrosshairCanvas;
	UPROPERTY(meta = (BindWidget))
	UImage* AncientCrosshair;
	
	UPROPERTY()
	UModernCrosshair* ModernCrosshair;
	UPROPERTY()
	ASaiyoraPlayerCharacter* PlayerChar;

	UFUNCTION()
	void OnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source) { UpdateCrosshairVisibility(NewPlane); }
	void UpdateCrosshairVisibility(const ESaiyoraPlane NewPlane);
	ESaiyoraPlane CachedLastPlane = ESaiyoraPlane::Ancient;

	UFUNCTION()
	void OnAbilityAdded(UCombatAbility* NewAbility);
	UFUNCTION()
	void OnAbilityRemoved(UCombatAbility* RemovedAbility);
	UPROPERTY()
	UFireWeapon* CurrentWeaponAbility = nullptr;
};
