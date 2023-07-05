#pragma once
#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SaiyoraProjectileComponent.generated.h"

class APredictableProjectile;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API USaiyoraProjectileComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SetInitialCatchUpTime(APredictableProjectile* Projectile, const float CatchUpTime);

private:

	float RemainingLag = 0.0f;
	UPROPERTY()
	APredictableProjectile* OwningProjectile;
};
