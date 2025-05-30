#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "DamageEnums.h"
#include "UserWidget.h"
#include "HealthBar.generated.h"

class UModernSpecialization;
class UAncientSpecialization;
class UCombatStatusComponent;
class UPlayerHUD;
class USaiyoraUIDataAsset;
class UTextBlock;
class UProgressBar;
class UDamageHandler;
class ASaiyoraPlayerCharacter;

UCLASS()
class SAIYORAV4_API UHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeDestruct() override;
	void InitHealthBar(UPlayerHUD* OwningHUD, ASaiyoraPlayerCharacter* OwningPlayer);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* HealthBar;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* AbsorbBar;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* LeftText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* MiddleText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* RightText;

	UPROPERTY()
	UPlayerHUD* OwnerHUD;
	UPROPERTY()
	UDamageHandler* OwnerDamageHandler = nullptr;
	UPROPERTY()
	UCombatStatusComponent* OwnerCombatStatusComp = nullptr;
	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayerChar = nullptr;
	UPROPERTY()
	USaiyoraUIDataAsset* UIDataAsset = nullptr;

	UFUNCTION()
	void UpdateHealth(AActor* Actor = nullptr, const float PreviousHealth = 0.0f, const float NewHealth = 0.0f);
	UFUNCTION()
	void UpdateLifeStatus(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);

	bool bShowingExtraInfo = false;
	UFUNCTION()
	void ToggleExtraInfo(const bool bShowExtraInfo);

	UPROPERTY(EditAnywhere, Category = "Health")
	bool bUseClassColor = false;
	void UpdateHealthBarColor();
	UFUNCTION()
	void UpdateClassColorOnAncientSpecChanged(UAncientSpecialization* OldSpec, UAncientSpecialization* NewSpec) { UpdateHealthBarColor(); }
	UFUNCTION()
	void UpdateClassColorOnModernSpecChanged(UModernSpecialization* OldSpec, UModernSpecialization* NewSpec) { UpdateHealthBarColor(); }
	UFUNCTION()
	void UpdateClassColorOnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source) { UpdateHealthBarColor(); }
};
