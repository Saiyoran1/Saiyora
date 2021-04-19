// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "SaiyoraEnums.h"
#include "Components/ActorComponent.h"
#include "SaiyoraPlaneComponent.generated.h"

USTRUCT()
struct FPlaneStatus
{
	GENERATED_BODY()

	UPROPERTY()
	ESaiyoraPlane CurrentPlane = ESaiyoraPlane::None;
	UPROPERTY()
	UObject* LastSwapSource = nullptr;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_FourParams(bool, FPlaneSwapCondition, class USaiyoraPlaneComponent*, Target, UObject*, Source, bool const, bToSpecificPlane, ESaiyoraPlane const, TargetPlane);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FPlaneSwapCallback, ESaiyoraPlane const, PreviousPlane, ESaiyoraPlane const, NewPlane, UObject*, Source);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlaneSwapNotification, ESaiyoraPlane const, PreviousPlane, ESaiyoraPlane const, NewPlane, UObject*, Source);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API USaiyoraPlaneComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	USaiyoraPlaneComponent();
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	ESaiyoraPlane GetCurrentPlane() const { return PlaneStatus.CurrentPlane; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Plane")
	ESaiyoraPlane PlaneSwap(bool const bIgnoreRestrictions, UObject* Source, bool const bToSpecificPlane, ESaiyoraPlane const TargetPlane = ESaiyoraPlane::None);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Plane")
	void AddPlaneSwapRestriction(FPlaneSwapCondition const& Condition);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Plane")
	void RemovePlaneSwapRestriction(FPlaneSwapCondition const& Condition);
	UFUNCTION(BlueprintCallable, Category = "Plane")
	void SubscribeToPlaneSwap(FPlaneSwapCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Plane")
	void UnsubscribeFromPlaneSwap(FPlaneSwapCallback const& Callback);

private:

	UPROPERTY(EditDefaultsOnly, Category = "Plane")
	ESaiyoraPlane DefaultPlane = ESaiyoraPlane::None;
	UPROPERTY(EditDefaultsOnly, Category = "Plane")
	bool bCanEverPlaneSwap = false;

	UPROPERTY(ReplicatedUsing = OnRep_PlaneStatus)
	FPlaneStatus PlaneStatus;

	UFUNCTION()
	void OnRep_PlaneStatus(FPlaneStatus const PreviousStatus);

	FPlaneSwapNotification OnPlaneSwapped;
	TArray<FPlaneSwapCondition> PlaneSwapRestrictions;
};