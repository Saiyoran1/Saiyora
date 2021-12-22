#pragma once
#include "CoreMinimal.h"
#include "CombatSystem/Abilities/CombatAbility.h"
#include "PlayerCombatAbility.generated.h"

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UPlayerCombatAbility : public UCombatAbility
{
	GENERATED_BODY()

//Display Info
	UPROPERTY(EditDefaultsOnly, Category = "Display Info")
	bool bHiddenCastBar = false;
	UPROPERTY(EditDefaultsOnly, Category = "Display Info")
	bool bHiddenOnActionBar = false;
	UPROPERTY(EditDefaultsOnly, Category = "Display Info")
	EActionBarType ActionBar = EActionBarType::None;
	UPROPERTY(EditDefaultsOnly, Category = "Cast Info")
    TSet<int32> TicksWithPredictionParams;
public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetHiddenCastBar() const { return bHiddenCastBar; }
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetHiddenOnActionBar() const { return bHiddenOnActionBar; }
	UFUNCTION(BlueprintCallable, BlueprintPure)
	EActionBarType GetActionBar() const { return ActionBar; }
//Cooldown
public:
	void UpdatePredictedChargesFromServer(int32 const PredictionID, int32 const ChargesSpent);
};
