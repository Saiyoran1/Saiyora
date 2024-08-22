#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "ResourceContainer.generated.h"

class UVerticalBox;
class UResourceBar;
class UResource;
class UResourceHandler;
class ASaiyoraPlayerCharacter;
class UPlayerHUD;

UCLASS()
class SAIYORAV4_API UResourceContainer : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitResourceContainer(ASaiyoraPlayerCharacter* OwningPlayer);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UVerticalBox* ResourceBox;
	
	UPROPERTY()
	UResourceHandler* OwnerResource = nullptr;

	UFUNCTION()
	void OnResourceAdded(UResource* AddedResource);
	UFUNCTION()
	void OnResourceRemoved(UResource* RemovedResource);

	UPROPERTY()
	TMap<UResource*, UResourceBar*> ResourceWidgets;
};
