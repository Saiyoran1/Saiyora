#pragma once
/*#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "DamageStructs.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UCombatGroup;
class ASaiyoraGameState;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

#pragma region Threat
	
	/*
	 *
	 * THREAT
	 *
	 */
	
/*public:
	static FGameplayTag GenericThreatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat")), false); }
	static float GlobalHealingThreatModifier;
	static float GlobalTauntThreatPercentage;
	static float GlobalThreatDecayPercentage;
	static float GlobalThreatDecayInterval;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	UCombatComponent* GetCurrentTarget() const { return CurrentTarget; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	FThreatEvent AddThreat(EThreatType const ThreatType, float const BaseThreat, UCombatComponent* AppliedBy,
			UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier);
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	float GetThreatLevel(UCombatComponent* Target) const;
	void NotifyAddedToThreatTable(UCombatComponent* Target);
	void NotifyRemovedFromThreatTable(UCombatComponent* Target);
	
private:
	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget)
	UCombatComponent* CurrentTarget = nullptr;
    UFUNCTION()
    void OnRep_CurrentTarget(UCombatComponent* PreviousTarget);
	void UpdateTarget();
	UPROPERTY()
	TArray<FThreatTarget> ThreatTable;
	void AddToThreatTable(UCombatComponent* Target, float const InitialThreat = 0.0f, bool const bFaded = false, UBuff* InitialFixate = nullptr, UBuff* InitialBlind = nullptr);
	void RemoveFromThreatTable(UCombatComponent* Target);
	void RemoveThreat(UCombatComponent* Target, float const Amount);
	FTimerDelegate DecayDelegate;
	UFUNCTION()
	void DecayThreat();
	FTimerHandle DecayHandle;
	UPROPERTY()
	TArray<UCombatComponent*> TargetedBy;
	/*UFUNCTION()
	bool CheckBuffForThreat(FBuffApplyEvent const& BuffEvent);
	FBuffEventCondition ThreatBuffRestriction;*/

/*#pragma region Threat Special Events

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Taunt(UCombatComponent* AppliedBy);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void DropThreat(UCombatComponent* Target, float const Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void TransferThreat(UCombatComponent* From, UCombatComponent* To, float const Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Vanish();
	void NotifyOfTargetVanish(UCombatComponent* Target) { RemoveFromThreatTable(Target); }
	void AddFixate(UCombatComponent* Target, UBuff* Source);
    void RemoveFixate(UCombatComponent* Target, UBuff* Source);
    void AddBlind(UCombatComponent* Target, UBuff* Source);
    void RemoveBlind(UCombatComponent* Target, UBuff* Source);
    void AddFade(UBuff* Source);
    void RemoveFade(UBuff* Source);
	bool HasActiveFade() const { return Fades.Num() != 0; }
	void NotifyOfTargetFadeStatusChange(UCombatComponent* Target, bool const bFaded);
    void AddMisdirect(UBuff* Source, UCombatComponent* Target);
    void RemoveMisdirect(UBuff* Source);
	UCombatComponent* GetMisdirectTarget() const { return Misdirects.Num() == 0 ? nullptr : Misdirects[Misdirects.Num() - 1].Target; }
private:
	UPROPERTY()
	TArray<UBuff*> Fades;
	UPROPERTY()
    TArray<FMisdirect> Misdirects;

#pragma endregion
#pragma region Subscriptions

public:
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void SubscribeToTargetChanged(FTargetCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void UnsubscribeFromTargetChanged(FTargetCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void SubscribeToCombatStatusChanged(FCombatStatusCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void UnsubscribeFromCombatStatusChanged(FCombatStatusCallback const& Callback);
private:
	FTargetNotification OnTargetChanged;
	FCombatStatusNotification OnCombatStatusChanged;
	
#pragma endregion
#pragma region Restrictions

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool CanEverReceiveThreat() const { return bCanEverReceiveThreat; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddIncomingThreatRestriction(FThreatRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveIncomingThreatRestriction(FThreatRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddOutgoingThreatRestriction(FThreatRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveOutgoingThreatRestriction(FThreatRestriction const& Restriction);
private:
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanEverReceiveThreat = true;
	TArray<FThreatRestriction> IncomingThreatRestrictions;
	bool CheckIncomingThreatRestricted(FThreatEvent const& ThreatEvent);
	TArray<FThreatRestriction> OutgoingThreatRestrictions;
	bool CheckOutgoingThreatRestricted(FThreatEvent const& ThreatEvent);
	
#pragma endregion 
#pragma region Modifiers

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddIncomingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveIncomingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddOutgoingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveOutgoingThreatModifier(FThreatModCondition const& Modifier);
private:
	TArray<FThreatModCondition> IncomingThreatMods;
	void GetIncomingThreatMods(FThreatEvent const& ThreatEvent, TArray<FCombatModifier>& OutMods);
	TArray<FThreatModCondition> OutgoingThreatMods;
	void GetOutgoingThreatMods(FThreatEvent const& ThreatEvent, TArray<FCombatModifier>& OutMods);
	
#pragma endregion 
	
#pragma endregion
	
};*/