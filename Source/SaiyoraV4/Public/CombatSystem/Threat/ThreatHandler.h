#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "GameplayTagContainer.h"
#include "ThreatStructs.h"
#include "DamageStructs.h"
#include "Components/ActorComponent.h"
#include "ThreatHandler.generated.h"

class UCombatGroup;
class UDamageHandler;
class UBuffHandler;
class UPlaneComponent;
class UFactionComponent;

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
	UDamageHandler* DamageHandlerRef;
	UPROPERTY()
	UBuffHandler* BuffHandlerRef;
	UPROPERTY()
	UPlaneComponent* PlaneCompRef;
	UPROPERTY()
	UFactionComponent* FactionCompRef;

//Combat

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	bool IsInCombat() const { return bInCombat; }
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Combat")
	UCombatGroup* GetCombatGroup() const { return CombatGroup; }
	void NotifyOfCombatJoined(UCombatGroup* NewGroup);
	void NotifyOfCombatLeft(UCombatGroup* LeftGroup);
	void NotifyOfCombatGroupMerge(UCombatGroup* OldGroup, UCombatGroup* NewGroup);
	void NotifyOfNewCombatant(AActor* NewCombatant) { AddToThreatTable(NewCombatant); }
	void NotifyOfRemovedCombatant(AActor* RemovedCombatant) { RemoveFromThreatTable(RemovedCombatant); }

private:

	UPROPERTY(ReplicatedUsing=OnRep_bInCombat)
	bool bInCombat = false;
	UFUNCTION()
	void OnRep_bInCombat();
	FCombatStatusNotification OnCombatChanged;
	UPROPERTY()
	UCombatGroup* CombatGroup;
	void StartNewCombat(UThreatHandler* TargetThreatHandler);

//Threat

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool HasThreatTable() const { return bHasThreatTable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool CanBeInThreatTable() const { return bCanBeInThreatTable; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat", meta = (AutoCreateRefTerm = "SourceModifier"))
	FThreatEvent AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
		UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier);
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	bool IsActorInThreatTable(AActor* Target) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
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
	float GetModifiedIncomingThreat(FThreatEvent const& ThreatEvent) const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddOutgoingThreatModifier(FThreatModCondition const& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void RemoveOutgoingThreatModifier(FThreatModCondition const& Modifier);
	float GetModifiedOutgoingThreat(FThreatEvent const& ThreatEvent, FThreatModCondition const& SourceModifier) const;

private:

	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bHasThreatTable = false;
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanBeInThreatTable = false;
	UPROPERTY()
	TArray<FThreatTarget> ThreatTable;
	void AddToThreatTable(AActor* Actor, float const InitialThreat = 0.0f, UBuff* InitialFixate = nullptr, UBuff* InitialBlind = nullptr);
	void RemoveFromThreatTable(AActor* Actor);
	void ClearThreatTable();
	void RemoveThreat(float const Amount, AActor* AppliedBy);
	TArray<FThreatModCondition> OutgoingThreatMods;
	TArray<FThreatModCondition> IncomingThreatMods;
	TArray<FThreatRestriction> OutgoingThreatRestrictions;
	TArray<FThreatRestriction> IncomingThreatRestrictions;
	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget)
	AActor* CurrentTarget = nullptr;
	UFUNCTION()
	void OnRep_CurrentTarget(AActor* PreviousTarget);
	void UpdateTarget();
	FTargetNotification OnTargetChanged;
	FDamageEventCallback ThreatFromDamageCallback;
	UFUNCTION()
	void OnOwnerDamageTaken(FDamagingEvent const& DamageEvent);
	FDamageEventCallback ThreatFromHealingCallback;
	UFUNCTION()
	void OnTargetHealingTaken(FDamagingEvent const& HealingEvent);
	FTimerHandle DecayHandle;
	FTimerDelegate DecayDelegate;
	UFUNCTION()
	void DecayThreat();
	
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

private:
	
	UPROPERTY()
	TArray<UBuff*> Fades;
	FFadeCallback FadeCallback;
	UFUNCTION()
	void OnTargetFadeStatusChanged(AActor* Actor, bool const FadeStatus);
	FFadeNotification OnFadeStatusChanged;
	UPROPERTY()
	TArray<FMisdirect> Misdirects;

	FBuffEventCondition ThreatBuffRestriction;
	UFUNCTION()
	bool CheckBuffForThreat(FBuffApplyEvent const& BuffEvent);
};