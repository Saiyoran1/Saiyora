#include "Threat/ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "CombatStatusComponent.h"

float UThreatHandler::GLOBALHEALINGTHREATMOD = 0.3f;
float UThreatHandler::GLOBALTAUNTTHREATPERCENT = 1.2f;

#pragma region Initialization

UThreatHandler::UThreatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UThreatHandler::InitializeComponent()
{
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Threat Handler."));
	CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
	checkf(IsValid(CombatStatusComponentRef), TEXT("Owner does not have a valid Combat Status Component, which Threat Handler depends on."));
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
}

void UThreatHandler::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwnerRole() == ROLE_Authority && IsValid(DamageHandlerRef))
	{
		if (DamageHandlerRef->HasHealth())
		{
			DamageHandlerRef->OnLifeStatusChanged.AddDynamic(this, &UThreatHandler::OnOwnerLifeStatusChanged);
		}
		if (DamageHandlerRef->CanEverReceiveDamage())
		{
			DamageHandlerRef->OnIncomingHealthEvent.AddDynamic(this, &UThreatHandler::OnOwnerDamageTaken);
		}
	}
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
	DOREPLIFETIME(UThreatHandler, bInCombat);
}

void UThreatHandler::OnOwnerDamageTaken(const FHealthEvent& DamageEvent)
{
	if (!DamageEvent.Result.Success || !DamageEvent.ThreatInfo.GeneratesThreat)
	{
		return;
	}
	AddThreat(EThreatType::Damage, DamageEvent.ThreatInfo.BaseThreat, DamageEvent.Info.AppliedBy,
		DamageEvent.Info.Source, DamageEvent.ThreatInfo.IgnoreRestrictions, DamageEvent.ThreatInfo.IgnoreModifiers,
		DamageEvent.ThreatInfo.SourceModifier);
}

void UThreatHandler::OnOwnerLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus,
	const ELifeStatus NewStatus)
{
	if (NewStatus != ELifeStatus::Alive)
	{
		ClearThreatTable();
	}
}

#pragma endregion 
#pragma region Threat

FThreatEvent UThreatHandler::AddThreat(const EThreatType ThreatType, const float BaseThreat, AActor* AppliedBy,
                                       UObject* Source, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const FThreatModCondition& SourceModifier)
{
	FThreatEvent Result;
	
	if (GetOwnerRole() != ROLE_Authority)
	{
		return Result;
	}
	if (!bHasThreatTable || BaseThreat < 0.0f || (IsValid(DamageHandlerRef) && DamageHandlerRef->IsDead()))
	{
		return Result;		
	}
	if (!IsValid(AppliedBy) || !AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return Result;
	}
	UThreatHandler* GeneratorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorThreat) || !GeneratorThreat->CanBeInThreatTable())
	{
		return Result;
	}
	const UDamageHandler* GeneratorHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
	if (IsValid(GeneratorHealth) && GeneratorHealth->IsDead())
	{
		return Result;
	}
	const UCombatStatusComponent* GeneratorCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(AppliedBy);
	if (!IsValid(GeneratorCombatStatus) || GeneratorCombatStatus->GetCurrentFaction() == CombatStatusComponentRef->GetCurrentFaction())
	{
		return Result;
	}

	bool bUseMisdirectTarget = false;
	if (IsValid(GeneratorThreat->GetMisdirectTarget()) && GeneratorThreat->GetMisdirectTarget()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		const UThreatHandler* MisdirectThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(GeneratorThreat->GetMisdirectTarget());
		if (IsValid(MisdirectThreat) && MisdirectThreat->CanBeInThreatTable())
		{
			const UDamageHandler* MisdirectHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(GeneratorThreat->GetMisdirectTarget());
			if (!IsValid(MisdirectHealth) || !MisdirectHealth->IsDead())
			{
				const UCombatStatusComponent* MisdirectCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GeneratorThreat->GetMisdirectTarget());
				if (IsValid(MisdirectCombatStatus) && MisdirectCombatStatus->GetCurrentFaction() != CombatStatusComponentRef->GetCurrentFaction())
				{
					bUseMisdirectTarget = true;
				}
			}
		}
	}
	
	Result.AppliedBy = bUseMisdirectTarget ? GeneratorThreat->GetMisdirectTarget() : AppliedBy;
	Result.AppliedTo = GetOwner();
	Result.Source = Source;
	Result.ThreatType = ThreatType;
	Result.AppliedByPlane = IsValid(GeneratorCombatStatus) ? GeneratorCombatStatus->GetCurrentPlane() : ESaiyoraPlane::None;
	Result.AppliedToPlane = IsValid(CombatStatusComponentRef) ? CombatStatusComponentRef->GetCurrentPlane() : ESaiyoraPlane::None;
	Result.AppliedXPlane = UCombatStatusComponent::CheckForXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
	Result.Threat = BaseThreat;

	if (!bIgnoreModifiers)
	{
		Result.Threat = GeneratorThreat->GetModifiedOutgoingThreat(Result, SourceModifier);
		Result.Threat = GetModifiedIncomingThreat(Result);
		if (Result.Threat <= 0.0f)
		{
			return Result;
		}
	}

	if (!bIgnoreRestrictions && (IncomingThreatRestrictions.IsRestricted(Result) || GeneratorThreat->CheckOutgoingThreatRestricted(Result)))
	{
		return Result;
	}

	int32 FoundIndex = ThreatTable.Num();
	bool bFound = false;
	for (; FoundIndex > 0; FoundIndex--)
	{
		if (ThreatTable[FoundIndex - 1].Target == Result.AppliedBy)
		{
			bFound = true;
			ThreatTable[FoundIndex - 1].Threat += Result.Threat;
			while (FoundIndex < ThreatTable.Num() && ThreatTable[FoundIndex] < ThreatTable[FoundIndex - 1])
			{
				ThreatTable.Swap(FoundIndex, FoundIndex - 1);
				FoundIndex++;
			}
			break;
		}
	}
	if (bFound && FoundIndex == ThreatTable.Num())
	{
		UpdateTarget();
	}
	if (!bFound)
	{
		Result.bInitialThreat = true;
		AddToThreatTable(FThreatTarget(Result.AppliedBy, Result.Threat, GeneratorThreat->HasActiveFade()));
	}
	Result.bSuccess = true;

	return Result;
}

void UThreatHandler::AddToThreatTable(const FThreatTarget& NewTarget)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(NewTarget.Target) || IsActorInThreatTable(NewTarget.Target))
	{
		return;
	}
	if (!NewTarget.Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return;
	}
	UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(NewTarget.Target);
	if (!IsValid(TargetThreatHandler) || !TargetThreatHandler->CanBeInThreatTable())
	{
		return;
	}
	const UCombatStatusComponent* TargetCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(NewTarget.Target);
	if (!IsValid(TargetCombatStatus) || TargetCombatStatus->GetCurrentFaction() == CombatStatusComponentRef->GetCurrentFaction())
	{
		return;
	}
	UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(NewTarget.Target);
	if (IsValid(TargetDamageHandler) && TargetDamageHandler->IsDead())
	{
		return;
	}
	bool bShouldUpdateTarget = false;
	bool bShouldUpdateCombat = false;
	if (ThreatTable.Num() == 0)
	{
		ThreatTable.Add(NewTarget);
		bShouldUpdateTarget = true;
		bShouldUpdateCombat = true;
	}
	else
	{
		int32 InsertIndex = 0;
		for (; InsertIndex < ThreatTable.Num(); InsertIndex++)
		{
			if (!(ThreatTable[InsertIndex] < NewTarget))
			{
				break;
			}
		}
		ThreatTable.Insert(NewTarget, InsertIndex);
		if (InsertIndex == ThreatTable.Num() - 1)
		{
			bShouldUpdateTarget = true;
		}
	}
	if (IsValid(TargetDamageHandler))
	{
		TargetDamageHandler->OnOutgoingHealthEvent.AddDynamic(this, &UThreatHandler::OnTargetHealingDone);
		if (TargetDamageHandler->HasHealth())
		{
			TargetDamageHandler->OnLifeStatusChanged.AddDynamic(this, &UThreatHandler::OnTargetLifeStatusChanged);
		}
		if (TargetDamageHandler->CanEverReceiveHealing())
		{
			TargetDamageHandler->OnIncomingHealthEvent.AddDynamic(this, &UThreatHandler::OnTargetHealingTaken);
		}
	}
	if (bShouldUpdateTarget)
	{
		UpdateTarget();
	}
	if (bShouldUpdateCombat)
	{
		UpdateCombat();
	}
	TargetThreatHandler->NotifyOfTargeting(this);
}

void UThreatHandler::NotifyOfTargeting(const UThreatHandler* TargetingComponent)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(TargetingComponent) || TargetedBy.Contains(TargetingComponent->GetOwner()))
	{
		return;
	}
	TargetedBy.Add(TargetingComponent->GetOwner());
	if (TargetedBy.Num() == 1)
	{
		UpdateCombat();
	}
	if (bHasThreatTable)
	{
		AddToThreatTable(FThreatTarget(TargetingComponent->GetOwner(), 0.0f, TargetingComponent->HasActiveFade()));
	}
}

void UThreatHandler::OnTargetHealingTaken(const FHealthEvent& HealthEvent)
{
	if (!HealthEvent.Result.Success || !HealthEvent.ThreatInfo.GeneratesThreat || HealthEvent.Info.EventType == EHealthEventType::Damage)
	{
		return;
	}
	AddThreat(EThreatType::Healing, (HealthEvent.ThreatInfo.BaseThreat * GLOBALHEALINGTHREATMOD), HealthEvent.Info.AppliedBy,
		HealthEvent.Info.Source, HealthEvent.ThreatInfo.IgnoreRestrictions, HealthEvent.ThreatInfo.IgnoreModifiers,
		HealthEvent.ThreatInfo.SourceModifier);
}

void UThreatHandler::OnTargetHealingDone(const FHealthEvent& HealthEvent)
{
	if (!HealthEvent.Result.Success || !HealthEvent.ThreatInfo.GeneratesThreat || HealthEvent.Info.EventType == EHealthEventType::Damage)
	{
		return;
	}
	//If a target heals someone not already in our threat table, we won't get a callback from that actor receiving the healing.
	if (!IsActorInThreatTable(HealthEvent.Info.AppliedTo))
	{
		//Add the threat, since we don't get the received healing callback to add it.
		AddThreat(EThreatType::Healing,(HealthEvent.ThreatInfo.BaseThreat * GLOBALHEALINGTHREATMOD), HealthEvent.Info.AppliedBy,
			HealthEvent.Info.Source, HealthEvent.ThreatInfo.IgnoreRestrictions, HealthEvent.ThreatInfo.IgnoreModifiers,
			HealthEvent.ThreatInfo.SourceModifier);
		//Then add the healed target to the threat table as well.
		const UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(HealthEvent.Info.AppliedTo);
		if (IsValid(TargetThreatHandler))
		{
			AddToThreatTable(FThreatTarget(HealthEvent.Info.AppliedTo, 0.0f, TargetThreatHandler->HasActiveFade()));
		}
	}
}

void UThreatHandler::OnTargetLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus,
	const ELifeStatus NewStatus)
{
	if (NewStatus != ELifeStatus::Alive)
	{
		RemoveFromThreatTable(Actor);
	}
}

void UThreatHandler::RemoveThreat(const float Amount, const AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(AppliedBy))
	{
		return;
	}
	bool bAffectedTarget = false;
	bool bFound = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == AppliedBy)
		{
			bFound = true;
			ThreatTable[i].Threat = FMath::Max(0.0f, ThreatTable[i].Threat - Amount);
			while (i > 0 && ThreatTable[i] < ThreatTable[i - 1])
			{
				ThreatTable.Swap(i, i - 1);
				if (!bAffectedTarget && i == ThreatTable.Num() - 1)
				{
					bAffectedTarget = true;
				}
				i--;
			}
			break;
		}
	}
	if (bFound && bAffectedTarget)
	{
		UpdateTarget();
	}
}

void UThreatHandler::RemoveFromThreatTable(const AActor* Actor)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Actor))
	{
		return;
	}
	bool bFound = false;
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Actor)
		{
			bFound = true;
			if (i == ThreatTable.Num() - 1)
			{
				bAffectedTarget = true;
			}
			ThreatTable.RemoveAt(i);
			break;
		}
	}
	if (bFound)
	{
		UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
		if (IsValid(TargetThreatHandler))
		{
			TargetThreatHandler->NotifyOfTargetingEnded(this);
		}
		UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Actor);
		if (IsValid(TargetDamageHandler))
		{
			TargetDamageHandler->OnOutgoingHealthEvent.RemoveDynamic(this, &UThreatHandler::OnTargetHealingDone);
			if (TargetDamageHandler->CanEverReceiveHealing())
			{
				TargetDamageHandler->OnIncomingHealthEvent.RemoveDynamic(this, &UThreatHandler::OnTargetHealingTaken);
			}
			if (TargetDamageHandler->HasHealth())
			{
				TargetDamageHandler->OnLifeStatusChanged.RemoveDynamic(this, &UThreatHandler::OnTargetLifeStatusChanged);
			}
		}
		if (bAffectedTarget)
		{
			UpdateTarget();
		}
		if (ThreatTable.Num() == 0)
		{
			UpdateCombat();
		}
	}
}

void UThreatHandler::NotifyOfTargetingEnded(const UThreatHandler* TargetingComponent)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(TargetingComponent) || !TargetedBy.Contains(TargetingComponent->GetOwner()))
	{
		return;
	}
	if (TargetedBy.Remove(TargetingComponent->GetOwner()) > 0 && TargetedBy.Num() == 0)
	{
		UpdateCombat();
	}
}

void UThreatHandler::ClearThreatTable()
{
	for (const FThreatTarget& ThreatTarget : ThreatTable)
	{
		UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(ThreatTarget.Target);
		if (IsValid(TargetDamageHandler))
		{
			TargetDamageHandler->OnOutgoingHealthEvent.RemoveDynamic(this, &UThreatHandler::OnTargetHealingDone);
			if (TargetDamageHandler->CanEverReceiveHealing())
			{
				TargetDamageHandler->OnIncomingHealthEvent.RemoveDynamic(this, &UThreatHandler::OnTargetHealingTaken);
			}
			if (TargetDamageHandler->HasHealth())
			{
				TargetDamageHandler->OnLifeStatusChanged.RemoveDynamic(this, &UThreatHandler::OnTargetLifeStatusChanged);
			}
		}
		UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(ThreatTarget.Target);
		if (IsValid(TargetThreatHandler))
		{
			TargetThreatHandler->NotifyOfTargetingEnded(this);
		}
	}
	ThreatTable.Empty();
	UpdateTarget();
	UpdateCombat();
}

bool UThreatHandler::IsActorInThreatTable(const AActor* Target) const
{
	if (!bHasThreatTable || !bInCombat || !IsValid(Target))
	{
		return false;
	}
	for (const FThreatTarget& ThreatTarget : ThreatTable)
	{
		if (ThreatTarget.Target == Target)
		{
			return true;
		}
	}
	return false;
}

float UThreatHandler::GetActorThreatValue(const AActor* Actor) const
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Actor))
	{
		return 0.0f;
	}
	for (const FThreatTarget& Target : ThreatTable)
	{
		if (Target.Target == Actor)
		{
			return Target.Threat;
		}
	}
	return 0.0f;
}

void UThreatHandler::GetActorsInThreatTable(TArray<AActor*>& OutActors) const
{
	OutActors.Empty();
	for (const FThreatTarget& Target : ThreatTable)
	{
		if (IsValid(Target.Target))
		{
			OutActors.Add(Target.Target);
		}
	}
}

void UThreatHandler::UpdateTarget()
{
	AActor* Previous = CurrentTarget;
	if (ThreatTable.Num() == 0)
	{
		CurrentTarget = nullptr;
	}
	else
	{
		CurrentTarget = ThreatTable[ThreatTable.Num() - 1].Target;
	}
	if (CurrentTarget != Previous)
	{
		OnTargetChanged.Broadcast(Previous, CurrentTarget);
	}
}

void UThreatHandler::UpdateCombat()
{
	const bool PreviouslyInCombat = bInCombat;
	bInCombat = ThreatTable.Num() > 0 || TargetedBy.Num() > 0;
	if (bInCombat != PreviouslyInCombat)
	{
		OnCombatChanged.Broadcast(bInCombat);
	}
}

void UThreatHandler::OnRep_bInCombat()
{
	OnCombatChanged.Broadcast(bInCombat);
}

void UThreatHandler::OnRep_CurrentTarget(AActor* PreviousTarget)
{
	OnTargetChanged.Broadcast(PreviousTarget, CurrentTarget);
}

#pragma endregion 
#pragma region Modifiers

float UThreatHandler::GetModifiedIncomingThreat(const FThreatEvent& ThreatEvent) const
{
	TArray<FCombatModifier> Mods;
	IncomingThreatMods.GetModifiers(Mods, ThreatEvent);
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

float UThreatHandler::GetModifiedOutgoingThreat(const FThreatEvent& ThreatEvent, const FThreatModCondition& SourceModifier) const
{
	TArray<FCombatModifier> Mods;
	OutgoingThreatMods.GetModifiers(Mods, ThreatEvent);
	if (SourceModifier.IsBound())
	{
		Mods.Add(SourceModifier.Execute(ThreatEvent));
	}
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

#pragma endregion
#pragma region Threat Special Actions

void UThreatHandler::Taunt(AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(AppliedBy))
	{
		return;
	}
	const float InitialThreat = GetActorThreatValue(AppliedBy);
	float HighestThreat = 0.0f;
	for (const FThreatTarget& Target : ThreatTable)
	{
		if (Target.Threat > HighestThreat)
		{
			HighestThreat = Target.Threat;
		}
	}
	HighestThreat *= GLOBALTAUNTTHREATPERCENT;
	AddThreat(EThreatType::Absolute, FMath::Max(0.0f, HighestThreat - InitialThreat), AppliedBy, nullptr, true, true, FThreatModCondition());
}

void UThreatHandler::DropThreat(AActor* Target, const float Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Target))
	{
		return;
	}
	const float DropThreat = GetActorThreatValue(Target) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (DropThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(DropThreat, Target);
}

void UThreatHandler::TransferThreat(AActor* FromActor, AActor* ToActor, const float Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(FromActor) || !IsValid(ToActor))
	{
		return;
	}
	float const TransferThreat = GetActorThreatValue(FromActor) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (TransferThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(TransferThreat, FromActor);
	AddThreat(EThreatType::Absolute, TransferThreat, ToActor, nullptr, true, true, FThreatModCondition());
}

void UThreatHandler::Vanish()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const TArray<AActor*> TargetingActors = TargetedBy;
	for (const AActor* Targeting : TargetingActors)
	{
		UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Targeting);
		if (IsValid(TargetThreatHandler))
		{
			TargetThreatHandler->NotifyOfTargetVanished(GetOwner());
		}
	}
}

void UThreatHandler::AddFixate(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Target) || !Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass())|| !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	const UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanBeInThreatTable())
	{
		return;
	}
	bool bFound = false;
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			bFound = true;
			const int32 PreviousFixates = ThreatTable[i].Fixates.Num();
			ThreatTable[i].Fixates.AddUnique(Source);
			//If this was the first fixate for this target, possible reorder on threat.
			if (PreviousFixates == 0 && ThreatTable[i].Fixates.Num() == 1)
			{
				while (i < ThreatTable.Num() - 1 && ThreatTable[i + 1] < ThreatTable[i])
				{
					ThreatTable.Swap(i, i + 1);
					if (!bAffectedTarget && i + 1 == ThreatTable.Num() - 1)
					{
						bAffectedTarget = true;
					}
					i++;
				}
			}
			break;
		}
	}
	if (bFound && bAffectedTarget)
	{
		UpdateTarget();
	}
	//If this unit was not in the threat table, need to add them.
	if (!bFound)
	{
		AddToThreatTable(FThreatTarget(Target, 0.0f, GeneratorComponent->HasActiveFade(), Source));
	}
}

void UThreatHandler::RemoveFixate(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source))
	{
		return;
	}
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			if (ThreatTable[i].Fixates.Num() > 0)
			{
				ThreatTable[i].Fixates.Remove(Source);
				//If we removed the last fixate for this target, possibly reorder on threat.
				if (ThreatTable[i].Fixates.Num() == 0)
				{
					while (i > 0 && ThreatTable[i] < ThreatTable[i - 1])
					{
						ThreatTable.Swap(i, i - 1);
						//If the last index was swapped, this will change current target.
						if (!bAffectedTarget && i == ThreatTable.Num() - 1)
						{
							bAffectedTarget = true;
						}
						i--;
					}
				}
			}
			break;
		}
	}
	if (bAffectedTarget)
	{
		UpdateTarget();
	}
}

void UThreatHandler::AddBlind(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	const UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanBeInThreatTable())
	{
		return;
	}
	
	bool bFound = false;
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			bFound = true;
			const int32 PreviousBlinds = ThreatTable[i].Blinds.Num();
			ThreatTable[i].Blinds.AddUnique(Source);
			//If this is the first Blind for the unit, possibly reorder on threat.
			if (PreviousBlinds == 0 && ThreatTable[i].Blinds.Num() == 1)
			{
				while (i > 0 && ThreatTable[i] < ThreatTable[i - 1])
				{
					ThreatTable.Swap(i, i - 1);
					//If the last index was affected by the swap, this will change current target.
					if (!bAffectedTarget && i == ThreatTable.Num() - 1)
					{
						bAffectedTarget = true;
					}
					i--;
				}
			}
			break;
		}
	}
	if (bFound && bAffectedTarget)
	{
		UpdateTarget();
	}
	if (!bFound)
	{
		AddToThreatTable(FThreatTarget(Target, 0.0f, GeneratorComponent->HasActiveFade(), nullptr, Source));
	}
}

void UThreatHandler::RemoveBlind(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source))
	{
		return;
	}
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			if (ThreatTable[i].Blinds.Num() > 0)
			{
				ThreatTable[i].Blinds.Remove(Source);
				//If we went from non-zero Blinds to zero Blinds, possibly have to reorder on threat.
				if (ThreatTable[i].Blinds.Num() == 0)
				{
					while (i < ThreatTable.Num() - 1 && ThreatTable[i + 1] < ThreatTable[i])
					{
						ThreatTable.Swap(i, i + 1);
						//If the last index was swapped, then this will change the current target.
						if (!bAffectedTarget && i == ThreatTable.Num() - 1)
						{
							bAffectedTarget = true;
						}
						i++;
					}
				}
			}
			break;
		}
	}
	if (bAffectedTarget)
	{
		UpdateTarget();
	}
}

void UThreatHandler::AddFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (Fades.Contains(Source))
	{
		return;
	}
	Fades.Add(Source);
	if (Fades.Num() == 1)
	{
		for (const AActor* Targeting : TargetedBy)
		{
			UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Targeting);
			if (IsValid(TargetThreatHandler))
			{
				TargetThreatHandler->NotifyOfTargetFadeStatusUpdate(GetOwner(), true);
			}
		}
	}
}

void UThreatHandler::RemoveFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	if (Fades.Remove(Source) != 0 && Fades.Num() == 0)
	{
		for (const AActor* Targeting : TargetedBy)
		{
			UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Targeting);
			if (IsValid(TargetThreatHandler))
			{
				TargetThreatHandler->NotifyOfTargetFadeStatusUpdate(GetOwner(), false);
			}
		}
	}
}

void UThreatHandler::NotifyOfTargetFadeStatusUpdate(const AActor* Target, const bool FadeStatus)
{
	if (!IsValid(Target))
	{
		return;
	}
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Target)
		{
			bool AffectedTarget = false;
			ThreatTable[i].Faded = FadeStatus;
			if (FadeStatus)
			{
				if (ThreatTable[i].Blinds.Num() == 0)
				{
					while (i > 0 && ThreatTable[i] < ThreatTable[i - 1])
					{
						ThreatTable.Swap(i, i - 1);
						if (!AffectedTarget && i == ThreatTable.Num() - 1)
						{
							AffectedTarget = true;
						}
						i--;
					}
				}
			}
			else
			{
				if (ThreatTable[i].Blinds.Num() == 0)
				{
					while (i < ThreatTable.Num() - 1 && ThreatTable[i + 1] < ThreatTable[i])
					{
						ThreatTable.Swap(i, i + 1);
						if (!AffectedTarget && i + 1 == ThreatTable.Num() - 1)
						{
							AffectedTarget = true;
						}
						i++;
					}
				}
			}
			if (AffectedTarget)
			{
				UpdateTarget();
			}
			return;
		}
	}
}

void UThreatHandler::AddMisdirect(UBuff* Source, AActor* Target)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Target) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	for (const FMisdirect& Misdirect : Misdirects)
	{
		if (Misdirect.Source == Source)
		{
			return;
		}
	}
	Misdirects.Add(FMisdirect(Source, Target));
}

void UThreatHandler::RemoveMisdirect(const UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	for (const FMisdirect& Misdirect : Misdirects)
	{
		if (Misdirect.Source == Source)
		{
			Misdirects.Remove(Misdirect);
			return;
		}
	}
}

#pragma endregion