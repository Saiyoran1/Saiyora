#include "PatrolRoute.h"
#include "NPCStructs.h"

#pragma region Patrol Route Component
#if WITH_EDITOR

void UPatrolRouteComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property != nullptr && IsValid(OwningPatrolRoute)
		&& PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USplineComponent, SplineCurves))
	{
		OwningPatrolRoute->UpdatePatrolPoints();
	}
}

void UPatrolRouteComponent::OnRegister()
{
	Super::OnRegister();
	
	OwningPatrolRoute = Cast<APatrolRoute>(GetOwner());
	if (IsValid(OwningPatrolRoute))
	{
		OwningPatrolRoute->UpdatePatrolPoints();
	}
}

#endif
#pragma endregion
#pragma region Patrol Route Actor

APatrolRoute::APatrolRoute()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RootComponent = SplineComponent = CreateDefaultSubobject<UPatrolRouteComponent>(TEXT("Spline"));
}

#if WITH_EDITOR

void APatrolRoute::UpdatePatrolPoints()
{
	if (!IsValid(SplineComponent))
	{
		PatrolPoints.Empty();
		return;
	}
	TArray<FVector> Points;
	for (int i = 0; i < SplineComponent->GetNumberOfSplinePoints(); i++)
	{
		Points.Add(SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
	}
	
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

void APatrolRoute::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	
	UpdatePatrolPoints();
}

#endif
#pragma endregion 