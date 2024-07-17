#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SaiyoraUIDataAsset.generated.h"

enum class EElementalSchool : uint8;
class USaiyoraErrorMessage;

UCLASS()
class SAIYORAV4_API USaiyoraUIDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	
	USaiyoraUIDataAsset();

	UPROPERTY(EditAnywhere, Category = "Errors")
	TSubclassOf<USaiyoraErrorMessage> ErrorMessageWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Colors")
	TMap<EElementalSchool, FLinearColor> SchoolColors;
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
};
