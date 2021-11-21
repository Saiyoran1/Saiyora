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
	static FGameplayTag BlingTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat.Blind")), false); }
	static FGameplayTag FadeTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat.Fade")), false); }
	static FGameplayTag MisdirectTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Threat.Misdirect")), false); }
	static float GlobalHealingThreatModifier;
	static float GlobalTauntThreatPercentage;
	static float GlobalThreatDecayPercentage;
	static float GlobalThreatDecayInterval;
	
	UThreatHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY()
	class UDamageHandler* DamageHandlerRef;
	UPROPERTY()
	class UBuffHandler* BuffHandlerRef;

//Threat

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool CanEverReceiveThreat() const { return bCanEverReceiveThreat; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool CanEverGenerateThreat() const { return bCanEverGenerateThreat; }
	/*UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool IsInCombat() const { return bInCombat; }*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat", meta = (AutoCreateRefTerm = "SourceModifier"))
	FThreatEvent AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
		UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	float GetActorThreatValue(AActor* Actor) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void SubscribeToTargetChanged(FTargetCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void UnsubscribeFromTargetChanged(FTargetCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddIncomingThreatRestriction(FThreatRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveIncomingThreatRestriction(FThreatRestriction const& Restriction);
	bool CheckIncomingThreatRestricted(FThreatEvent const& Event);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddOutgoingThreatRestriction(FThreatRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveOutgoingThreatRestriction(FThreatRestriction const& Restriction);
	bool CheckOutgoingThreatRestricted(FThreatEvent const& Event);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddIncomingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveIncomingThreatModifier(FThreatModCondition const& Modifier);
	float GetModifiedIncomingThreat(FThreatEvent const& ThreatEvent, FThreatModCondition const& SourceModifier) const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddOutgoingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveOutgoingThreatModifier(FThreatModCondition const& Modifier);
	float GetModifiedOutgoingThreat(FThreatEvent const& ThreatEvent) const;

private:

//Threat Actions

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Taunt(AActor* AppliedBy);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void DropThreat(AActor* Target, float const Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Vanish();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void TransferThreat(AActor* FromActor, AActor* ToActor, float const Percentage);
	
	AActor* GetMisdirectTarget() const { return Misdirects.Num() == 0 ? nullptr : Misdirects[Misdirects.Num() - 1].Target; }
	bool HasActiveFade() const { return Fades.Num() != 0; }

	void NotifyAddedToThreatTable(AActor* Actor);
	void NotifyRemovedFromThreatTable(AActor* Actor);
	
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
	
	void SubscribeToVanished(FVanishCallback const& Callback);
	void UnsubscribeFromVanished(FVanishCallback const& Callback);
	

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

	UFUNCTION()
	bool CheckBuffForThreat(FBuffApplyEvent const& BuffEvent);
	FBuffEventCondition ThreatBuffRestriction;
	
	TArray<FThreatModCondition> OutgoingThreatMods;
	TArray<FThreatModCondition> IncomingThreatMods;
	TArray<FThreatRestriction> OutgoingThreatRestrictions;
	TArray<FThreatRestriction> IncomingThreatRestrictions;
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