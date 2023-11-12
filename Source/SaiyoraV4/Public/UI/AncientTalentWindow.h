#pragma once
#include "CoreMinimal.h"
#include "SaiyoraPlayerCharacter.h"
#include "SpecializationStructs.h"
#include "UserWidget.h"
#include "AncientTalentWindow.generated.h"

class UAncientSpecialization;

UCLASS(Blueprintable)
class SAIYORAV4_API UAncientTalentWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable)
	void SelectSpec(const TSubclassOf<UAncientSpecialization> Spec) { CurrentSpec = Spec; }
	UFUNCTION(BlueprintCallable)
	void UpdateTalentChoice(const TSubclassOf<UCombatAbility> BaseAbility, const TSubclassOf<UAncientTalent> TalentSelection);
	UFUNCTION(BlueprintCallable)
	void SwapTalentRowSlots(const int32 FirstRow, const int32 SecondRow);

	UFUNCTION(BlueprintPure)
	TSubclassOf<UAncientSpecialization> GetCurrentSpec() const { return CurrentSpec; }
	UFUNCTION(BlueprintPure)
	FAncientSpecLayout GetCurrentLayout() const { return Layouts.FindRef(CurrentSpec); }
	UFUNCTION(BlueprintPure)
	bool IsLayoutDirty() const;

	UFUNCTION(BlueprintCallable)
	void SaveLayout();

	UFUNCTION(BlueprintPure)
	ASaiyoraPlayerCharacter* GetOwningSaiyoraPlayer() const { return OwningPlayer; }

private:

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer = nullptr;
	TMap<TSubclassOf<UAncientSpecialization>, FAncientSpecLayout> Layouts;
	TSubclassOf<UAncientSpecialization> CurrentSpec;
	FAncientSpecLayout LastSavedLayout;
};
