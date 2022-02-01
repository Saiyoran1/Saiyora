#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "SaiyoraEnums.h"
#include "Hitbox.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API UHitbox : public UBoxComponent
{
	GENERATED_BODY()
public:
	static const int32 SnapshotBufferSize;
	static const float MaxLagCompTime;
	UHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
private:
	void UpdateFactionCollision(EFaction const NewFaction);
	UPROPERTY()
	class ASaiyoraGameMode* GameModeRef;
};
