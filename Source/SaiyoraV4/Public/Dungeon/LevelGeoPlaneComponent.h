﻿#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "Components/ActorComponent.h"
#include "LevelGeoPlaneComponent.generated.h"

class UCombatStatusComponent;
class ASaiyoraGameState;
class ASaiyoraPlayerCharacter;

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
	ESaiyoraPlane DefaultPlane = ESaiyoraPlane::Both;
	UPROPERTY(EditDefaultsOnly, Category = "Plane")
	UMaterialInterface* XPlaneMaterial = nullptr;

	UPROPERTY()
	TMap<UMeshComponent*, FMeshMaterials> Materials;
	UPROPERTY()
	UCombatStatusComponent* LocalPlayerCombatStatusComponent = nullptr;
	bool bIsRenderedXPlane = false;

	UFUNCTION()
	void OnLocalPlayerPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source);

	void SetInitialCollision();
	void SetupMaterialSwapping();
	void UpdateCameraCollision();

	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;
	UFUNCTION()
	void OnPlayerAdded(ASaiyoraPlayerCharacter* NewPlayer);
};
