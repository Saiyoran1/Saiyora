#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "Components/ActorComponent.h"
#include "LevelGeoPlaneComponent.generated.h"

class UPlaneComponent;

USTRUCT()
struct FMeshMaterials
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<int32, UMaterialInterface*> Materials;
};

UCLASS(Abstract, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API ULevelGeoPlaneComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	ULevelGeoPlaneComponent();
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, Category = "Plane")
	ESaiyoraPlane DefaultPlane = ESaiyoraPlane::None;
	UPROPERTY(EditDefaultsOnly, Category = "Plane")
	UMaterialInterface* XPlaneMaterial;

	UPROPERTY()
	TMap<UMeshComponent*, FMeshMaterials> Materials;
	UPROPERTY()
	UPlaneComponent* LocalPlayerPlaneComponent;
	bool bIsRenderedXPlane = false;

	UFUNCTION()
	void OnLocalPlayerPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source);
};
