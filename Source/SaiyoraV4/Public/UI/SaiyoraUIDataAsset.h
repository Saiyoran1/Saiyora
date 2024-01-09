#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SaiyoraUIDataAsset.generated.h"

class USaiyoraErrorMessage;

UCLASS()
class SAIYORAV4_API USaiyoraUIDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Errors")
	TSubclassOf<USaiyoraErrorMessage> ErrorMessageWidgetClass;
};
