#pragma once
#include "CoreMinimal.h"
#include "ThreatStructs.h"
#include "DamageStructs.h"
#include "NPCEnums.h"
#include "Components/ActorComponent.h"
#include "ThreatHandler.generated.h"

class UDamageHandler;
class UBuffHandler;
class UCombatStatusComponent;
class UNPCAbilityComponent;
class UAggroRadius;
class UCombatGroup;

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
	UCombatStatusComponent* CombatStatusComponentRef = nullptr;
	UPROPERTY()
	UNPCAbilityComponent* NPCComponentRef = nullptr;

	UFUNCTION()
	void OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior, const ENPCCombatBehavior NewBehavior);
	FThreatRestriction DisableThreatEvents;
	UFUNCTION()
	bool DisableAllThreatEvents(const FThreatEvent& Event) { return true; }

//Threat

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat", meta = (AutoCreateRefTerm = "SourceModifier"))
	FThreatEvent AddThreat(const EThreatType ThreatType, const float BaseThreat, AActor* AppliedBy,
		UObject* Source, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const FThreatModCondition& SourceModifier);
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInCombat() const { return bInCombat; }
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetCombatTime() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetCombatStartTime() const { return CombatStartTime; }
	UFUNCTION(BlueprintPure, Category = "Threat")
	bool HasThreatTable() const { return bHasThreatTable; }
	UFUNCTION(BlueprintPure, Category = "Threat")
	bool CanBeInThreatTable() const { return bCanBeInThreatTable; }
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	bool IsActorInThreatTable(const AActor* Target) const;
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	float GetActorThreatValue(const AActor* Target) const;
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

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void AddIncomingThreatRestriction(const FThreatRestriction& Restriction) { IncomingThreatRestrictions.Add(Restriction); }
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void RemoveIncomingThreatRestriction(const FThreatRestriction& Restriction) { IncomingThreatRestrictions.Remove(Restriction); }

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void AddOutgoingThreatRestriction(const FThreatRestriction& Restriction) { OutgoingThreatRestrictions.Add(Restriction); }
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void RemoveOutgoingThreatRestriction(const FThreatRestriction& Restriction) { OutgoingThreatRestrictions.Remove(Restriction); }
	bool CheckOutgoingThreatRestricted(const FThreatEvent& Event) { return OutgoingThreatRestrictions.IsRestricted(Event); }

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void AddIncomingThreatModifier(const FThreatModCondition& Modifier) { IncomingThreatMods.Add(Modifier); }
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void RemoveIncomingThreatModifier(const FThreatModCondition& Modifier) { IncomingThreatMods.Remove(Modifier); }
	float GetModifiedIncomingThreat(const FThreatEvent& ThreatEvent) const;

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void AddOutgoingThreatModifier(const FThreatModCondition& Modifier) { OutgoingThreatMods.Add(Modifier); }
	UFUNCTION(BlueprintCallable, Category = "Threat")
	void RemoveOutgoingThreatModifier(const FThreatModCondition& Modifier) { OutgoingThreatMods.Remove(Modifier); }
	float GetModifiedOutgoingThreat(const FThreatEvent& ThreatEvent, const FThreatModCondition& SourceModifier) const;

private:

	UPROPERTY(ReplicatedUsing = OnRep_bInCombat)
	bool bInCombat = false;
	UPROPERTY(Replicated)
	float CombatStartTime = 0.0f;
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
	
	TConditionalModifierList<FThreatModCondition> OutgoingThreatMods;
	TConditionalModifierList<FThreatModCondition> IncomingThreatMods;
	TConditionalRestrictionList<FThreatRestriction> OutgoingThreatRestrictions;
	TConditionalRestrictionList<FThreatRestriction> IncomingThreatRestrictions;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentTarget)
	AActor* CurrentTarget = nullptr;
	UFUNCTION()
	void OnRep_CurrentTarget(AActor* PreviousTarget);
	void UpdateTarget();
	UFUNCTION()
	void OnOwnerDamageTaken(const FHealthEvent& DamageEvent);
	
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

	//Detection

private:

	UPROPERTY(EditAnywhere, Category = "Threat")
	bool bProximityAggro = false;
	UPROPERTY(EditAnywhere, Category = "Threat")
	float DefaultDetectionRadius = 0.0f;

	UPROPERTY()
	UAggroRadius* AggroRadius;

	//Combat Group

public:

	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Threat")
	UCombatGroup* GetCombatGroup() const { return CombatGroup; }

	void NotifyOfCombat(UCombatGroup* Group);
	void NotifyOfNewCombatant(UThreatHandler* Combatant) { AddToThreatTable(FThreatTarget(Combatant)); }
	void NotifyOfCombatantLeft(const UThreatHandler* Combatant) { RemoveFromThreatTable(Combatant->GetOwner()); }

private:
	
	int32 FindOrAddToThreatTable(UThreatHandler* Target, bool& bAdded);
	int32 FindInThreatTable(const UThreatHandler* Target) const;
	UPROPERTY()
	UCombatGroup* CombatGroup;
};