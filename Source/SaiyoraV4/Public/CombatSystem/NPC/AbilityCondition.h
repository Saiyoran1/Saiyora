#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "AbilityCondition.generated.h"

UCLASS(Blueprintable)
class SAIYORAV4_API UAbilityCondition : public UObject
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintNativeEvent)
	bool IsConditionMet(AActor* Owner) const;
	virtual bool IsConditionMet_Implementation(AActor* Owner) const { return true; }
};
