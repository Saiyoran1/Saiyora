#include "CombatGroup.h"
#include "CombatStatusComponent.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

bool UCombatGroup::AddCombatant(UThreatHandler* Combatant)
{
	const UCombatStatusComponent* CombatantCombat = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Combatant->GetOwner());
	if (!IsValid(CombatantCombat))
	{
		return false;
	}
	UDamageHandler* CombatantDamage = ISaiyoraCombatInterface::Execute_GetDamageHandler(Combatant->GetOwner());
	if (IsValid(CombatantDamage) && CombatantDamage->IsDead())
	{
		return false;
	}
	const bool bIsFriendly = CombatantCombat->GetCurrentFaction() == EFaction::Friendly;
	if (bIsFriendly)
	{
		if (Friendlies.Contains(Combatant))
		{
			return false;
		}
		Friendlies.Add(Combatant);
	}
	else
	{
		if (Enemies.Contains(Combatant))
		{
			return false;
		}
		Enemies.Add(Combatant);
	}
	if (IsValid(CombatantDamage))
	{
		CombatantDamage->OnIncomingHealthEvent.AddDynamic(this, &UCombatGroup::OnCombatantIncomingHealthEvent);
		CombatantDamage->OnOutgoingHealthEvent.AddDynamic(this, &UCombatGroup::OnCombatantOutgoingHealthEvent);
		CombatantDamage->OnLifeStatusChanged.AddDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
	}
	Combatant->NotifyOfCombat(this);
	for (UThreatHandler* Enemy : bIsFriendly ? Enemies : Friendlies)
	{
		Enemy->NotifyOfNewCombatant(Combatant);
		Combatant->NotifyOfNewCombatant(Enemy);
	}
	return true;
}

void UCombatGroup::RemoveCombatant(UThreatHandler* Combatant)
{
	if (!IsValid(Combatant))
	{
		return;
	}
	const UCombatStatusComponent* CombatantCombat = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Combatant->GetOwner());
	if (!IsValid(CombatantCombat))
	{
		return;
	}
	const bool bIsFriendly = CombatantCombat->GetCurrentFaction() == EFaction::Friendly;
	bool bCombatShouldEnd = false;
	if (bIsFriendly)
	{
		if (Friendlies.Remove(Combatant) <= 0)
		{
			return;
		}
		if (Friendlies.Num() == 0)
		{
			bCombatShouldEnd = true;
		}
	}
	else
	{
		if (Enemies.Remove(Combatant) <= 0)
		{
			return;
		}
		if (Enemies.Num() == 0)
		{
			bCombatShouldEnd = true;
		}
	}
	UDamageHandler* CombatantDamage = ISaiyoraCombatInterface::Execute_GetDamageHandler(Combatant->GetOwner());
	if (IsValid(CombatantDamage))
	{
		CombatantDamage->OnIncomingHealthEvent.RemoveDynamic(this, &UCombatGroup::OnCombatantIncomingHealthEvent);
		CombatantDamage->OnOutgoingHealthEvent.RemoveDynamic(this, &UCombatGroup::OnCombatantOutgoingHealthEvent);
		CombatantDamage->OnLifeStatusChanged.RemoveDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
	}
	Combatant->NotifyOfCombat(nullptr);
	for (UThreatHandler* Enemy : bIsFriendly ? Enemies : Friendlies)
	{
		if (bCombatShouldEnd)
		{
			Enemy->NotifyOfCombat(nullptr);
		}
		else
		{
			Enemy->NotifyOfCombatantLeft(Combatant);
		}
	}
}

void UCombatGroup::MergeWith(UCombatGroup* OtherGroup)
{
	for (UThreatHandler* Friendly : OtherGroup->Friendlies)
	{
		Friendly->NotifyOfCombat(this);
		for (UThreatHandler* Enemy : Enemies)
		{
			Friendly->NotifyOfNewCombatant(Enemy);
			Enemy->NotifyOfNewCombatant(Friendly);
		}
		UDamageHandler* FriendlyDamage = ISaiyoraCombatInterface::Execute_GetDamageHandler(Friendly->GetOwner());
		if (IsValid(FriendlyDamage))
		{
			FriendlyDamage->OnIncomingHealthEvent.AddDynamic(this, &UCombatGroup::OnCombatantIncomingHealthEvent);
			FriendlyDamage->OnOutgoingHealthEvent.AddDynamic(this, &UCombatGroup::OnCombatantOutgoingHealthEvent);
			FriendlyDamage->OnLifeStatusChanged.AddDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
		}
	}
	for (UThreatHandler* Enemy : OtherGroup->Enemies)
	{
		Enemy->NotifyOfCombat(this);
		for (UThreatHandler* Friendly : Friendlies)
		{
			Enemy->NotifyOfNewCombatant(Friendly);
			Friendly->NotifyOfNewCombatant(Enemy);
		}
		UDamageHandler* EnemyDamage = ISaiyoraCombatInterface::Execute_GetDamageHandler(Enemy->GetOwner());
		if (IsValid(EnemyDamage))
		{
			EnemyDamage->OnIncomingHealthEvent.AddDynamic(this, &UCombatGroup::OnCombatantIncomingHealthEvent);
			EnemyDamage->OnOutgoingHealthEvent.AddDynamic(this, &UCombatGroup::OnCombatantOutgoingHealthEvent);
			EnemyDamage->OnLifeStatusChanged.AddDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
		}
	}
	Friendlies.Append(OtherGroup->Friendlies);
	Enemies.Append(OtherGroup->Enemies);
	OtherGroup->NotifyOfMerge();
}

void UCombatGroup::NotifyOfMerge()
{
	for (const UThreatHandler* Friendly : Friendlies)
	{
		UDamageHandler* FriendlyDamage = ISaiyoraCombatInterface::Execute_GetDamageHandler(Friendly->GetOwner());
		if (IsValid(FriendlyDamage))
		{
			FriendlyDamage->OnIncomingHealthEvent.RemoveDynamic(this, &UCombatGroup::OnCombatantIncomingHealthEvent);
			FriendlyDamage->OnOutgoingHealthEvent.RemoveDynamic(this, &UCombatGroup::OnCombatantOutgoingHealthEvent);
			FriendlyDamage->OnLifeStatusChanged.RemoveDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
		}
	}
	for (const UThreatHandler* Enemy : Enemies)
	{
		UDamageHandler* EnemyDamage = ISaiyoraCombatInterface::Execute_GetDamageHandler(Enemy->GetOwner());
		if (IsValid(EnemyDamage))
		{
			EnemyDamage->OnIncomingHealthEvent.RemoveDynamic(this, &UCombatGroup::OnCombatantIncomingHealthEvent);
			EnemyDamage->OnOutgoingHealthEvent.RemoveDynamic(this, &UCombatGroup::OnCombatantOutgoingHealthEvent);
			EnemyDamage->OnLifeStatusChanged.RemoveDynamic(this, &UCombatGroup::OnCombatantLifeStatusChanged);
		}
	}
	Friendlies.Empty();
	Enemies.Empty();
}

void UCombatGroup::UpdateCombatantFadeStatus(const UThreatHandler* Combatant, const bool bFaded)
{
	if (!IsValid(Combatant))
	{
		return;
	}
	const UCombatStatusComponent* CombatantCombat = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Combatant->GetOwner());
	if (!IsValid(CombatantCombat))
	{
		return;
	}
	if (CombatantCombat->GetCurrentFaction() == EFaction::Friendly)
	{
		for (UThreatHandler* Enemy : Enemies)
		{
			Enemy->NotifyOfCombatantFadeStatus(Combatant, bFaded);
		}
	}
	else
	{
		for (UThreatHandler* Friendly : Friendlies)
		{
			Friendly->NotifyOfCombatantFadeStatus(Combatant, bFaded);
		}
	}
}

void UCombatGroup::OnCombatantIncomingHealthEvent(const FHealthEvent& Event)
{
	if (!Event.Result.Success || !Event.ThreatInfo.GeneratesThreat ||
		!(Event.Info.EventType == EHealthEventType::Healing || Event.Info.EventType == EHealthEventType::Absorb))
	{
		return;
	}
	if (!IsValid(Event.Info.AppliedBy) || !Event.Info.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return;
	}
	UThreatHandler* HealerThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Event.Info.AppliedBy);
	if (!IsValid(HealerThreat) || !HealerThreat->CanBeInThreatTable())
	{
		return;
	}
	const UCombatStatusComponent* HealerCombat = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Event.Info.AppliedBy);
	if (!IsValid(HealerCombat))
	{
		return;
	}
	const bool bIsFriendly = HealerCombat->GetCurrentFaction() == EFaction::Friendly;
	if (IsValid(HealerThreat->GetCombatGroup()))
	{
		if (HealerThreat->GetCombatGroup() != this)
		{
			MergeWith(HealerThreat->GetCombatGroup());
		}
	}
	else
	{
		if (!AddCombatant(HealerThreat))
		{
			return;
		}
	}
	for (UThreatHandler* Enemy : bIsFriendly ? Enemies : Friendlies)
	{
		Enemy->AddThreat(EThreatType::Healing, Event.ThreatInfo.SeparateBaseThreat ? Event.ThreatInfo.BaseThreat : Event.Result.AppliedValue,
			Event.Info.AppliedBy, Event.Info.Source,Event.ThreatInfo.IgnoreRestrictions, Event.ThreatInfo.IgnoreModifiers,
			Event.ThreatInfo.SourceModifier);
	}
}

void UCombatGroup::OnCombatantOutgoingHealthEvent(const FHealthEvent& Event)
{
	if (!Event.Result.Success || !Event.ThreatInfo.GeneratesThreat ||
		!(Event.Info.EventType == EHealthEventType::Healing || Event.Info.EventType == EHealthEventType::Absorb))
	{
		return;
	}
	if (!IsValid(Event.Info.AppliedTo) || !Event.Info.AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass())
		|| !IsValid(Event.Info.AppliedBy) || !Event.Info.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return;
	}
	UThreatHandler* TargetThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Event.Info.AppliedTo);
	if (!IsValid(TargetThreat) || !TargetThreat->CanBeInThreatTable() || IsValid(TargetThreat->GetCombatGroup()))
	{
		return;
	}
	AddCombatant(TargetThreat);
	const UCombatStatusComponent* HealerCombat = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Event.Info.AppliedBy);
	if (!IsValid(HealerCombat))
	{
		return;
	}
	const bool bIsFriendly = HealerCombat->GetCurrentFaction() == EFaction::Friendly;
	for (UThreatHandler* Enemy : bIsFriendly ? Enemies : Friendlies)
	{
		Enemy->AddThreat(EThreatType::Healing, Event.ThreatInfo.SeparateBaseThreat ? Event.ThreatInfo.BaseThreat : Event.Result.AppliedValue,
			Event.Info.AppliedBy, Event.Info.Source,Event.ThreatInfo.IgnoreRestrictions, Event.ThreatInfo.IgnoreModifiers,
			Event.ThreatInfo.SourceModifier);
	}
}

void UCombatGroup::OnCombatantLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (NewStatus != ELifeStatus::Alive && IsValid(Actor)
		&& Actor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UThreatHandler* Combatant = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
		if (IsValid(Combatant))
		{
			RemoveCombatant(Combatant);
		}
	}
}

