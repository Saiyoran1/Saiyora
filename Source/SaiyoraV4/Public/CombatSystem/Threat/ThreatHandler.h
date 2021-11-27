#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "GameplayTagContainer.h"
#include "ThreatStructs.h"
#include "DamageStructs.h"
#include "Components/ActorComponent.h"
#include "ThreatHandler.generated.h"

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

//Threat

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat", meta = (AutoCreateRefTerm = "SourceModifier"))
	FThreatEvent AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
		UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	bool IsInCombat() const { return bInCombat; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool HasThreatTable() const { return bHasThreatTable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	bool CanBeInThreatTable() const { return bCanBeInThreatTable; }
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	bool IsActorInThreatTable(AActor* Target) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	float GetActorThreatValue(AActor* Actor) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Threat")
	AActor* GetCurrentTarget() const { return CurrentTarget; }
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	void GetActorsInThreatTable(TArray<AActor*>& OutActors) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	void GetActorsTargetingThis(TArray<AActor*>& OutActors) const { OutActors = TargetedBy; }

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void SubscribeToTargetChanged(FTargetCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void UnsubscribeFromTargetChanged(FTargetCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void SubscribeToCombatStatusChanged(FCombatStatusCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void UnsubscribeFromCombatStatusChanged(FCombatStatusCallback const& Callback);

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

	void NotifyOfTargeting(UThreatHandler* TargetingComponent);
	void NotifyOfTargetingEnded(UThreatHandler* TargetingComponent);

private:

	UPROPERTY(ReplicatedUsing=OnRep_bInCombat)
	bool bInCombat = false;
	UFUNCTION()
	void OnRep_bInCombat();
	void UpdateCombat();
	FCombatStatusNotification OnCombatChanged;
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bHasThreatTable = false;
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanBeInThreatTable = false;
	UPROPERTY()
	TArray<FThreatTarget> ThreatTable;
	UPROPERTY()
	TArray<AActor*> TargetedBy;
	void AddToThreatTable(FThreatTarget const& NewTarget);
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
	FDamageEventCallback ThreatFromIncomingHealingCallback;
	UFUNCTION()
	void OnTargetHealingTaken(FDamagingEvent const& HealingEvent);
	FDamageEventCallback ThreatFromOutgoingHealingCallback;
	UFUNCTION()
	void OnTargetHealingDone(FDamagingEvent const& HealingEvent);
	FLifeStatusCallback TargetLifeStatusCallback;
	UFUNCTION()
	void OnTargetLifeStatusChanged(AActor* Actor, ELifeStatus const PreviousStatus, ELifeStatus const NewStatus);
	FLifeStatusCallback OwnerLifeStatusCallback;
	UFUNCTION()
	void OnOwnerLifeStatusChanged(AActor* Actor, ELifeStatus const PreviousStatus, ELifeStatus const NewStatus);
	FBuffEventCondition ThreatBuffRestriction;
	UFUNCTION()
	bool CheckBuffForThreat(FBuffApplyEvent const& BuffEvent);
	
//Threat Actions

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Taunt(AActor* AppliedBy);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void DropThreat(AActor* Target, float const Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void TransferThreat(AActor* FromActor, AActor* ToActor, float const Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Vanish();
	void NotifyOfTargetVanished(AActor* Target) { RemoveFromThreatTable(Target); }

	void AddFixate(AActor* Target, UBuff* Source);
	void RemoveFixate(AActor* Target, UBuff* Source);
	
	void AddBlind(AActor* Target, UBuff* Source);
	void RemoveBlind(AActor* Target, UBuff* Source);
	
	void AddFade(UBuff* Source);
	void RemoveFade(UBuff* Source);
	bool HasActiveFade() const { return Fades.Num() != 0; }
	void NotifyOfTargetFadeStatusUpdate(AActor* Target, bool const FadeStatus);
	
	void AddMisdirect(UBuff* Source, AActor* Target);
	void RemoveMisdirect(UBuff* Source);
	AActor* GetMisdirectTarget() const { return Misdirects.Num() == 0 ? nullptr : Misdirects[Misdirects.Num() - 1].Target; }

private:
	
	UPROPERTY()
	TArray<UBuff*> Fades;
	UPROPERTY()
	TArray<FMisdirect> Misdirects;
};