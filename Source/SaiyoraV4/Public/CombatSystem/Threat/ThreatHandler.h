// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "GameplayTagContainer.h"
#include "ThreatStructs.h"
#include "DamageStructs.h"
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
	static float GlobalHealingThreatModifier;
	
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

	void NotifyAddedToThreatTable(AActor* Actor);
	void NotifyRemovedFromThreatTable(AActor* Actor);

	FThreatEvent AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
		UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier);

	void AddFixate(AActor* Target, UBuff* Source);
	void RemoveFixate(UBuff* Source);
	void AddBlind(AActor* Target, UBuff* Source);
	void RemoveBlind(UBuff* Source);
	void AddFade(UBuff* Source);
	void RemoveFade(UBuff* Source);
	void SubscribeToFadeStatusChanged(FFadeCallback const& Callback);
	void UnsubscribeFromFadeStatusChanged(FFadeCallback const& Callback);
	//void AddMisdirect(AActor* ThreatFrom, AActor* ThreatTo, UBuff* Source);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool IsInCombat() const { return bInCombat; }

private:

	void UpdateCombatStatus();
	UPROPERTY(ReplicatedUsing=OnRep_bInCombat)
	bool bInCombat = false;
	UFUNCTION()
	void OnRep_bInCombat();
	void AddToThreatTable(AActor* Actor, float const InitialThreat = 0.0f, int32 const InitialFixates = 0, int32 const InitialBlinds = 0);
	void RemoveFromThreatTable(AActor* Actor);
	UFUNCTION()
	void OnTargetDied(AActor* Actor, ELifeStatus Previous, ELifeStatus New);
	UFUNCTION()
	void OnOwnerDied(AActor* Actor, ELifeStatus Previous, ELifeStatus New);
	UFUNCTION()
	void OnOwnerDamageTaken(FDamagingEvent const& DamageEvent);
	UFUNCTION()
	void OnTargetHealingTaken(FHealingEvent const& HealingEvent);
	
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
	UPROPERTY()
	TMap<UBuff*, AActor*> Blinds;
	UPROPERTY()
	TArray<UBuff*> Fades;
	UFUNCTION()
	void OnTargetFadeStatusChanged(AActor* Actor, bool const FadeStatus);
	UPROPERTY()
	TMap<UBuff*, AActor*> Misdirects;

	UPROPERTY()
	TArray<AActor*> TargetedBy;

	UPROPERTY()
	class UDamageHandler* DamageHandlerRef;
	
	TArray<FThreatModCondition> OutgoingThreatMods;
	TArray<FThreatModCondition> IncomingThreatMods;
	TArray<FThreatCondition> OutgoingThreatRestrictions;
	TArray<FThreatCondition> IncomingThreatRestrictions;
	FTargetNotification OnTargetChanged;
	FCombatStatusNotification OnCombatChanged;
	FFadeCallback FadeCallback;
	FFadeNotification OnFadeStatusChanged;
	FLifeStatusCallback DeathCallback;
	FLifeStatusCallback OwnerDeathCallback;
	FDamageEventCallback ThreatFromDamageCallback;
	FHealingEventCallback ThreatFromHealingCallback;
	FBuffRemoveCallback BuffRemovalCallback;
};