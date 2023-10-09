#pragma once
#include "CoreMinimal.h"
#include "TextBlock.h"
#include "UserWidget.h"
#include "FloatingName.generated.h"

class UCombatStatusComponent;

UCLASS()
class SAIYORAV4_API UFloatingName : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(UCombatStatusComponent* CombatStatusComponent);

private:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UTextBlock* NameText;
	UPROPERTY(EditDefaultsOnly, Category = "Name")
	int32 MaxNameLength = 20;

	UPROPERTY()
	UCombatStatusComponent* OwnerCombatStatus;
	FName CurrentName;

	UFUNCTION()
	void UpdateName(const FName PreviousName, const FName NewName);
};
