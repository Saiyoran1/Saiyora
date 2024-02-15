#include "Threat/ThreatHandler.h"
#include "AbilityComponent.h"
#include "AbilityFunctionLibrary.h"
#include "AggroRadius.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "CombatGroup.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "CombatStatusComponent.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlayerCharacter.h"

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
	//checkf(IsValid(CombatStatusComponentRef), TEXT("Owner does not have a valid Combat Status Component, which Threat Handler depends on."));
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	UAbilityComponent* AbilityComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwner());
	if (IsValid(AbilityComponent))
	{
		NPCComponentRef = Cast<UNPCAbilityComponent>(AbilityComponent);
	}
	DisableThreatEvents.BindDynamic(this, &UThreatHandler::DisableAllThreatEvents);
}

void UThreatHandler::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(DamageHandlerRef))
		{
			if (DamageHandlerRef->CanEverReceiveDamage())
			{
				DamageHandlerRef->OnIncomingHealthEvent.AddDynamic(this, &UThreatHandler::OnOwnerDamageTaken);
			}
		}
		if (IsValid(NPCComponentRef))
		{
			NPCComponentRef->OnCombatBehaviorChanged.AddDynamic(this, &UThreatHandler::OnCombatBehaviorChanged);
			if (NPCComponentRef->GetCombatBehavior() == ENPCCombatBehavior::None || NPCComponentRef->GetCombatBehavior() == ENPCCombatBehavior::Resetting)
			{
				AddOutgoingThreatRestriction(DisableThreatEvents);
				AddIncomingThreatRestriction(DisableThreatEvents);
			}
		}
		if (bProximityAggro && DefaultDetectionRadius > 0.0f)
		{
			AggroRadius = Cast<UAggroRadius>(GetOwner()->AddComponentByClass(UAggroRadius::StaticClass(), false, FTransform::Identity, false));
			if (IsValid(AggroRadius))
			{
				AggroRadius->Initialize(this, DefaultDetectionRadius);
			}
		}
	}
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
	DOREPLIFETIME(UThreatHandler, bInCombat);
	DOREPLIFETIME(UThreatHandler, CombatStartTime);
}

void UThreatHandler::OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior,
	const ENPCCombatBehavior NewBehavior)
{
	const bool bWasDisabled = PreviousBehavior == ENPCCombatBehavior::None || PreviousBehavior == ENPCCombatBehavior::Resetting;
	const bool bShouldBeDisabled = NewBehavior == ENPCCombatBehavior::None || NewBehavior == ENPCCombatBehavior::Resetting;
	if (bWasDisabled && !bShouldBeDisabled)
	{
		RemoveOutgoingThreatRestriction(DisableThreatEvents);
		RemoveIncomingThreatRestriction(DisableThreatEvents);
	}
	else if (!bWasDisabled && bShouldBeDisabled)
	{
		AddOutgoingThreatRestriction(DisableThreatEvents);
		AddIncomingThreatRestriction(DisableThreatEvents);
	}
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

float UThreatHandler::GetCombatTime() const
{
	if (CombatStartTime <= 0.0f)
	{
		return 0.0f;
	}
	if (!IsValid(GetWorld()) || !IsValid(GetWorld()->GetGameState()))
	{
		return 0.0f;
	}
	return GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - CombatStartTime;
}

#pragma endregion 
#pragma region Threat

void UThreatHandler::NotifyOfNewCombatant(UThreatHandler* Combatant)
{
	const FThreatTarget NewTarget = FThreatTarget(Combatant);
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
		UpdateTarget();
	}
}

void UThreatHandler::NotifyOfCombatantLeft(const UThreatHandler* Combatant)
{
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat == Combatant)
		{
			ThreatTable.RemoveAt(i);
			if (i == ThreatTable.Num())
			{
				UpdateTarget();
			}
			break;
		}
	}
}

int32 UThreatHandler::FindOrAddToThreatTable(UThreatHandler* Target, bool& bAdded)
{
	bAdded = false;
	if (!IsValid(Target))
	{
		return -1;
	}
	//Check threat table for already existing entry.
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat == Target)
		{
			return i;
		}
	}
	//Not in threat table, need to enter combat with the target.
	if (bInCombat && IsValid(CombatGroup))
	{
		if (Target->IsInCombat() && IsValid(Target->GetCombatGroup()))
		{
			//Both handlers are already in separate combat groups.
			CombatGroup->MergeWith(Target->GetCombatGroup());
		}
		else
		{
			//This handler is in combat but enemy isn't.
			CombatGroup->AddCombatant(Target);
		}
	}
	else
	{
		if (Target->IsInCombat() && IsValid(Target->GetCombatGroup()))
		{
			//This handler is not in combat, but the enemy is.
			Target->GetCombatGroup()->AddCombatant(this);
		}
		else
		{
			//Neither handler is in combat.
			UCombatGroup* NewCombat = NewObject<UCombatGroup>();
			if (!IsValid(NewCombat))
			{
				return -1;
			}
			NewCombat->AddCombatant(this);
			NewCombat->AddCombatant(Target);
		}
	}
	//After entering combat, check the threat table again, as the combat group should've added the new combatant.
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat == Target)
		{
			bAdded = true;
			return i;
		}
	}
	return -1;
}

int32 UThreatHandler::FindInThreatTable(const UThreatHandler* Target) const
{
	if (!IsValid(Target) || !Target->CanBeInThreatTable())
	{
		return -1;
	}
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat == Target)
		{
			return i;
		}
	}
	return -1;
}

int32 UThreatHandler::FindInThreatTable(const AActor* Target) const
{
	if (!IsValid(Target))
	{
		return -1;
	}
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].TargetThreat->GetOwner() == Target)
		{
			return i;
		}
	}
	return -1;
}

void UThreatHandler::ClearThreatTable()
{
	ThreatTable.Empty();
	UpdateTarget();
}

FThreatEvent UThreatHandler::AddThreat(const EThreatType ThreatType, const float BaseThreat, AActor* AppliedBy,
                                       UObject* Source, const bool bIgnoreRestrictions, const bool bIgnoreModifiers, const FThreatModCondition& SourceModifier)
{
	FThreatEvent Result;
	
	if (GetOwnerRole() != ROLE_Authority)
	{
		return Result;
	}
	if (!bHasThreatTable || BaseThreat <= 0.0f || (IsValid(DamageHandlerRef) && DamageHandlerRef->IsDead()))
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
	
	bool bUseMisdirectTarget = false;
	UThreatHandler* MisdirectThreat = nullptr;
	if (IsValid(GeneratorThreat->GetMisdirectTarget()) && GeneratorThreat->GetMisdirectTarget()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		MisdirectThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(GeneratorThreat->GetMisdirectTarget());
		if (IsValid(MisdirectThreat) && MisdirectThreat->CanBeInThreatTable())
		{
			const UDamageHandler* MisdirectHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(GeneratorThreat->GetMisdirectTarget());
			if (!IsValid(MisdirectHealth) || !MisdirectHealth->IsDead())
			{
				const UCombatStatusComponent* MisdirectCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GeneratorThreat->GetMisdirectTarget());
				if (IsValid(MisdirectCombatStatus) && MisdirectCombatStatus->GetCurrentFaction() != CombatStatusComponentRef->GetCurrentFaction())
				{
					bUseMisdirectTarget = true;
					Result.AppliedByPlane = MisdirectCombatStatus->GetCurrentPlane();
				}
			}
		}
	}
	if (!bUseMisdirectTarget)
	{
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
		Result.AppliedByPlane = GeneratorCombatStatus->GetCurrentPlane();
	}
	
	Result.AppliedBy = bUseMisdirectTarget ? GeneratorThreat->GetMisdirectTarget() : AppliedBy;
	Result.AppliedTo = GetOwner();
	Result.Source = Source;
	Result.ThreatType = ThreatType;
	Result.AppliedToPlane = IsValid(CombatStatusComponentRef) ? CombatStatusComponentRef->GetCurrentPlane() : ESaiyoraPlane::Both;
	Result.AppliedXPlane = UAbilityFunctionLibrary::IsXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
	Result.Threat = BaseThreat;

	if (!bIgnoreModifiers)
	{
		Result.Threat = GeneratorThreat->GetModifiedOutgoingThreat(Result, SourceModifier);
		//Currently this uses the generator to modify outgoing threat, even if a misdirect is applied.
		Result.Threat = GetModifiedIncomingThreat(Result);
		if (Result.Threat <= 0.0f)
		{
			return Result;
		}
	}

	if (!bIgnoreRestrictions && (IncomingThreatRestrictions.IsRestricted(Result) || GeneratorThreat->CheckOutgoingThreatRestricted(Result)))
	{
		//Currently this uses the generator to check restrictions, even if a misdirect is active.
		return Result;
	}
	
	const int32 TargetIndex = FindOrAddToThreatTable(bUseMisdirectTarget ? MisdirectThreat : GeneratorThreat, Result.bInitialThreat);
	if (TargetIndex == -1)
	{
		//Failed to add to threat table.
		return Result;
	}
	ThreatTable[TargetIndex].Threat += Result.Threat;
	SortModifiedThreatTarget(TargetIndex);
	Result.bSuccess = true;
	return Result;
}

void UThreatHandler::RemoveThreat(const float Amount, const AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(AppliedBy) || Amount <= 0.0f)
	{
		return;
	}
	if (!AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return;
	}
	const UThreatHandler* TargetThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(TargetThreat) || !TargetThreat->CanBeInThreatTable())
	{
		return;
	}
	const int32 TargetIndex = FindInThreatTable(TargetThreat);
	if (TargetIndex == -1)
	{
		return;
	}
	ThreatTable[TargetIndex].Threat = FMath::Max(0.0f, ThreatTable[TargetIndex].Threat - Amount);
	SortModifiedThreatTarget(TargetIndex);
}

void UThreatHandler::SortModifiedThreatTarget(const int32 ModifiedIndex)
{
	if (ThreatTable.Num() <= 1 || !ThreatTable.IsValidIndex(ModifiedIndex))
	{
		return;
	}
	int32 CurrentIndex = ModifiedIndex;
	while (CurrentIndex >= 0 && CurrentIndex <= ThreatTable.Num() - 1)
	{
		if (CurrentIndex > 0 && ThreatTable[CurrentIndex] < ThreatTable[CurrentIndex - 1])
		{
			ThreatTable.Swap(CurrentIndex, CurrentIndex - 1);
			CurrentIndex--;
		}
		else if (CurrentIndex < ThreatTable.Num() - 1 && ThreatTable[CurrentIndex + 1] < ThreatTable[CurrentIndex])
		{
			ThreatTable.Swap(CurrentIndex, CurrentIndex + 1);
			CurrentIndex++;
		}
		else
		{
			break;
		}
	}
	if (ModifiedIndex == ThreatTable.Num() - 1 || CurrentIndex == ThreatTable.Num() - 1)
	{
		UpdateTarget();
	}
}

bool UThreatHandler::IsActorInThreatTable(const AActor* Target) const
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !bInCombat || !IsValid(Target))
	{
		return false;
	}
	return FindInThreatTable(Target) != -1;
}

float UThreatHandler::GetActorThreatValue(const AActor* Target) const
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !bInCombat || !IsValid(Target))
	{
		return 0.0f;
	}
	const int32 Index = FindInThreatTable(Target);
	if (Index == -1)
	{
		return 0.0f;
	}
	return ThreatTable[Index].Threat;
}

void UThreatHandler::GetActorsInThreatTable(TArray<AActor*>& OutActors) const
{
	OutActors.Empty();
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !bInCombat)
	{
		return;
	}
	for (const FThreatTarget& Target : ThreatTable)
	{
		if (IsValid(Target.TargetThreat))
		{
			OutActors.Add(Target.TargetThreat->GetOwner());
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
		CurrentTarget = ThreatTable[ThreatTable.Num() - 1].TargetThreat->GetOwner();
	}
	if (CurrentTarget != Previous)
	{
		OnTargetChanged.Broadcast(Previous, CurrentTarget);
	}
}

void UThreatHandler::OnRep_bInCombat()
{
	OnCombatChanged.Broadcast(this, bInCombat);
	NotifyLocalPlayerOfCombatChange();
}

void UThreatHandler::NotifyLocalPlayerOfCombatChange()
{
	if (IsValid(CombatStatusComponentRef) && CombatStatusComponentRef->GetCurrentFaction() == EFaction::Enemy)
	{
		if (!IsValid(LocalPlayer))
		{
			LocalPlayer = USaiyoraCombatLibrary::GetLocalSaiyoraPlayer(this);
		}
		if (IsValid(LocalPlayer))
		{
			LocalPlayer->NotifyEnemyCombatChanged(GetOwner(), bInCombat);
		}
	}
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
		//Have to search for highest threat because current target might be due to fixate/blind and not highest threat value.
		if (Target.Threat > HighestThreat)
		{
			HighestThreat = Target.Threat;
		}
	}
	HighestThreat *= GLOBALTAUNTTHREATPERCENT;
	AddThreat(EThreatType::Absolute, FMath::Max(0.0f, HighestThreat - InitialThreat), AppliedBy, nullptr, false, true, FThreatModCondition());
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
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(FromActor) || !IsValid(ToActor))
	{
		return;
	}
	float const TransferThreat = GetActorThreatValue(FromActor) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (TransferThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(TransferThreat, FromActor);
	AddThreat(EThreatType::Absolute, TransferThreat, ToActor, nullptr, false, true, FThreatModCondition());
}

void UThreatHandler::Vanish()
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(CombatGroup))
	{
		return;
	}
	CombatGroup->RemoveCombatant(this);
}

void UThreatHandler::AddFixate(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable || !IsValid(Target)
		|| !Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) || !IsValid(Source)
		|| Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	UThreatHandler* GeneratorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorThreat) || !GeneratorThreat->CanBeInThreatTable())
	{
		return;
	}
	bool bAdded = false;
	const int32 TargetIndex = FindOrAddToThreatTable(GeneratorThreat, bAdded);
	if (TargetIndex == -1)
	{
		//Failed to add to threat table.
		return;
	}
	if (ThreatTable[TargetIndex].Fixates.Contains(Source))
	{
		return;
	}
	ThreatTable[TargetIndex].Fixates.Add(Source);
	SortModifiedThreatTarget(TargetIndex);
}

void UThreatHandler::RemoveFixate(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source))
	{
		return;
	}
	const int32 TargetIndex = FindInThreatTable(Target);
	if (TargetIndex == -1)
	{
		return;
	}
	const int32 Removed = ThreatTable[TargetIndex].Fixates.Remove(Source);
	if (Removed > 0)
	{
		SortModifiedThreatTarget(TargetIndex);
	}
}

void UThreatHandler::AddBlind(const AActor* Target, UBuff* Source)
{
	//TODO: How to handle Blind added for target not yet in threat table?
	//Currently a blind will just enter combat, which isn't really intuitive.
	if (GetOwnerRole() != ROLE_Authority || !bHasThreatTable ||
		!IsValid(Target) || !Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) || !IsValid(Source))
	{
		return;
	}
	UThreatHandler* GeneratorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorThreat) || !GeneratorThreat->CanBeInThreatTable())
	{
		return;
	}
	bool bAdded = false;
	const int32 TargetIndex = FindOrAddToThreatTable(GeneratorThreat, bAdded);
	if (TargetIndex == -1)
	{
		return;
	}
	if (ThreatTable[TargetIndex].Blinds.Contains(Source))
	{
		return;
	}
	ThreatTable[TargetIndex].Blinds.Add(Source);
	SortModifiedThreatTarget(TargetIndex);
}

void UThreatHandler::RemoveBlind(const AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source))
	{
		return;
	}
	const int32 TargetIndex = FindInThreatTable(Target);
	if (TargetIndex == -1)
	{
		return;
	}
	const int32 Removed = ThreatTable[TargetIndex].Blinds.Remove(Source);
	if (Removed > 0)
	{
		SortModifiedThreatTarget(TargetIndex);
	}
}

void UThreatHandler::AddFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
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
		if (IsValid(CombatGroup))
		{
			CombatGroup->UpdateCombatantFadeStatus(this, true);
		}
	}
}

void UThreatHandler::RemoveFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanBeInThreatTable || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	const int32 Removed = Fades.Remove(Source);
	if (Removed > 0 && Fades.Num() == 0)
	{
		if (IsValid(CombatGroup))
		{
			CombatGroup->UpdateCombatantFadeStatus(this, false);
		}
	}
}

void UThreatHandler::NotifyOfCombatantFadeStatus(const UThreatHandler* Combatant, const bool FadeStatus)
{
	const int32 TargetIndex = FindInThreatTable(Combatant);
	if (TargetIndex == -1)
	{
		return;
	}
	ThreatTable[TargetIndex].Faded = FadeStatus;
	SortModifiedThreatTarget(TargetIndex);
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
#pragma region Combat Group

void UThreatHandler::NotifyOfCombat(UCombatGroup* Group)
{
	const bool bPreviouslyInCombat = bInCombat;
	if (!IsValid(Group))
	{
		if (bPreviouslyInCombat)
		{
			CombatGroup = nullptr;
			bInCombat = false;
			CombatStartTime = 0.0f;
			ClearThreatTable();
			OnCombatChanged.Broadcast(this, false);
			NotifyLocalPlayerOfCombatChange();
		}
		return;
	}
	CombatGroup = Group;
	if (!bPreviouslyInCombat)
	{
		bInCombat = true;
		CombatStartTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		OnCombatChanged.Broadcast(this, true);
		NotifyLocalPlayerOfCombatChange();
	}
}

#pragma endregion
