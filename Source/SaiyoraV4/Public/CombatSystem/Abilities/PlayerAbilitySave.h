// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "GameFramework/SaveGame.h"
#include "PlayerAbilitySave.generated.h"

UCLASS()
class SAIYORAV4_API UPlayerAbilitySave : public USaveGame
{
	GENERATED_BODY()

public:
	
	UPlayerAbilitySave();

private:
	
	UPROPERTY()
	TSet<TSubclassOf<UCombatAbility>> UnlockedAbilities;
	UPROPERTY()
	FPlayerAbilityLoadout PlayerAbilityLoadout;

	//TODO: Add talent selections later.

public:

	void AddUnlockedAbility(TSubclassOf<UCombatAbility> const NewAbility);
	void ModifyLoadout(EActionBarType const BarType, int32 const SlotNumber, TSubclassOf<UCombatAbility> const AbilityClass);

	void GetUnlockedAbilities(TSet<TSubclassOf<UCombatAbility>>& OutAbilities);
	void GetLastSavedLoadout(FPlayerAbilityLoadout& OutLoadout);
};
