#pragma once
#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "CombatLink.generated.h"

class UThreatHandler;

UCLASS()
class SAIYORAV4_API ACombatLink : public AActor
{
	GENERATED_BODY()

public:
	
	ACombatLink();

	UPROPERTY(EditAnywhere, Category = "Threat")
	TArray<AActor*> LinkedActors;

protected:
	
	virtual void BeginPlay() override;

private:

	bool bGroupInCombat = false;
	TMap<UThreatHandler*, bool> ThreatHandlers;
	UFUNCTION()
	void OnActorCombat(UThreatHandler* Handler, const bool bInCombat);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Display, meta = (AllowPrivateAccess = "true"))
	USphereComponent* Sphere;
};
