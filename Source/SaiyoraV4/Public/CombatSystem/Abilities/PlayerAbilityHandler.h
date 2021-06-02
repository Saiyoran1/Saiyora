// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatSystem/Abilities/AbilityHandler.h"
#include "PlayerAbilityHandler.generated.h"

UCLASS()
class SAIYORAV4_API UPlayerAbilityHandler : public UAbilityHandler
{
	GENERATED_BODY()

	UPlayerAbilityHandler();
	virtual void InitializeComponent() override;

	UFUNCTION()
	void AddNewAbilityToBars(UCombatAbility* NewAbility);

	int32 FindFirstOpenAbilityBind(ESaiyoraPlane const BarPlane) const;

	static const int32 AbilitiesPerBar;
	
	UPROPERTY()
	TMap<int32, UCombatAbility*> AncientAbilities;
	UPROPERTY()
	TMap<int32, UCombatAbility*> ModernAbilities;

	FAbilityBindingNotification OnAbilityBindUpdated;

public:

	UFUNCTION(BlueprintCallable)
	void SubscribeToAbilityBindUpdated(FAbilityBindingCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromAbilityBindUpdated(FAbilityBindingCallback const& Callback);
};