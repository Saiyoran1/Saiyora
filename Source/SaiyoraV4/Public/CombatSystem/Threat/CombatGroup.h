#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "DamageStructs.h"
#include "Object.h"
#include "SaiyoraEnums.h"
#include "CombatGroup.generated.h"

struct FDamagingEvent;
class UCombatComponent;
class ASaiyoraGameState;

UCLASS()
class SAIYORAV4_API UCombatGroup : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;
	bool bInitialized = false;

	UPROPERTY()
	TArray<UCombatComponent*> CombatActors;
	float CombatStartTime = 0.0f;
	void AddNewCombatant(UCombatComponent* Combatant);
	void RemoveCombatant(UCombatComponent* Combatant);
	FDamageEventCallback DamageTakenCallback;
	UFUNCTION()
	void OnCombatantDamageTaken(FDamagingEvent const& DamageEvent);
	FDamageEventCallback DamageDoneCallback;
	UFUNCTION()
	void OnCombatantDamageDone(FDamagingEvent const& DamageEvent);
	FDamageEventCallback HealingTakenCallback;
	UFUNCTION()
	void OnCombatantHealingTaken(FDamagingEvent const& HealingEvent);
	FDamageEventCallback HealingDoneCallback;
	UFUNCTION()
	void OnCombatantHealingDone(FDamagingEvent const& HealingEvent);
	FLifeStatusCallback LifeStatusCallback;
	UFUNCTION()
	void OnCombatantLifeStatusChanged(AActor* Actor, ELifeStatus const Previous, ELifeStatus const New);
	//TODO: OnCombatantFactionSwap
	
public:
	void Initialize(FDamagingEvent const& DamageEvent);
	void Initialize(FThreatEvent const& ThreatEvent);
	void Initialize(UCombatComponent* Instigator, UCombatComponent* Target);

	//TODO: Combat logging could take place here?
};
