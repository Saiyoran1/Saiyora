// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "GameplayTagContainer.h"
#include "ThreatStructs.h"
#include "Components/ActorComponent.h"
#include "ThreatHandler.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UThreatHandler : public UActorComponent
{
	GENERATED_BODY()

public:
	static FGameplayTag GenericThreatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat")), false); }
	static FGameplayTag FixateTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat.Fixate")), false); }
	static FGameplayTag BlindTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat.Blind")), false); }
	static FGameplayTag FadeTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat.Fade")), false); }
	static FGameplayTag MisdirectTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat.Misdirect")), false); }
	
	UThreatHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool CanEverReceiveThreat() const { return bCanEverReceiveThreat; }
	bool CanEverGenerateThreat() const { return bCanEverGenerateThreat; }
	//TODO: Get current misdirect target.
	AActor* GetMisdirectTarget() const { return nullptr; }
	bool HasActiveFade() const { return Fades.Num() != 0; }

	void GetIncomingThreatMods(TArray<FCombatModifier>& OutMods, FThreatEvent const& Event);
	void GetOutgoingThreatMods(TArray<FCombatModifier>& OutMods, FThreatEvent const& Event);

	bool CheckIncomingThreatRestricted(FThreatEvent const& Event);
	bool CheckOutgoingThreatRestricted(FThreatEvent const& Event);

	void AddThreat(FThreatEvent& Event);

	void AddFixate(AActor* Target, UBuff* Source);
	//void AddBlind(AActor* Target, UBuff* Source);
	//void AddMisdirect(AActor* ThreatFrom, AActor* ThreatTo, UBuff* Source);

private:
	UPROPERTY()
	class UBuffHandler* BuffHandlerRef;

	void EnterCombat();
	void LeaveCombat();
	UPROPERTY(ReplicatedUsing=OnRep_bInCombat)
	bool bInCombat = false;
	UFUNCTION()
	void OnRep_CombatStatus();
	
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanEverReceiveThreat = false;
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanEverGenerateThreat = false;
	
	UPROPERTY()
	TArray<FThreatTarget> ThreatTable;
	void UpdateTarget();
	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget)
	AActor* CurrentTarget = nullptr;
	UFUNCTION()
	void OnRep_CurrentTarget(AActor* PreviousTarget);
	UPROPERTY()
	TMap<UBuff*, AActor*> Fixates;
	UFUNCTION()
	void RemoveThreatControlsOnBuffRemoved(FBuffRemoveEvent const& RemoveEvent);
	UPROPERTY()
	TMap<UBuff*, AActor*> Blinds;
	UPROPERTY()
	TArray<UBuff*> Fades;
	UPROPERTY()
	TMap<UBuff*, AActor*> Misdirects;
	
	TArray<FThreatModCondition> OutgoingThreatMods;
	TArray<FThreatModCondition> IncomingThreatMods;
	TArray<FThreatCondition> OutgoingThreatRestrictions;
	TArray<FThreatCondition> IncomingThreatRestrictions;
	FTargetNotification OnTargetChanged;
	FCombatStatusNotification OnCombatChanged;
	FCombatStatusNotification OnFadeStatusChanged;

	FBuffRemoveCallback BuffRemovalCallback;
};