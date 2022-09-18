#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatStatusStructs.h"
#include "CombatStatusComponent.generated.h"

class UBuff;
class ASaiyoraGameState;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UCombatStatusComponent : public UActorComponent
{
	GENERATED_BODY()

//Setup

public:
	
	UCombatStatusComponent();
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

private:

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;

//Plane

public:

	UFUNCTION(BlueprintPure, Category = "Plane")
	ESaiyoraPlane GetCurrentPlane() const { return PlaneStatus.CurrentPlane; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Plane")
	ESaiyoraPlane PlaneSwap(const bool bIgnoreRestrictions, UObject* Source, const bool bToSpecificPlane, const ESaiyoraPlane TargetPlane = ESaiyoraPlane::None);
	UFUNCTION(BlueprintPure, Category = "Plane")
	static bool CheckForXPlane(const ESaiyoraPlane FromPlane, const ESaiyoraPlane ToPlane);

	UPROPERTY(BlueprintAssignable)
	FPlaneSwapNotification OnPlaneSwapped;
	
	void AddPlaneSwapRestriction(UBuff* Source, const FPlaneSwapRestriction& Restriction);
	void RemovePlaneSwapRestriction(const UBuff* Source);
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Plane")
	ESaiyoraPlane DefaultPlane = ESaiyoraPlane::None;
	UPROPERTY(EditAnywhere, Category = "Plane")
	bool bCanEverPlaneSwap = false;
	UPROPERTY(ReplicatedUsing = OnRep_PlaneStatus)
	FPlaneStatus PlaneStatus;
	UFUNCTION()
	void OnRep_PlaneStatus(const FPlaneStatus& PreviousStatus);
	
	UPROPERTY()
	TMap<UBuff*, FPlaneSwapRestriction> PlaneSwapRestrictions;

	//Faction

public:

	//TODO: Implement faction swapping if needed.
	UFUNCTION(BlueprintPure, Category = "Faction")
	EFaction GetCurrentFaction() const { return DefaultFaction; }
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Faction")
	EFaction DefaultFaction = EFaction::None;

	//Rendering

private:
	
	UPROPERTY()
	UCombatStatusComponent* LocalPlayerStatusComponent;
	UPROPERTY()
	TArray<UMeshComponent*> OwnerMeshes;
	UFUNCTION()
	void OnLocalPlayerPlaneSwap(const ESaiyoraPlane Previous, const ESaiyoraPlane New, UObject* Source) { UpdateOwnerCustomRendering(); }
	void UpdateOwnerCustomRendering();
	int32 CurrentPlaneID = 0;
	int32 StencilValue = 200;
	static TMap<int32, UCombatStatusComponent*> RenderingIDs;
	void AssignXPlaneID();
	void AssignSamePlaneID();

	//Collision

private:

	void UpdateOwnerPlaneCollision();
};