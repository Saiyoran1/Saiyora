#include "SaiyoraProjectileComponent.h"
#include "PredictableProjectile.h"

void USaiyoraProjectileComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                FActorComponentTickFunction* ThisTickFunction)
{
	if (GetOwnerRole() == ROLE_Authority && RemainingLag > 0.0f)
	{
		//Calculate time to add, we want to tick twice as fast until we catch up.
		const float AddTime = FMath::Min(DeltaTime, RemainingLag);
		RemainingLag -= AddTime;
		Super::TickComponent(DeltaTime + AddTime, TickType, ThisTickFunction);
		if (RemainingLag <= 0.0f)
		{
			if (IsValid(OwningProjectile))
			{
				OwningProjectile->MarkCatchupComplete();
			}
		}
	}
	else
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}
}

void USaiyoraProjectileComponent::SetInitialCatchUpTime(APredictableProjectile* Projectile, const float CatchUpTime)
{
	OwningProjectile = Projectile;
	RemainingLag = CatchUpTime;
	if (RemainingLag <= 0.0f)
	{
		if (IsValid(OwningProjectile))
		{
			OwningProjectile->MarkCatchupComplete();
		}
	}
}

