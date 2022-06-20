#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "CombatEnums.h"
#include "Hitbox.generated.h"

class ASaiyoraGameState;

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API UHitbox : public UBoxComponent
{
	GENERATED_BODY()
	
public:

	UHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	
private:

	void UpdateFactionCollision(EFaction const NewFaction);
	UPROPERTY()
	ASaiyoraGameState* GameState;
};
