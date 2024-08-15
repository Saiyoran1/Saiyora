#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "BossRequirementDisplay.generated.h"

class ADungeonGameState;
class UTextBlock;

UCLASS()
class SAIYORAV4_API UBossRequirementDisplay : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitializeBossRequirement(const FString& BossDisplayName);
	void UpdateRequirementMet(const bool bMet);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* TimestampText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* RequirementText;
};
