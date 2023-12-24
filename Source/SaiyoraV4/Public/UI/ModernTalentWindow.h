#pragma once
#include "CoreMinimal.h"
#include "ModernSpecialization.h"
#include "UserWidget.h"
#include "ModernTalentWindow.generated.h"

UCLASS()
class SAIYORAV4_API UModernTalentWindow : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeOnInitialized() override;
	UFUNCTION(BlueprintPure)
	ASaiyoraPlayerCharacter* GetOwningSaiyoraPlayer() const { return OwningPlayer; }

	UFUNCTION(BlueprintCallable)
	void SelectSpec(const TSubclassOf<UModernSpecialization> Spec) { CurrentSpec = Spec; }
	UFUNCTION(BlueprintCallable)
	void SelectTalent(const int32 Slot, const TSubclassOf<UCombatAbility> Talent);
	UFUNCTION(BlueprintCallable)
	void SwapTalentSlots(const int32 FirstSlot, const int32 SecondSlot);
	UFUNCTION(BlueprintCallable)
	void SaveLayout();

	UFUNCTION(BlueprintPure)
	TSubclassOf<UModernSpecialization> GetCurrentSpec() const { return CurrentSpec; }
	UFUNCTION(BlueprintPure)
	FModernSpecLayout GetCurrentLayout() const { return Layouts.FindRef(CurrentSpec); }
	UFUNCTION(BlueprintPure)
	void GetModernLayouts(TArray<FModernSpecLayout>& OutLayouts) const { Layouts.GenerateValueArray(OutLayouts); }
	UFUNCTION(BlueprintPure)
	bool IsLayoutDirty() const;

private:

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer = nullptr;
	TMap<TSubclassOf<UModernSpecialization>, FModernSpecLayout> Layouts;
	TSubclassOf<UModernSpecialization> CurrentSpec;
	FModernSpecLayout LastSavedLayout;
};
