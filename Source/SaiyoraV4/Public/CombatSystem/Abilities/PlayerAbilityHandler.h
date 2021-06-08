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
	virtual void BeginPlay() override;

	UFUNCTION()
	void SetupNewAbilityBind(UCombatAbility* NewAbility);
	UFUNCTION()
	void UpdateAbilityPlane(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source);

	static const int32 AbilitiesPerBar;
	
	UPROPERTY()
	TMap<int32, UCombatAbility*> AncientBinds;
	UPROPERTY()
	TMap<int32, UCombatAbility*> ModernBinds;
	UPROPERTY()
	TMap<int32, UCombatAbility*> HiddenBinds;

	FAbilityBindingNotification OnAbilityBindUpdated;
	FBarSwapNotification OnBarSwap;

	ESaiyoraPlane AbilityPlane = ESaiyoraPlane::None;

public:

	UFUNCTION(BlueprintCallable)
	void AbilityInput(int32 const BindNumber, bool const bHidden);
	
	UFUNCTION(BlueprintCallable)
	void SubscribeToAbilityBindUpdated(FAbilityBindingCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromAbilityBindUpdated(FAbilityBindingCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void SubscribeToBarSwap(FBarSwapCallback const& Callback);
	UFUNCTION(BlueprintCallable)
	void UnsubscribeFromBarSwap(FBarSwapCallback const& Callback);
};