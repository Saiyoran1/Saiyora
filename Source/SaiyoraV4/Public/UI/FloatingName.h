#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "TextBlock.h"
#include "UserWidget.h"
#include "FloatingName.generated.h"

class UCombatStatusComponent;

UCLASS()
class SAIYORAV4_API UFloatingName : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(const UCombatStatusComponent* CombatStatusComponent);

private:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UTextBlock* NameText;
	UPROPERTY(EditDefaultsOnly, Category = "Name")
	int32 MaxNameLength = 20;

	UFUNCTION()
	void UpdateName(const FName InName, const EFaction OwnerFaction = EFaction::Neutral);
};
