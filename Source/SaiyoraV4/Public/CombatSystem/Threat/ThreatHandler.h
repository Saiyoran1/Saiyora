// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ThreatStructs.h"
#include "Components/ActorComponent.h"
#include "ThreatHandler.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UThreatHandler : public UActorComponent
{
	GENERATED_BODY()

public:
	UThreatHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool CanEverReceiveThreat() const { return bCanEverReceiveThreat; }
	bool CanEverGenerateThreat() const { return bCanEverGenerateThreat; }
	//TODO: Get current misdirect target.
	AActor* GetMisdirectTarget() const { return nullptr; }
	//TODO: Check if this actor is currently not preferred to enemies.
	bool HasActiveFade() const { return Fades.Num() != 0; }

	void GetIncomingThreatMods(TArray<FCombatModifier>& OutMods, FThreatEvent const& Event);
	void GetOutgoingThreatMods(TArray<FCombatModifier>& OutMods, FThreatEvent const& Event);

	bool CheckIncomingThreatRestricted(FThreatEvent const& Event);
	bool CheckOutgoingThreatRestricted(FThreatEvent const& Event);

	void AddThreat(FThreatEvent& Event);

	//void AddFixate(AActor* Target, UBuff* Source);
	//void AddBlind(AActor* Target, UBuff* Source);
	//void AddMisdirect(AActor* ThreatFrom, AActor* ThreatTo, UBuff* Source);

private:

	void EnterCombat();
	void LeaveCombat();
	bool bInCombat = false;
	
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanEverReceiveThreat = false;
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanEverGenerateThreat = false;
	
	UPROPERTY()
	TArray<FThreatTarget> ThreatTable;
	void UpdateTarget();
	UPROPERTY(Replicated)
	AActor* CurrentTarget = nullptr;
	UPROPERTY()
	TArray<FFixate> Fixates;
	UPROPERTY()
	TArray<FBlind> Blinds;
	UPROPERTY()
	TArray<UBuff*> Fades;
	TArray<FThreatModCondition> OutgoingThreatMods;
	TArray<FThreatModCondition> IncomingThreatMods;
	TArray<FThreatCondition> OutgoingThreatRestrictions;
	TArray<FThreatCondition> IncomingThreatRestrictions;
	FTargetNotification OnTargetChanged;
	FCombatStatusNotification OnCombatChanged;
};
