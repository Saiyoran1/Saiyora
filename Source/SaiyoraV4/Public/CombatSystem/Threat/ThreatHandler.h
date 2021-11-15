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
	static float GlobalHealingThreatModifier;
	static float GlobalTauntThreatPercentage;
	static float GlobalThreatDecayPercentage;
	static float GlobalThreatDecayInterval;
	
	UThreatHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool CanEverReceiveThreat() const { return bCanEverReceiveThreat; }
	bool CanEverGenerateThreat() const { return bCanEverGenerateThreat; }
	AActor* GetMisdirectTarget() const { return Misdirects.Num() == 0 ? nullptr : Misdirects[Misdirects.Num() - 1].TargetActor; }
	bool HasActiveFade() const { return Fades.Num() != 0; }

	void GetIncomingThreatMods(TArray<FCombatModifier>& OutMods, FThreatEvent const& Event);
	void GetOutgoingThreatMods(TArray<FCombatModifier>& OutMods, FThreatEvent const& Event);

	bool CheckIncomingThreatRestricted(FThreatEvent const& Event);
	bool CheckOutgoingThreatRestricted(FThreatEvent const& Event);

	void NotifyAddedToThreatTable(AActor* Actor);
	void NotifyRemovedFromThreatTable(AActor* Actor);

	float GetThreatLevel(AActor* Target) const;
	FThreatEvent AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
		UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier);
	void RemoveThreat(float const Amount, AActor* AppliedBy);

	void AddFixate(AActor* Target, UBuff* Source);
	void RemoveFixate(AActor* Target, UBuff* Source);
	void AddBlind(AActor* Target, UBuff* Source);
	void RemoveBlind(AActor* Target, UBuff* Source);
	void AddFade(UBuff* Source);
	void RemoveFade(UBuff* Source);
	void SubscribeToFadeStatusChanged(FFadeCallback const& Callback);
	void UnsubscribeFromFadeStatusChanged(FFadeCallback const& Callback);
	void AddMisdirect(UBuff* Source, AActor* Target);
	void RemoveMisdirect(UBuff* Source);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Taunt(AActor* AppliedBy);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void DropThreat(AActor* Target, float const Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Vanish();
	void SubscribeToVanished(FVanishCallback const& Callback);
	void UnsubscribeFromVanished(FVanishCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void TransferThreat(AActor* FromActor, AActor* ToActor, float const Percentage);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool IsInCombat() const { return bInCombat; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	AActor* GetCurrentTarget() const { return CurrentTarget; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	float GetActorThreatValue(AActor* Actor) const;

private:

	void UpdateCombatStatus();
	UPROPERTY(ReplicatedUsing=OnRep_bInCombat)
	bool bInCombat = false;
	UFUNCTION()
	void OnRep_bInCombat();
	void AddToThreatTable(AActor* Actor, float const InitialThreat = 0.0f, bool const Faded = false, UBuff* InitialFixate = nullptr, UBuff* InitialBlind = nullptr);
	void RemoveFromThreatTable(AActor* Actor);
	UFUNCTION()
	void OnTargetDied(AActor* Actor, ELifeStatus Previous, ELifeStatus New);
	UFUNCTION()
	void OnOwnerDied(AActor* Actor, ELifeStatus Previous, ELifeStatus New);
	UFUNCTION()
	void OnOwnerDamageTaken(FDamagingEvent const& DamageEvent);
	UFUNCTION()
	void OnTargetHealingTaken(FDamagingEvent const& HealingEvent);
	UFUNCTION()
	void OnTargetVanished(AActor* Actor);
	UFUNCTION()
	void DecayThreat();
	FTimerHandle DecayHandle;
	FTimerDelegate DecayDelegate;
	
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
	TArray<UBuff*> Fades;
	UFUNCTION()
	void OnTargetFadeStatusChanged(AActor* Actor, bool const FadeStatus);
	UPROPERTY()
	TArray<FMisdirect> Misdirects;

	UPROPERTY()
	TArray<AActor*> TargetedBy;

	UPROPERTY()
	class UCombatComponent* DamageHandlerRef;
	UPROPERTY()
	class UBuffHandler* BuffHandlerRef;
	UFUNCTION()
	bool CheckBuffForThreat(FBuffApplyEvent const& BuffEvent);
	FBuffEventCondition ThreatBuffRestriction;
	
	TArray<FThreatModCondition> OutgoingThreatMods;
	TArray<FThreatModCondition> IncomingThreatMods;
	TArray<FThreatCondition> OutgoingThreatRestrictions;
	TArray<FThreatCondition> IncomingThreatRestrictions;
	FTargetNotification OnTargetChanged;
	FCombatStatusNotification OnCombatChanged;
	FFadeCallback FadeCallback;
	FFadeNotification OnFadeStatusChanged;
	FVanishCallback VanishCallback;
	FVanishNotification OnVanished;
	FLifeStatusCallback DeathCallback;
	FLifeStatusCallback OwnerDeathCallback;
	FDamageEventCallback ThreatFromDamageCallback;
	FDamageEventCallback ThreatFromHealingCallback;
	FBuffRemoveCallback BuffRemovalCallback;
};