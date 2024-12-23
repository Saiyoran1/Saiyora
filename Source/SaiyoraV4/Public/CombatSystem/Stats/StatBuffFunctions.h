#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "GameplayTagContainer.h"
#include "StatBuffFunctions.generated.h"

class UStatHandler;

//Buff function for applying stat modifiers that will last the duration of the buff.
//These modifiers can optionally update with buff stacks.
UCLASS()
class UStatModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnChange(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Stat Modifiers", meta = (Categories = "Stat"))
	TMap<FGameplayTag, FCombatModifier> StatMods;
	
	TMap<FGameplayTag, FCombatModifierHandle> StatModHandles;
	UPROPERTY()
	UStatHandler* TargetHandler = nullptr;
};