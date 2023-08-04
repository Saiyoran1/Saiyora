#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "AbilityCondition.generated.h"

class UCombatAbility;

UCLASS(Blueprintable)
class SAIYORAV4_API UAbilityCondition : public UObject
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintNativeEvent)
	bool IsConditionMet(AActor* Owner, TSubclassOf<UCombatAbility> AbilityClass, const float OptionalParameter, UObject* OptionaObject) const;
	virtual bool IsConditionMet_Implementation(AActor* Owner, TSubclassOf<UCombatAbility> AbilityClass, const float OptionalParameter, UObject* OptionaObject) const { return true; }
};
