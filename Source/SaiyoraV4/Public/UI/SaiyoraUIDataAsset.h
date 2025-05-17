#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SaiyoraUIDataAsset.generated.h"

class UDeathOverlay;
class UPlayerHUD;
enum class EElementalSchool : uint8;
class USaiyoraErrorMessage;

UCLASS()
class SAIYORAV4_API USaiyoraUIDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	
	USaiyoraUIDataAsset();

	UPROPERTY(EditAnywhere, Category = "Widget Classes")
	TSubclassOf<UPlayerHUD> PlayerHUDClass;
	UPROPERTY(EditAnywhere, Category = "Widget Classes")
	TSubclassOf<USaiyoraErrorMessage> ErrorMessageWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Widget Classes")
	TSubclassOf<UDeathOverlay> DeathOverlayClass;

	UPROPERTY(EditAnywhere, Category = "Colors")
	TMap<EElementalSchool, FLinearColor> SchoolColors;
	FLinearColor GetSchoolColor(const EElementalSchool School) const { return SchoolColors.FindRef(School); }
	
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor PlayerHealthColor = FLinearColor::Red;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor PlayerDeadColor = FLinearColor::Black;
	
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor EnemyHealthColor = FLinearColor::Red;
	
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor DamageOutlineColor = FLinearColor::Black;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor HealingOutlineColor = FLinearColor::Green;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor AbsorbOutlineColor = FLinearColor::Blue;
	
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor UninterruptibleCastColor = FLinearColor::Gray;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor InterruptedCastColor = FLinearColor::Red;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor CancelledCastColor = FLinearColor::Gray;
	
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor DefaultTextColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor SuccessTextColor = FLinearColor::Green;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor FailureTextColor = FLinearColor::Red;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor StartProgressColor = FLinearColor::Yellow;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor EndProgressColor = FLinearColor::Red;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor SuccessProgressColor = FLinearColor::Green;
	UPROPERTY(EditAnywhere, Category = "Colors")
	FLinearColor FailureProgressColor = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, Category = "Icons")
	UTexture2D* UninterruptibleCastIcon = nullptr;

	UPROPERTY(EditAnywhere, Category = "Resources")
	float HUDResourceBarRadius = 240.0f;
	UPROPERTY(EditAnywhere, Category = "Resources")
	float HUDResourceBarAngleExtent = 110.0f;
	UPROPERTY(EditAnywhere, Category = "Resources")
	FLinearColor InactiveDiscreteResourceIconColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "Ability")
	UTexture2D* InvalidAbilityTexture;
	UPROPERTY(EditAnywhere, Category = "Ability")
	FLinearColor InvalidAbilityTint = FLinearColor::Transparent;
};
