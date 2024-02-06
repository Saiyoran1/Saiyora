#pragma once
#include "CoreMinimal.h"
#include "CombatDebugOptions.generated.h"

struct FAbilityEvent;

UCLASS()
class SAIYORAV4_API UCombatDebugOptions : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Abilities")
	bool bLogAbilityEvents = false;
	void LogAbilityEvent(const AActor* Actor, const FAbilityEvent& Event);
};
