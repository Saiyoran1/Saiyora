#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "NPCEnums.h"
#include "StatStructs.h"
#include "Components/SphereComponent.h"
#include "AggroRadius.generated.h"

class UNPCAbilityComponent;
class UStatHandler;
class UThreatHandler;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UAggroRadius : public USphereComponent
{
	GENERATED_BODY()

public:
	
	UAggroRadius();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	void Initialize(UThreatHandler* ThreatHandler, const float DefaultRadius);

	EFaction GetOwnerFaction() const { return OwnerFaction; }

private:

	float DefaultAggroRadius = 0.0f;
	EFaction OwnerFaction = EFaction::Neutral;
	UPROPERTY()
	UThreatHandler* ThreatHandlerRef;
	UPROPERTY()
	UNPCAbilityComponent* NPCComponentRef;
	UFUNCTION()
	void OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior, const ENPCCombatBehavior NewBehavior);
	UPROPERTY()
	UStatHandler* StatHandlerRef;
	FStatCallback AggroRadiusCallback;
	UFUNCTION()
	void OnAggroRadiusStatChanged(const FGameplayTag StatTag, const float NewValue);

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};