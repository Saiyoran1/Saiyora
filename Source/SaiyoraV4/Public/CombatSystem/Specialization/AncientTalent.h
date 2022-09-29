#pragma once
#include "CoreMinimal.h"
#include "AncientSpecialization.h"
#include "Object.h"
#include "AncientTalent.generated.h"

UCLASS(Blueprintable)
class SAIYORAV4_API UAncientTalent : public UObject
{
	GENERATED_BODY()

public:

	void SelectTalent(UAncientSpecialization* OwningSpecialization) { OwningSpec = OwningSpecialization; OnTalentSelected(); }
	void UnselectTalent() { OnTalentUnselected(); }

protected:

	UFUNCTION(BlueprintPure, Category = "Talent")
	UAncientSpecialization* GetOwningSpec() const { return OwningSpec; }

	UFUNCTION(BlueprintNativeEvent)
	void OnTalentSelected();
	void OnTalentSelected_Implementation() {}
	UFUNCTION(BlueprintNativeEvent)
	void OnTalentUnselected();
	void OnTalentUnselected_Implementation() {}

private:

	UPROPERTY()
	UAncientSpecialization* OwningSpec;
};
