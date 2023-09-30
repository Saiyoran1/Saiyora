#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "Overlay.h"
#include "ProgressBar.h"
#include "TextBlock.h"
#include "WrapBox.h"
#include "Blueprint/UserWidget.h"
#include "FloatingHealthBar.generated.h"

class UDamageHandler;
class UBuffHandler;

UCLASS(Abstract)
class SAIYORAV4_API UFloatingHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(AActor* TargetActor);

private:

	UFUNCTION()
	void UpdateHealth(AActor* Actor, const float PreviousHealth, const float NewHealth);
	UFUNCTION()
	void UpdateAbsorb(AActor* Actor, const float PreviousHealth, const float NewHealth);
	UFUNCTION()
	void UpdateLifeStatus(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);

	//Widgets

private:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UOverlay* HealthOverlay;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UProgressBar* HealthBar;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UTextBlock* HealthText;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UProgressBar* AbsorbBar;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UWrapBox* BuffBox;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UWrapBox* DebuffBox;

	//Refs

private:

	UPROPERTY()
	UDamageHandler* TargetDamageHandler;
	UPROPERTY()
	UBuffHandler* TargetBuffHandler;
};
