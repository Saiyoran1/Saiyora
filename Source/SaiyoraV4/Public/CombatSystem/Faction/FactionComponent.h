#pragma once
#include "CoreMinimal.h"
#include "SaiyoraEnums.h"
#include "FactionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UFactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFactionComponent();
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Faction")
	EFaction GetCurrentFaction() const { return DefaultFaction; }

private:

	UPROPERTY(EditAnywhere, Category = "Faction")
	EFaction DefaultFaction = EFaction::None;

	UPROPERTY()
	TArray<UMeshComponent*> OwnerMeshes;
	UFUNCTION()
	void UpdateOwnerCustomRendering();
};
