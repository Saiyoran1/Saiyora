#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatStatusStructs.h"
#include "CombatStructs.h"
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
	
	void AddPlaneSwapRestriction(const FPlaneSwapRestriction& Restriction) { PlaneSwapRestrictions.Add(Restriction); }
	void RemovePlaneSwapRestriction(const FPlaneSwapRestriction& Restriction) { PlaneSwapRestrictions.Remove(Restriction); }
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Plane")
	ESaiyoraPlane DefaultPlane = ESaiyoraPlane::None;
	UPROPERTY(EditAnywhere, Category = "Plane")
	bool bCanEverPlaneSwap = false;
	UPROPERTY(ReplicatedUsing = OnRep_PlaneStatus)
	FPlaneStatus PlaneStatus;
	UFUNCTION()
	void OnRep_PlaneStatus(const FPlaneStatus& PreviousStatus);
	
	TRestrictionList<FPlaneSwapRestriction> PlaneSwapRestrictions;

	//Faction

public:
	
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
	
	static const int32 DEFAULTENEMYSAMEPLANE;
	static const int32 ENEMYSAMEPLANESTART;
	static const int32 ENEMYSAMEPLANEEND;
	static const int32 DEFAULTENEMYXPLANE;
	static const int32 ENEMYXPLANESTART;
	static const int32 ENEMYXPLANEEND;
	static const int32 DEFAULTNEUTRALSAMEPLANE;
	static const int32 NEUTRALSAMEPLANESTART;
	static const int32 NEUTRALSAMEPLANEEND;
	static const int32 DEFAULTNEUTRALXPLANE;
	static const int32 NEUTRALXPLANESTART;
	static const int32 NEUTRALXPLANEEND;
	static const int32 DEFAULTSTENCIL;
	static const int32 FRIENDLYSAMEPLANESTART;
	static const int32 FRIENDLYSAMEPLANEEND;
	static const int32 FRIENDLYXPLANESTART;
	static const int32 FRIENDLYXPLANEEND;

	void UpdateOwnerCustomRendering();
	bool UpdateStencilValue();
	static TMap<int32, UCombatStatusComponent*> StencilValues;
	int32 StencilValue = 200;
	bool bUseCustomDepth = false;
	
	UFUNCTION()
	void OnLocalPlayerPlaneSwap(const ESaiyoraPlane Previous, const ESaiyoraPlane New, UObject* Source) { UpdateOwnerCustomRendering(); }

	//Collision

private:

	void UpdateOwnerPlaneCollision();
};