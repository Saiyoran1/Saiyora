#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "UserWidget.h"
#include "HealthBar.generated.h"

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

	void InitHealthBar(ASaiyoraPlayerCharacter* OwningPlayer);

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
	UDamageHandler* OwnerDamageHandler = nullptr;
	UPROPERTY()
	USaiyoraUIDataAsset* UIDataAsset = nullptr;

	UFUNCTION()
	void UpdateHealth(AActor* Actor, const float PreviousHealth, const float NewHealth);
	UFUNCTION()
	void UpdateLifeStatus(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
};
