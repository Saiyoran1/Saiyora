#include "SaiyoraProjectileComponent.h"


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
			//TODO: Call projectile parent catchup complete.
		}
	}
	else
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}
}

void USaiyoraProjectileComponent::SetInitialCatchUpTime(const float CatchUpTime)
{
	RemainingLag = CatchUpTime;
}

