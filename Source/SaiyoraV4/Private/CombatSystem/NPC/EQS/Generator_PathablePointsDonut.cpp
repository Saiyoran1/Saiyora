#include "EQS/Generator_PathablePointsDonut.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"

UEQG_PathablePointsDonut::UEQG_PathablePointsDonut()
{
	ItemType = UEnvQueryItemType_Point::StaticClass();
	MinRange.DefaultValue = 0.0f;
	MaxRange.DefaultValue = 500.0f;
	DistanceBetweenPoints.DefaultValue = 100.0f;
	ProjectionData.TraceMode = EEnvQueryTrace::Navigation;
}

void UEQG_PathablePointsDonut::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	TArray<AActor*> CenterActors;
	QueryInstance.PrepareContext(CenterActor, CenterActors);
	if (CenterActors.Num() <= 0)
	{
		return;
	}
	const FVector CenterLoc = CenterActors[0]->GetActorLocation();
	
	const UObject* BindOwner = QueryInstance.Owner.Get();
	MinRange.BindData(BindOwner, QueryInstance.QueryID);
	MaxRange.BindData(BindOwner, QueryInstance.QueryID);
	DistanceBetweenPoints.BindData(BindOwner, QueryInstance.QueryID);

	const float ClampedDistanceBetweenPoints = FMath::Max(1.0f, DistanceBetweenPoints.GetValue());
	const float ClampedMinRange = MinRange.GetValue() <= 0.0f ? ClampedDistanceBetweenPoints : MinRange.GetValue();
	const float ClampedMaxRange = FMath::Max(MaxRange.GetValue(), ClampedMinRange + 1.0f);
	const int NumAdditionalRings = ((ClampedMaxRange - ClampedMinRange) / ClampedDistanceBetweenPoints);

	TArray<FNavLocation> Points;
	for (int i = 0; i <= NumAdditionalRings; i++)
	{
		const float Radius = ClampedMinRange + (i * ClampedDistanceBetweenPoints);
		const float Circumference = 2 * PI * Radius;
		const int NumPoints = Circumference / ClampedDistanceBetweenPoints;
		const float AngleBetweenPoints = NumPoints == 0 ? 0.0f : 360.0f / NumPoints;
		for (int j = 0; j < NumPoints; j++)
		{
			const FVector Direction = FVector::ForwardVector.RotateAngleAxis(AngleBetweenPoints * j, FVector::UpVector);
			Points.Add(FNavLocation(CenterLoc + (Direction * Radius)));
		}
	}

	ProjectAndFilterNavPoints(Points, QueryInstance);
	StoreNavPoints(Points, QueryInstance);
}