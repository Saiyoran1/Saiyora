// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CrowdControl.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CrowdControlHandler.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UCrowdControlHandler : public UActorComponent
{
	GENERATED_BODY()

public:

	static const FGameplayTag GenericCrowdControlTag;
	
	UCrowdControlHandler();
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	TArray<TSubclassOf<UCrowdControl>> GetActiveCcTypes() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	void ApplyCrowdControl(UBuff* Source, TSubclassOf<UCrowdControl> const CrowdControlType);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	void RemoveCrowdControl(UBuff* Source, TSubclassOf<UCrowdControl> const CrowdControlType);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	void UpdateCrowdControlStatus(TSubclassOf<UCrowdControl> const CrowdControlType, UBuff* RefreshedBuff);

	UCrowdControl* GetCrowdControlObject(TSubclassOf<UCrowdControl> const CrowdControlType);

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void SubscribeToCrowdControlChanged(FCrowdControlCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void UnsubscribeFromCrowdControlChanged(FCrowdControlCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	void AddCrowdControlRestriction(FCrowdControlRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	void RemoveCrowdControlRestriction(FCrowdControlRestriction const& Restriction);

	void UpdateCrowdControlFromReplication(FReplicatedCrowdControl const& ReplicatedCrowdControl);
	void RemoveCrowdControlFromReplication(TSubclassOf<UCrowdControl> const CrowdControlClass);

private:

	UPROPERTY()
	TMap<TSubclassOf<UCrowdControl>, UCrowdControl*> CrowdControls;
	TArray<FCrowdControlRestriction> CrowdControlRestrictions;
	bool CheckCrowdControlRestricted(UBuff* Source, TSubclassOf<UCrowdControl> const CrowdControlType);
	UFUNCTION()
	bool RestrictImmunedCrowdControls(UBuff* Source, TSubclassOf<UCrowdControl> CrowdControlType);
	void RemoveNewlyRestrictedCrowdControls(FCrowdControlRestriction const& NewRestriction);
	
	UPROPERTY(EditAnywhere, Category = "Crowd Control", meta = (AllowPrivateAccess = true))
	TArray<TSubclassOf<UCrowdControl>> CrowdControlImmunities;

	FCrowdControlNotification OnCrowdControlChanged;

	UPROPERTY(Replicated)
	FReplicatedCrowdControlArray ReplicatedCrowdControls;
};