#pragma once
#include "CoreMinimal.h"
#include "DungeonGameState.h"
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
	void SelectSpec(const TSubclassOf<UModernSpecialization> Spec);
	UFUNCTION(BlueprintCallable)
	void SelectTalent(const int32 SlotNumber, const TSubclassOf<UCombatAbility> Talent);
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
	UFUNCTION(BlueprintPure)
	bool IsEnabled() const { return bEnabled; }

protected:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PostChange();

private:

	TMap<TSubclassOf<UModernSpecialization>, FModernSpecLayout> Layouts;
	TSubclassOf<UModernSpecialization> CurrentSpec;
	FModernSpecLayout LastSavedLayout;
	bool bEnabled = false;

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer = nullptr;
	UPROPERTY()
	ADungeonGameState* GameStateRef = nullptr;
	UFUNCTION()
	void OnDungeonPhaseChanged(const EDungeonPhase PreviousPhase, const EDungeonPhase NewPhase)
	{
		bEnabled = NewPhase == EDungeonPhase::WaitingToStart;
		PostChange();
	}
};
