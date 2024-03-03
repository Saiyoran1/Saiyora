#pragma once
#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "PatrolRoute.generated.h"

struct FPatrolPoint;

UCLASS()
class SAIYORAV4_API UPatrolRouteComponent : public USplineComponent
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	
public:

	UPROPERTY()
	class APatrolRoute* OwningPatrolRoute = nullptr;

private:
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void OnRegister() override;

#endif
};

UENUM()
enum class ESplineOperation : uint8
{
	None,
	Add,
	Remove,
	Change
};

//Actor in the world that represents the patrol route for an NPC.
UCLASS()
class SAIYORAV4_API APatrolRoute : public AActor
{
	GENERATED_BODY()

public:

	APatrolRoute();
	void GetPatrolRoute(TArray<FPatrolPoint>& OutPatrolPoints) const { OutPatrolPoints = PatrolPoints; }

private:

	UPROPERTY(EditAnywhere)
	UPatrolRouteComponent* SplineComponent = nullptr;
	UPROPERTY(EditAnywhere, Category = "Patrol")
	TArray<FPatrolPoint> PatrolPoints;

#if WITH_EDITORONLY_DATA

public:

	void UpdatePatrolPointsFromComponent(const TArray<FVector>& Points);

#endif
};