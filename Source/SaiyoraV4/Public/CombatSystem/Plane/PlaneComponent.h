#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlaneStructs.h"
#include "PlaneComponent.generated.h"

class UBuff;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UPlaneComponent : public UActorComponent
{
	GENERATED_BODY()

//Setup

public:
	
	UPlaneComponent();
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

private:

	UPROPERTY()
	UPlaneComponent* LocalPlayerPlaneComponent;
	UPROPERTY()
	TArray<UMeshComponent*> OwnerMeshes;
	FPlaneSwapCallback LocalPlayerSwapCallback;
	UFUNCTION()
	void OnLocalPlayerPlaneSwap(ESaiyoraPlane const Previous, ESaiyoraPlane const New, UObject* Source) { UpdateOwnerCustomRendering(); }

//Plane

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	ESaiyoraPlane GetCurrentPlane() const { return PlaneStatus.CurrentPlane; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Plane")
	ESaiyoraPlane PlaneSwap(bool const bIgnoreRestrictions, UObject* Source, bool const bToSpecificPlane, ESaiyoraPlane const TargetPlane = ESaiyoraPlane::None);
	UFUNCTION(BlueprintCallable, Category = "Plane")
	void SubscribeToPlaneSwap(FPlaneSwapCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Plane")
	void UnsubscribeFromPlaneSwap(FPlaneSwapCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Plane")
	static bool CheckForXPlane(ESaiyoraPlane const FromPlane, ESaiyoraPlane const ToPlane);
	void AddPlaneSwapRestriction(UBuff* Source, FPlaneSwapRestriction const& Restriction);
	void RemovePlaneSwapRestriction(UBuff* Source);
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Plane")
	ESaiyoraPlane DefaultPlane = ESaiyoraPlane::None;
	UPROPERTY(EditAnywhere, Category = "Plane")
	bool bCanEverPlaneSwap = false;
	UPROPERTY(ReplicatedUsing = OnRep_PlaneStatus)
	FPlaneStatus PlaneStatus;
	UFUNCTION()
	void OnRep_PlaneStatus(FPlaneStatus const PreviousStatus);
	FPlaneSwapNotification OnPlaneSwapped;
	UPROPERTY()
	TMap<UBuff*, FPlaneSwapRestriction> PlaneSwapRestrictions;
	UFUNCTION()
	void UpdateOwnerCustomRendering();
};