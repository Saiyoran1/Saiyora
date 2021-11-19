#include "CombatGroup.h"
#include "SaiyoraGameState.h"
#include "MegaComponent/CombatComponent.h"

void UCombatGroup::Initialize(UCombatComponent* Instigator, UCombatComponent* Target)
{
	if (bInitialized || !IsValid(Instigator) || !IsValid(Target))
	{
		MarkPendingKill();
		return;
	}
	//TODO: Check actors are different factions.
	bInitialized = true;
	GameStateRef = Instigator->GetGameStateRef();
	if (!IsValid(GameStateRef))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Game State Ref in Combat Group!"));
		MarkPendingKill();
		return;
	}
	CombatStartTime = GameStateRef->GetServerWorldTimeSeconds();
	DamageDoneCallback.BindDynamic(this, &UCombatGroup::OnCombatantDamageDone);
	DamageTakenCallback.BindDynamic(this, &UCombatGroup::OnCombatantDamageTaken);
	HealingDoneCallback.BindDynamic(this, &UCombatGroup::OnCombatantHealingDone);
	HealingTakenCallback.BindDynamic(this, &UCombatGroup::OnCombatantHealingTaken);
	LifeStatusCallback.BindDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
	AddNewCombatant(Instigator);
	AddNewCombatant(Target);
}

void UCombatGroup::AddNewCombatant(UCombatComponent* Combatant)
{
	if (!IsValid(Combatant) || Combatant->IsInCombat() || CombatActors.Contains(Combatant))
	{
		return;
	}
	CombatActors.Add(Combatant);
	Combatant->SubscribeToOutgoingDamageSuccess(DamageDoneCallback);
	Combatant->SubscribeToIncomingDamageSuccess(DamageTakenCallback);
	Combatant->SubscribeToOutgoingHealingSuccess(HealingDoneCallback);
	Combatant->SubscribeToIncomingHealingSuccess(HealingTakenCallback);
	Combatant->SubscribeToLifeStatusChanged(LifeStatusCallback);
	Combatant->NotifyOfCombatJoined(this);
	for (UCombatComponent* Component : CombatActors)
	{
		if (IsValid(Component) && Component != Combatant)
		{
			Component->NotifyOfNewCombatant(Combatant);
		}
	}
}

void UCombatGroup::RemoveCombatant(UCombatComponent* Combatant)
{
	if (!IsValid(Combatant) || !Combatant->IsInCombat() || !CombatActors.Contains(Combatant))
	{
		return;
	}
	CombatActors.Remove(Combatant);
	Combatant->UnsubscribeFromOutgoingDamageSuccess(DamageDoneCallback);
	Combatant->UnsubscribeFromIncomingDamageSuccess(DamageTakenCallback);
	Combatant->UnsubscribeFromOutgoingHealingSuccess(HealingDoneCallback);
	Combatant->UnsubscribeFromIncomingHealingSuccess(HealingTakenCallback);
	Combatant->UnsubscribeFromLifeStatusChanged(LifeStatusCallback);
	Combatant->NotifyOfCombatLeft(this);
	for (UCombatComponent* Component : CombatActors)
	{
		if (IsValid(Component) && Component != Combatant)
		{
			Component->NotifyOfCombatantRemoved(Combatant);
		}
	}
	if (CombatActors.Num() == 0)
	{
		//TODO: Logging? Cleanup?
		MarkPendingKill();
	}
	else if (CombatActors.Num() == 1)
	{
		RemoveCombatant(CombatActors[0]);
	}
}

void UCombatGroup::LogDamageEvent(FDamagingEvent const& DamageEvent, bool const bHealing)
{
	FDamageLogEvent LogEvent;
	LogEvent.TimeStamp = GameStateRef->GetServerWorldTimeSeconds();
	LogEvent.bWasHealing = bHealing;
	//TODO: Combat name.
	LogEvent.Instigator = DamageEvent.Info.AppliedBy->GetName();
	LogEvent.Target = DamageEvent.Info.AppliedTo->GetName();
	LogEvent.Source = IsValid(DamageEvent.Info.Source) ? DamageEvent.Info.Source->GetName() : FString(TEXT("None"));
	LogEvent.Value = DamageEvent.Result.AmountDealt;
	LogEvent.HitStyle = DamageEvent.Info.HitStyle;
	LogEvent.School = DamageEvent.Info.School;
	DamageLog.Add(LogEvent);
}

void UCombatGroup::OnCombatantDamageTaken(FDamagingEvent const& DamageEvent)
{
	if (IsValid(DamageEvent.Info.AppliedBy) && !CombatActors.Contains(DamageEvent.Info.AppliedBy))
	{
		AddNewCombatant(DamageEvent.Info.AppliedBy);
	}
	LogDamageEvent(DamageEvent);
}

void UCombatGroup::OnCombatantDamageDone(FDamagingEvent const& DamageEvent)
{
	if (IsValid(DamageEvent.Info.AppliedTo) && !CombatActors.Contains(DamageEvent.Info.AppliedTo))
	{
		AddNewCombatant(DamageEvent.Info.AppliedBy);
		//We only log if adding a new combatant. If this combatant is already in our table, then we will log on received damage.
		LogDamageEvent(DamageEvent);
	}
}

void UCombatGroup::OnCombatantHealingTaken(FDamagingEvent const& HealingEvent)
{
	if (IsValid(HealingEvent.Info.AppliedBy) && !CombatActors.Contains(HealingEvent.Info.AppliedBy))
	{
		AddNewCombatant(HealingEvent.Info.AppliedBy);
	}
	LogDamageEvent(HealingEvent, true);
	for (UCombatComponent* Component : CombatActors)
	{
		if (IsValid(Component) && Component->CanEverReceiveThreat()) //And component faction is hostile to healer's faction
		{
			Component->AddThreat(EThreatType::Healing, HealingEvent.ThreatInfo.BaseThreat, HealingEvent.Info.AppliedBy,
				HealingEvent.Info.Source, HealingEvent.ThreatInfo.IgnoreRestrictions, HealingEvent.ThreatInfo.IgnoreModifiers,
				HealingEvent.ThreatInfo.SourceModifier);
		}
	}
}

void UCombatGroup::OnCombatantHealingDone(FDamagingEvent const& HealingEvent)
{
	if (IsValid(HealingEvent.Info.AppliedTo) && !CombatActors.Contains(HealingEvent.Info.AppliedTo))
	{
		AddNewCombatant(HealingEvent.Info.AppliedTo);
		//Log and apply threat only if adding a new combatant. Otherwise, the healing taken event will do these for us.
		LogDamageEvent(HealingEvent, true);
		for (UCombatComponent* Component : CombatActors)
		{
			if (IsValid(Component) && Component->CanEverReceiveThreat()) //And component faction is hostile to healer's faction
				{
				Component->AddThreat(EThreatType::Healing, HealingEvent.ThreatInfo.BaseThreat, HealingEvent.Info.AppliedBy,
					HealingEvent.Info.Source, HealingEvent.ThreatInfo.IgnoreRestrictions, HealingEvent.ThreatInfo.IgnoreModifiers,
					HealingEvent.ThreatInfo.SourceModifier);
				}
		}
	}
}
