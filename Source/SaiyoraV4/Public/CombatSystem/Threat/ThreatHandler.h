#pragma once
#include "CoreMinimal.h"
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
	
	static float GLOBALHEALINGTHREATMOD;
	static float GLOBALTAUNTTHREATPERCENT;
	
	UThreatHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY()
	UDamageHandler* DamageHandlerRef = nullptr;
	UPROPERTY()
	UPlaneComponent* PlaneCompRef = nullptr;
	UPROPERTY()
	UFactionComponent* FactionCompRef = nullptr;

//Threat

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat", meta = (AutoCreateRefTerm = "SourceModifier"))
	FThreatEvent AddThreat(const EThreatType ThreatType, const float BaseThreat, AActor* AppliedBy,
		UObject* Source, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const FThreatModCondition& SourceModifier);
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInCombat() const { return bInCombat; }
	UFUNCTION(BlueprintPure, Category = "Threat")
	bool HasThreatTable() const { return bHasThreatTable; }
	UFUNCTION(BlueprintPure, Category = "Threat")
	bool CanBeInThreatTable() const { return bCanBeInThreatTable; }
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	bool IsActorInThreatTable(const AActor* Target) const;
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	float GetActorThreatValue(const AActor* Actor) const;
	UFUNCTION(BlueprintPure, Category = "Threat")
	AActor* GetCurrentTarget() const { return CurrentTarget; }
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	void GetActorsInThreatTable(TArray<AActor*>& OutActors) const;
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	void GetActorsTargetingThis(TArray<AActor*>& OutActors) const { OutActors = TargetedBy; }

	UPROPERTY(BlueprintAssignable)
	FTargetNotification OnTargetChanged;
	UPROPERTY(BlueprintAssignable)
	FCombatStatusNotification OnCombatChanged;

	void AddIncomingThreatRestriction(UBuff* Source, const FThreatRestriction& Restriction);
	void RemoveIncomingThreatRestriction(const UBuff* Source);
	bool CheckIncomingThreatRestricted(const FThreatEvent& Event);
	void AddOutgoingThreatRestriction(UBuff* Source, const FThreatRestriction& Restriction);
	void RemoveOutgoingThreatRestriction(const UBuff* Source);
	bool CheckOutgoingThreatRestricted(const FThreatEvent& Event);
	void AddIncomingThreatModifier(UBuff* Source, const FThreatModCondition& Modifier);
	void RemoveIncomingThreatModifier(const UBuff* Source);
	float GetModifiedIncomingThreat(const FThreatEvent& ThreatEvent) const;
	void AddOutgoingThreatModifier(UBuff* Source, const FThreatModCondition& Modifier);
	void RemoveOutgoingThreatModifier(const UBuff* Source);
	float GetModifiedOutgoingThreat(const FThreatEvent& ThreatEvent, const FThreatModCondition& SourceModifier) const;

	void NotifyOfTargeting(const UThreatHandler* TargetingComponent);
	void NotifyOfTargetingEnded(const UThreatHandler* TargetingComponent);

private:

	UPROPERTY(ReplicatedUsing = OnRep_bInCombat)
	bool bInCombat = false;
	UFUNCTION()
	void OnRep_bInCombat();
	void UpdateCombat();
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bHasThreatTable = false;
	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bCanBeInThreatTable = false;
	UPROPERTY()
	TArray<FThreatTarget> ThreatTable;
	UPROPERTY()
	TArray<AActor*> TargetedBy;
	void AddToThreatTable(const FThreatTarget& NewTarget);
	void RemoveFromThreatTable(const AActor* Actor);
	void ClearThreatTable();
	void RemoveThreat(const float Amount, const AActor* AppliedBy);
	UPROPERTY()
	TMap<UBuff*, FThreatModCondition> OutgoingThreatMods;
	UPROPERTY()
	TMap<UBuff*, FThreatModCondition> IncomingThreatMods;
	UPROPERTY()
	TMap<UBuff*, FThreatRestriction> OutgoingThreatRestrictions;
	UPROPERTY()
	TMap<UBuff*, FThreatRestriction> IncomingThreatRestrictions;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentTarget)
	AActor* CurrentTarget = nullptr;
	UFUNCTION()
	void OnRep_CurrentTarget(AActor* PreviousTarget);
	void UpdateTarget();
	UFUNCTION()
	void OnOwnerDamageTaken(const FHealthEvent& DamageEvent);
	UFUNCTION()
	void OnTargetHealingTaken(const FHealthEvent& HealthEvent);
	UFUNCTION()
	void OnTargetHealingDone(const FHealthEvent& HealthEvent);
	UFUNCTION()
	void OnTargetLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	UFUNCTION()
	void OnOwnerLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	
//Threat Actions

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Taunt(AActor* AppliedBy);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void DropThreat(AActor* Target, const float Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void TransferThreat(AActor* FromActor, AActor* ToActor, const float Percentage);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Vanish();
	void NotifyOfTargetVanished(AActor* Target) { RemoveFromThreatTable(Target); }

	void AddFixate(AActor* Target, UBuff* Source);
	void RemoveFixate(const AActor* Target, UBuff* Source);
	
	void AddBlind(AActor* Target, UBuff* Source);
	void RemoveBlind(const AActor* Target, UBuff* Source);
	
	void AddFade(UBuff* Source);
	void RemoveFade(UBuff* Source);
	bool HasActiveFade() const { return Fades.Num() != 0; }
	void NotifyOfTargetFadeStatusUpdate(const AActor* Target, const bool FadeStatus);
	
	void AddMisdirect(UBuff* Source, AActor* Target);
	void RemoveMisdirect(const UBuff* Source);
	AActor* GetMisdirectTarget() const { return Misdirects.Num() == 0 ? nullptr : Misdirects[Misdirects.Num() - 1].Target; }

private:
	
	UPROPERTY()
	TArray<UBuff*> Fades;
	UPROPERTY()
	TArray<FMisdirect> Misdirects;
};