#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
#include "Object.h"
#include "UIBuffFunctions.generated.h"

class UPlayerHUD;
class UCombatAbility;

UCLASS()
class SAIYORAV4_API UAbilityProcFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	virtual void SetupBuffFunction() override;
	//TODO: Should probably add an OnRemove instead of making the ActionSlot bind to the delegate?
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;

private:

	UPROPERTY(EditAnywhere, Category = "Proc")
	TSubclassOf<UCombatAbility> AbilityClass;
	
	UPROPERTY()
	UPlayerHUD* LocalPlayerHUD = nullptr;
};