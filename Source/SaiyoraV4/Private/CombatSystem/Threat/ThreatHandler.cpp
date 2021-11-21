#include "Threat/ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraBuffLibrary.h"

float UThreatHandler::GlobalHealingThreatModifier = 0.3f;
float UThreatHandler::GlobalTauntThreatPercentage = 1.2f;
float UThreatHandler::GlobalThreatDecayPercentage = 0.9f;
float UThreatHandler::GlobalThreatDecayInterval = 3.0f;

UThreatHandler::UThreatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UThreatHandler::InitializeComponent()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		FadeCallback.BindDynamic(this, &UThreatHandler::OnTargetFadeStatusChanged);
		VanishCallback.BindDynamic(this, &UThreatHandler::OnTargetVanished);
		DeathCallback.BindDynamic(this, &UThreatHandler::OnTargetDied);
		OwnerDeathCallback.BindDynamic(this, &UThreatHandler::OnOwnerDied);
		ThreatFromDamageCallback.BindDynamic(this, &UThreatHandler::OnOwnerDamageTaken);
		ThreatFromHealingCallback.BindDynamic(this, &UThreatHandler::OnTargetHealingTaken);
		ThreatBuffRestriction.BindDynamic(this, &UThreatHandler::CheckBuffForThreat);
		DecayDelegate.BindUObject(this, &UThreatHandler::DecayThreat);
	}
}

void UThreatHandler::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		BuffHandlerRef = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
		if (IsValid(BuffHandlerRef))
		{
			if (!bCanEverGenerateThreat)
			{
				BuffHandlerRef->AddOutgoingBuffRestriction(ThreatBuffRestriction);
			}
			if (!bCanEverReceiveThreat)
			{
				BuffHandlerRef->AddIncomingBuffRestriction(ThreatBuffRestriction);
			}
		}
		DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
		if (IsValid(DamageHandlerRef))
		{
			DamageHandlerRef->SubscribeToLifeStatusChanged(OwnerDeathCallback);
			DamageHandlerRef->SubscribeToIncomingDamageSuccess(ThreatFromDamageCallback);
		}
	}
}

void UThreatHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThreatHandler, CurrentTarget);
	DOREPLIFETIME(UThreatHandler, bInCombat);
}

float UThreatHandler::GetActorThreatValue(AActor* Actor) const
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Actor))
	{
		return 0.0f;
	}
	for (FThreatTarget const& Target : ThreatTable)
	{
		if (Target.Target == Actor)
		{
			return Target.Threat;
		}
	}
	return 0.0f;
}

FThreatEvent UThreatHandler::AddThreat(EThreatType const ThreatType, float const BaseThreat, AActor* AppliedBy,
                                       UObject* Source, bool const bIgnoreRestrictions, bool const bIgnoreModifiers, FThreatModCondition const& SourceModifier)
{
	FThreatEvent Result;
	
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || BaseThreat <= 0.0f || !bCanEverReceiveThreat)
	{
		return Result;
	}
	
	//Target must either be alive, or not have a health component.
	if (IsValid(DamageHandlerRef) && DamageHandlerRef->IsDead())
	{
		return Result;
	}
	
	if (!AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		return Result;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanEverGenerateThreat())
	{
		return Result;
	}
	//Generator must either be alive, or not have a health component.
	UDamageHandler* GeneratorHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(AppliedBy);
	if (IsValid(GeneratorHealth) && GeneratorHealth->IsDead())
	{
		return Result;
	}

	if (IsValid(GeneratorComponent->GetMisdirectTarget()))
	{
		//If we are misdirected to a different actor, that actor must also be alive or not have a health component.
		UDamageHandler* MisdirectHealth = ISaiyoraCombatInterface::Execute_GetDamageHandler(GeneratorComponent->GetMisdirectTarget());
		if (IsValid(MisdirectHealth) && MisdirectHealth->IsDead())
		{
			//If the misdirected actor is dead, threat will come from the original generator.
			Result.AppliedBy = AppliedBy;
		}
		else
		{
			Result.AppliedBy = GeneratorComponent->GetMisdirectTarget();
		}
	}
	else
	{
		Result.AppliedBy = AppliedBy;
	}

	Result.AppliedTo = GetOwner();
	Result.Source = Source;
	Result.ThreatType = ThreatType;
	Result.AppliedByPlane = USaiyoraCombatLibrary::GetActorPlane(Result.AppliedBy);
	Result.AppliedToPlane = USaiyoraCombatLibrary::GetActorPlane(Result.AppliedTo);
	Result.AppliedXPlane = USaiyoraCombatLibrary::CheckForXPlane(Result.AppliedByPlane, Result.AppliedToPlane);
	Result.Threat = BaseThreat;

	if (!bIgnoreModifiers)
	{
		TArray<FCombatModifier> Mods;
		GeneratorComponent->GetModifiedOutgoingThreat(Result);
		GetModifiedIncomingThreat(Result,);
		if (SourceModifier.IsBound())
		{
			Mods.Add(SourceModifier.Execute(Result));
		}
		Result.Threat = FCombatModifier::ApplyModifiers(Mods, Result.Threat);
		if (Result.Threat <= 0.0f)
		{
			return Result;
		}
	}

	if (!bIgnoreRestrictions)
	{
		if (GeneratorComponent->CheckOutgoingThreatRestricted(Result) || CheckIncomingThreatRestricted(Result))
		{
			return Result;
		}
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
		AddToThreatTable(Result.AppliedBy, Result.Threat, GeneratorComponent->HasActiveFade());
	}
	Result.bSuccess = true;

	return Result;
}

#pragma region Subscriptions

void UThreatHandler::SubscribeToTargetChanged(FTargetCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnTargetChanged.AddUnique(Callback);
	}
}

void UThreatHandler::UnsubscribeFromTargetChanged(FTargetCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnTargetChanged.Remove(Callback);
	}
}

#pragma endregion
#pragma region Restrictions

void UThreatHandler::AddIncomingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingThreatRestrictions.AddUnique(Restriction);
	}
}

void UThreatHandler::RemoveIncomingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		IncomingThreatRestrictions.Remove(Restriction);
	}
}

bool UThreatHandler::CheckIncomingThreatRestricted(FThreatEvent const& Event)
{
	for (FThreatRestriction const& Restriction : IncomingThreatRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(Event))
		{
			return true;
		}
	}
	return false;
}

void UThreatHandler::AddOutgoingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingThreatRestrictions.AddUnique(Restriction);
	}
}

void UThreatHandler::RemoveOutgoingThreatRestriction(FThreatRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		OutgoingThreatRestrictions.Remove(Restriction);
	}
}

bool UThreatHandler::CheckOutgoingThreatRestricted(FThreatEvent const& Event)
{
	for (FThreatRestriction const& Restriction : OutgoingThreatRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(Event))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion
#pragma region Modifiers

void UThreatHandler::AddIncomingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		IncomingThreatMods.AddUnique(Modifier);
	}
}

void UThreatHandler::RemoveIncomingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		IncomingThreatMods.Remove(Modifier);
	}
}

float UThreatHandler::GetModifiedIncomingThreat(FThreatEvent const& ThreatEvent, FThreatModCondition const& SourceModifier) const
{
	TArray<FCombatModifier> Mods;
	for (FThreatModCondition const& Modifier : IncomingThreatMods)
	{
		if (Modifier.IsBound())
		{
			Mods.Add(Modifier.Execute(ThreatEvent));
		}
	}
	if (SourceModifier.IsBound())
	{
		Mods.Add(SourceModifier.Execute(ThreatEvent));
	}
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

void UThreatHandler::AddOutgoingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		OutgoingThreatMods.AddUnique(Modifier);
	}
}

void UThreatHandler::RemoveOutgoingThreatModifier(FThreatModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		OutgoingThreatMods.Remove(Modifier);
	}
}

float UThreatHandler::GetModifiedOutgoingThreat(FThreatEvent const& ThreatEvent) const
{
	TArray<FCombatModifier> Mods;
	for (FThreatModCondition const& Modifier : OutgoingThreatMods)
	{
		if (Modifier.IsBound())
		{
			Mods.Add(Modifier.Execute(ThreatEvent));
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, ThreatEvent.Threat);
}

#pragma endregion 

void UThreatHandler::OnOwnerDied(AActor* Actor, ELifeStatus Previous, ELifeStatus New)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Actor) || Actor != GetOwner() || New != ELifeStatus::Dead)
	{
		return;
	}
	for (int i = ThreatTable.Num() - 1; i > 0; i--)
	{
		if (IsValid(ThreatTable[i].Target))
		{
			RemoveFromThreatTable(ThreatTable[i].Target);
		}
	}
}

void UThreatHandler::OnOwnerDamageTaken(FDamagingEvent const& DamageEvent)
{
	if (!DamageEvent.Result.Success || !DamageEvent.ThreatInfo.GeneratesThreat)
	{
		return;
	}
	AddThreat(EThreatType::Damage, DamageEvent.ThreatInfo.BaseThreat, DamageEvent.Info.AppliedBy,
		DamageEvent.Info.Source, DamageEvent.ThreatInfo.IgnoreRestrictions, DamageEvent.ThreatInfo.IgnoreModifiers,
		DamageEvent.ThreatInfo.SourceModifier);
}

void UThreatHandler::OnTargetHealingTaken(FDamagingEvent const& HealingEvent)
{
	if (!HealingEvent.Result.Success || !HealingEvent.ThreatInfo.GeneratesThreat)
	{
		return;
	}
	AddThreat(EThreatType::Healing, (HealingEvent.ThreatInfo.BaseThreat * GlobalHealingThreatModifier), HealingEvent.Info.AppliedBy,
		HealingEvent.Info.Source, HealingEvent.ThreatInfo.IgnoreRestrictions, HealingEvent.ThreatInfo.IgnoreModifiers,
		HealingEvent.ThreatInfo.SourceModifier);
}

void UThreatHandler::OnTargetVanished(AActor* Actor)
{
	if (IsValid(Actor))
	{
		RemoveFromThreatTable(Actor);
	}
}

void UThreatHandler::DecayThreat()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (FThreatTarget& Target : ThreatTable)
		{
			Target.Threat *= GlobalThreatDecayPercentage;
		}
	}
}

void UThreatHandler::NotifyAddedToThreatTable(AActor* Actor)
{
	if (TargetedBy.Contains(Actor))
	{
		return;
	}
	TargetedBy.Add(Actor);
	if (TargetedBy.Num() == 1)
	{
		UpdateCombatStatus();
	}
}

void UThreatHandler::NotifyRemovedFromThreatTable(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}
	if (TargetedBy.Remove(Actor) > 0 && TargetedBy.Num() == 0)
	{
		UpdateCombatStatus();
	}
}

float UThreatHandler::GetThreatLevel(AActor* Target) const
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target))
	{
		return 0.0f;
	}
	for (FThreatTarget const& ThreatEntry : ThreatTable)
	{
		if (IsValid(ThreatEntry.Target) && ThreatEntry.Target == Target)
		{
			return ThreatEntry.Threat;
		}
	}
	return 0.0f;
}

void UThreatHandler::RemoveThreat(float const Amount, AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !bCanEverReceiveThreat)
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

void UThreatHandler::UpdateCombatStatus()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (bInCombat)
	{
		if (ThreatTable.Num() == 0 && TargetedBy.Num() == 0)
		{
			bInCombat = false;
			OnCombatChanged.Broadcast(false);
			GetWorld()->GetTimerManager().ClearTimer(DecayHandle);
		}
	}
	else
	{
		if (ThreatTable.Num() > 0 || TargetedBy.Num() > 0)
		{
			bInCombat = true;
			OnCombatChanged.Broadcast(true);
			GetWorld()->GetTimerManager().SetTimer(DecayHandle, DecayDelegate, GlobalThreatDecayInterval, true);
		}
	}
}

void UThreatHandler::OnRep_bInCombat()
{
	OnCombatChanged.Broadcast(bInCombat);
}

void UThreatHandler::AddToThreatTable(AActor* Actor, float const InitialThreat, bool const Faded, UBuff* InitialFixate,
	UBuff* InitialBlind)
{
	if (!IsValid(Actor) || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (ThreatTable.Num() == 0)
	{
		ThreatTable.Add(FThreatTarget(Actor, InitialThreat, Faded, InitialFixate, InitialBlind));
		UpdateTarget();
		UpdateCombatStatus();
	}
	else
	{
		for (FThreatTarget const& Target : ThreatTable)
		{
			if (Target.Target == Actor)
			{
				return;
			}
		}
		FThreatTarget const NewTarget = FThreatTarget(Actor, InitialThreat, Faded, InitialFixate, InitialBlind);
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
	UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
	if (IsValid(TargetThreatHandler))
	{
		TargetThreatHandler->SubscribeToFadeStatusChanged(FadeCallback);
		TargetThreatHandler->SubscribeToVanished(VanishCallback);
		TargetThreatHandler->NotifyAddedToThreatTable(GetOwner());
	}
	UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Actor);
	if (IsValid(TargetDamageHandler))
	{	
		TargetDamageHandler->SubscribeToLifeStatusChanged(DeathCallback);
		TargetDamageHandler->SubscribeToIncomingHealingSuccess(ThreatFromHealingCallback);
	}
}

void UThreatHandler::RemoveFromThreatTable(AActor* Actor)
{
	if (!IsValid(Actor) || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	bool bAffectedTarget = false;
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Actor)
		{
			UThreatHandler* TargetThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Actor);
			if (IsValid(TargetThreatHandler))
			{
				TargetThreatHandler->UnsubscribeFromFadeStatusChanged(FadeCallback);
				TargetThreatHandler->UnsubscribeFromVanished(VanishCallback);
				TargetThreatHandler->NotifyRemovedFromThreatTable(GetOwner());
			}
			UDamageHandler* TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Actor);
			if (IsValid(TargetDamageHandler))
			{	
				TargetDamageHandler->UnsubscribeFromLifeStatusChanged(DeathCallback);
				TargetDamageHandler->UnsubscribeFromIncomingHealingSuccess(ThreatFromHealingCallback);
			}
			if (i == ThreatTable.Num() - 1)
			{
				bAffectedTarget = true;
			}
			ThreatTable.RemoveAt(i);
			break;
		}
	}
	if (bAffectedTarget)
	{
		UpdateTarget();
	}
	if (ThreatTable.Num() == 0)
	{
		UpdateCombatStatus();
	}
}

void UThreatHandler::OnTargetDied(AActor* Actor, ELifeStatus Previous, ELifeStatus New)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Actor) || New != ELifeStatus::Dead)
	{
		return;
	}
	RemoveFromThreatTable(Actor);
}

void UThreatHandler::SubscribeToFadeStatusChanged(FFadeCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnFadeStatusChanged.AddUnique(Callback);
}

void UThreatHandler::UnsubscribeFromFadeStatusChanged(FFadeCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnFadeStatusChanged.Remove(Callback);
}

void UThreatHandler::AddMisdirect(UBuff* Source, AActor* Target)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Target))
	{
		return;
	}
	for (FMisdirect const& Misdirect : Misdirects)
	{
		if (Misdirect.Source == Source)
		{
			return;
		}
	}
	Misdirects.Add(FMisdirect(Source, Target));
}

void UThreatHandler::RemoveMisdirect(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source))
	{
		return;
	}
	for (FMisdirect& Misdirect : Misdirects)
	{
		if (Misdirect.Source == Source)
		{
			Misdirects.Remove(Misdirect);
			return;
		}
	}
}

void UThreatHandler::SubscribeToVanished(FVanishCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnVanished.AddUnique(Callback);
}

void UThreatHandler::UnsubscribeFromVanished(FVanishCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnVanished.Remove(Callback);
}

void UThreatHandler::Taunt(AActor* AppliedBy)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AppliedBy) || !bCanEverReceiveThreat)
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(AppliedBy);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanEverGenerateThreat())
	{
		return;
	}
	float const InitialThreat = GetThreatLevel(AppliedBy);
	float HighestThreat = 0.0f;
	for (FThreatTarget const& Target : ThreatTable)
	{
		if (Target.Threat > HighestThreat)
		{
			HighestThreat = Target.Threat;
		}
	}
	HighestThreat *= GlobalTauntThreatPercentage;
	AddThreat(EThreatType::Absolute, FMath::Max(0.0f, HighestThreat - InitialThreat), AppliedBy, nullptr, true, true, FThreatModCondition());
}

void UThreatHandler::DropThreat(AActor* Target, float const Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !bCanEverReceiveThreat)
	{
		return;
	}
	float const DropThreat = GetThreatLevel(Target) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (DropThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(DropThreat, Target);
}

void UThreatHandler::OnTargetFadeStatusChanged(AActor* Actor, bool const FadeStatus)
{
	if (!IsValid(Actor))
	{
		return;
	}
	for (int i = 0; i < ThreatTable.Num(); i++)
	{
		if (ThreatTable[i].Target == Actor)
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

bool UThreatHandler::CheckBuffForThreat(FBuffApplyEvent const& BuffEvent)
{
	if (!IsValid(BuffEvent.BuffClass))
	{
		return false;
	}
	UBuff* DefaultBuff = BuffEvent.BuffClass.GetDefaultObject();
	if (!IsValid(DefaultBuff))
	{
		return false;
	}
	FGameplayTagContainer BuffTags;	
	DefaultBuff->GetBuffTags(BuffTags);
	return BuffTags.HasTag(GenericThreatTag());
}

void UThreatHandler::UpdateTarget()
{
	AActor* Previous = CurrentTarget;
	if (ThreatTable.Num() == 0)
	{
		CurrentTarget = nullptr;
		if (IsValid(Previous))
		{
			OnTargetChanged.Broadcast(Previous, CurrentTarget);
		}
		return;
	}
	CurrentTarget = ThreatTable[ThreatTable.Num() - 1].Target;
	if (CurrentTarget != Previous)
	{
		OnTargetChanged.Broadcast(Previous, CurrentTarget);
	}
}

void UThreatHandler::OnRep_CurrentTarget(AActor* PreviousTarget)
{
	OnTargetChanged.Broadcast(PreviousTarget, CurrentTarget);
}

void UThreatHandler::AddFixate(AActor* Target, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanEverGenerateThreat())
	{
		return;
	}
	UDamageHandler* HealthComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(Target);
	if (IsValid(HealthComponent) && HealthComponent->IsDead())
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
			int32 const PreviousFixates = ThreatTable[i].Fixates.Num();
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
		AddToThreatTable(Target, 0.0f, GeneratorComponent->HasActiveFade(), Source);
	}
}

void UThreatHandler::RemoveFixate(AActor* Target, UBuff* Source)
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
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Target) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	UThreatHandler* GeneratorComponent = ISaiyoraCombatInterface::Execute_GetThreatHandler(Target);
	if (!IsValid(GeneratorComponent) || !GeneratorComponent->CanEverGenerateThreat())
	{
		return;
	}
	UDamageHandler* HealthComponent = ISaiyoraCombatInterface::Execute_GetDamageHandler(Target);
	if (IsValid(HealthComponent) && HealthComponent->IsDead())
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
			int32 const PreviousBlinds = ThreatTable[i].Blinds.Num();
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
		AddToThreatTable(Target, 0.0f, GeneratorComponent->HasActiveFade(), nullptr, Source);
	}
}

void UThreatHandler::RemoveBlind(AActor* Target, UBuff* Source)
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
	if (GetOwnerRole() != ROLE_Authority || !bCanEverReceiveThreat || !IsValid(Source))
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
		OnFadeStatusChanged.Broadcast(GetOwner(), true);
	}
}

void UThreatHandler::RemoveFade(UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || Fades.Num() == 0)
	{
		return;
	}
	int32 const Removed = Fades.Remove(Source);
	if (Removed != 0 && Fades.Num() == 0)
	{
		OnFadeStatusChanged.Broadcast(GetOwner(), true);
	}
}

void UThreatHandler::Vanish()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	OnVanished.Broadcast(GetOwner());
	//This only clears us from actors that have us in their threat table. This does NOT clear our threat table. This shouldn't matter, as if an NPC vanishes its unlikely the player will care.
	//This only has ramifications if NPCs can possibly attack each other.
}

void UThreatHandler::TransferThreat(AActor* FromActor, AActor* ToActor, float const Percentage)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(FromActor) || !IsValid(ToActor))
	{
		return;
	}
	float const TransferThreat = GetThreatLevel(FromActor) * FMath::Clamp(Percentage, 0.0f, 1.0f);
	if (TransferThreat <= 0.0f)
	{
		return;
	}
	RemoveThreat(TransferThreat, FromActor);
	AddThreat(EThreatType::Absolute, TransferThreat, ToActor, nullptr, true, true, FThreatModCondition());
}