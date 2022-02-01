#include "AbilityFunctionLibrary.h"

FAbilityTarget UAbilityFunctionLibrary::MakeAbilityTarget(int32 const IDNumber, FVector const& Origin,
	FVector const& AimLocation, FVector const& AimDirection, AActor* HitTarget)
{
	FAbilityTarget Target;
	Target.IDNumber = IDNumber;
	Target.Origin = Origin;
	Target.AimLocation = AimLocation;
	Target.AimDirection = AimDirection;
	Target.HitTarget = HitTarget;
	return Target;
}