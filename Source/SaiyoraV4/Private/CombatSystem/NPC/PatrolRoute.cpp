#include "PatrolRoute.h"
#include "NPCStructs.h"

#if WITH_EDITOR
void UPatrolRouteComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr)
	{
		if (IsValid(OwningPatrolRoute))
		{
			TArray<FVector> Locations;
			for (int i = 0; i < GetNumberOfSplinePoints(); i++)
			{
				Locations.Add(GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
			}
			OwningPatrolRoute->UpdatePatrolPointsFromComponent(Locations);
		}
	}
}

void UPatrolRouteComponent::OnRegister()
{
	Super::OnRegister();
	OwningPatrolRoute = Cast<APatrolRoute>(GetOwner());
	if (IsValid(OwningPatrolRoute))
	{
		TArray<FVector> Locations;
		for (int i = 0; i < GetNumberOfSplinePoints(); i++)
		{
			Locations.Add(GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
		}
		OwningPatrolRoute->UpdatePatrolPointsFromComponent(Locations);
	}
}
#endif

APatrolRoute::APatrolRoute()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RootComponent = SplineComponent = CreateDefaultSubobject<UPatrolRouteComponent>(TEXT("Spline"));
}

#if WITH_EDITOR

void APatrolRoute::UpdatePatrolPointsFromComponent(const TArray<FVector>& Points)
{
	while (PatrolPoints.Num() > Points.Num())
	{
		PatrolPoints.RemoveAt(PatrolPoints.Num() - 1);
	}
	while (PatrolPoints.Num() < Points.Num())
	{
		PatrolPoints.AddDefaulted();
	}
	for (int i = 0; i < PatrolPoints.Num(); i++)
	{
		PatrolPoints[i].Location = Points[i];
	}
}

#endif