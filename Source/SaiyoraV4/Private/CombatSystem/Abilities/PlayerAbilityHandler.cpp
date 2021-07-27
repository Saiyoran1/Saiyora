// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerAbilityHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlaneComponent.h"
#include "ResourceHandler.h"
#include "DamageHandler.h"
#include "PlayerCombatAbility.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "PlayerSpecialization.h"

int32 UPlayerAbilityHandler::ClientPredictionID = 0;
const float UPlayerAbilityHandler::MaxPingCompensation = 0.2f;
const float UPlayerAbilityHandler::AbilityQueWindowSec = 0.2f;

#pragma region SetupFunctions

UPlayerAbilityHandler::UPlayerAbilityHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UPlayerAbilityHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	ResetReplicatedLifetimeProperty(StaticClass(), UAbilityHandler::StaticClass(), FName(TEXT("CastingState")), COND_SkipOwner, OutLifetimeProps);
}

bool UPlayerAbilityHandler::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	/*bWroteSomething |= Channel->ReplicateSubobject(CurrentSpec, *Bunch, *RepFlags);*/
	return bWroteSomething;
}

void UPlayerAbilityHandler::InitializeComponent()
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	FAbilityRestriction PlaneAbilityRestriction;
	PlaneAbilityRestriction.BindDynamic(this, &UPlayerAbilityHandler::CheckForAbilityBarRestricted);
	AddAbilityRestriction(PlaneAbilityRestriction);
}

void UPlayerAbilityHandler::BeginPlay()
{
	Super::BeginPlay();

	AbilityPermission = USaiyoraCombatLibrary::GetActorNetPermission(GetOwner());
	
	if (GetOwnerRole() != ROLE_SimulatedProxy)
	{
		USaiyoraPlaneComponent* PlaneComponent = ISaiyoraCombatInterface::Execute_GetPlaneComponent(GetOwner());
		if (IsValid(PlaneComponent))
		{
			switch (PlaneComponent->GetCurrentPlane())
			{
				case ESaiyoraPlane::None :
					break;
				case ESaiyoraPlane::Neither :
					break;
				case ESaiyoraPlane::Both :
					break;
				case ESaiyoraPlane::Ancient :
					CurrentBar = EActionBarType::Ancient;
					break;
				case ESaiyoraPlane::Modern :
					CurrentBar = EActionBarType::Modern;
					break;
				default :
					break;
			}
			FPlaneSwapCallback PlaneSwapCallback;
			PlaneSwapCallback.BindDynamic(this, &UPlayerAbilityHandler::SwapBarOnPlaneSwap);
			PlaneComponent->SubscribeToPlaneSwap(PlaneSwapCallback);
		}
	}
}

int32 UPlayerAbilityHandler::GenerateNewPredictionID()
{
	ClientPredictionID++;
	if (ClientPredictionID == 0)
	{
		ClientPredictionID++;
	}
	return ClientPredictionID;
}

#pragma endregion
#pragma region Ability Management

UPlayerCombatAbility* UPlayerAbilityHandler::FindActiveAbility(
	TSubclassOf<UPlayerCombatAbility> const AbilityClass)
{
	return Cast<UPlayerCombatAbility>(Super::FindActiveAbility(AbilityClass));
}

UCombatAbility* UPlayerAbilityHandler::AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass) || !Spellbook.Contains(AbilityClass))
	{
		return nullptr;
	}
	return Super::AddNewAbility(AbilityClass);
}

/*void UPlayerAbilityHandler::LearnAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass) || Spellbook.Contains(AbilityClass))
	{
		return;
	}
	Spellbook.Add(AbilityClass);
	OnSpellbookUpdated.Broadcast();
}

void UPlayerAbilityHandler::UnlearnAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass) || !Spellbook.Contains(AbilityClass))
	{
		return;
	}
	if (Spellbook.Remove(AbilityClass) != 0)
	{
		OnSpellbookUpdated.Broadcast();
		RemoveAbility(AbilityClass);
	}
}

void UPlayerAbilityHandler::UpdateAbilityBind(TSubclassOf<UCombatAbility> const Ability, int32 const Bind,
                                              EActionBarType const Bar)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!IsValid(Ability))
	{
		return;
	}
	switch (Bar)
	{
		case EActionBarType::Ancient :
			CurrentLoadout.AncientLoadout.Add(Bind, Ability);
			break;
		case EActionBarType::Modern :
			CurrentLoadout.ModernLoadout.Add(Bind, Ability);
			break;
		case EActionBarType::Hidden :
			CurrentLoadout.HiddenLoadout.Add(Bind, Ability);
			break;
		default :
			return;
	}
	OnAbilityBindUpdated.Broadcast(Bind, Bar, FindActiveAbility(Ability));
}*/

void UPlayerAbilityHandler::SubscribeToAbilityBindUpdated(FAbilityBindingCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnAbilityBindUpdated.AddUnique(Callback);
}

void UPlayerAbilityHandler::UnsubscribeFromAbilityBindUpdated(FAbilityBindingCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnAbilityBindUpdated.Remove(Callback);
}

void UPlayerAbilityHandler::SubscribeToBarSwap(FBarSwapCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnBarSwap.AddUnique(Callback);
}

void UPlayerAbilityHandler::UnsubscribeFromBarSwap(FBarSwapCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnBarSwap.Remove(Callback);
}

void UPlayerAbilityHandler::SubscribeToSpellbookUpdated(FSpellbookCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnSpellbookUpdated.AddUnique(Callback);
}

void UPlayerAbilityHandler::UnsubscribeFromSpellbookUpdated(FSpellbookCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnSpellbookUpdated.Remove(Callback);
}

void UPlayerAbilityHandler::SetupInitialAbilities()
{
	//TODO: Check loadaout for open spots, fill them with abilities from spellbook?
	return;
}

void UPlayerAbilityHandler::SwapBarOnPlaneSwap(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source)
{
	if (NewPlane == ESaiyoraPlane::Ancient || NewPlane == ESaiyoraPlane::Modern)
	{
		EActionBarType const PreviousBar = CurrentBar;
		CurrentBar = NewPlane == ESaiyoraPlane::Ancient ? EActionBarType::Ancient : EActionBarType::Modern;
		if (PreviousBar != CurrentBar)
		{
			OnBarSwap.Broadcast(CurrentBar);
		}
	}
}

bool UPlayerAbilityHandler::CheckForAbilityBarRestricted(UCombatAbility* Ability)
{
	if (!IsValid(Ability))
	{
		return false;
	}
	switch (Ability->GetAbilityPlane())
	{
	case ESaiyoraPlane::None :
		return false;
	case ESaiyoraPlane::Neither :
		return false;
	case ESaiyoraPlane::Both :
		return false;
	case ESaiyoraPlane::Ancient :
		return CurrentBar != EActionBarType::Ancient;
	case ESaiyoraPlane::Modern :
		return CurrentBar != EActionBarType::Modern;
	default :
		return false;
	}
}

void UPlayerAbilityHandler::RemoveAllAbilities()
{
	for (UCombatAbility* Ability : GetActiveAbilities())
	{
		RemoveAbility(Ability->GetClass());
	}
}

#pragma endregion
#pragma region AbilityUsage

/*FCastEvent UPlayerAbilityHandler::AbilityInput(int32 const BindNumber, bool const bHidden)
{
	FCastEvent Failure;
	if (BindNumber > AbilitiesPerBar - 1)
	{
		Failure.FailReason = ECastFailReason::InvalidAbility;
		return Failure;
	}
	TSubclassOf<UCombatAbility> AbilityClass;
	if (bHidden)
	{
		AbilityClass = CurrentLoadout.HiddenLoadout.FindRef(BindNumber);
	}
	else
	{
		switch (CurrentBar)
		{
		case EActionBarType::Ancient :
			AbilityClass = CurrentLoadout.AncientLoadout.FindRef(BindNumber);
			break;
		case EActionBarType::Modern :
			AbilityClass = CurrentLoadout.ModernLoadout.FindRef(BindNumber);
			break;
		default :
			break;
		}
	}
	if (IsValid(AbilityClass))
	{
		return UseAbility(AbilityClass);
	}
	Failure.FailReason = ECastFailReason::InvalidAbility;
	return Failure;
}*/

FCastEvent UPlayerAbilityHandler::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (AbilityPermission != EActorNetPermission::ListenServer && AbilityPermission != EActorNetPermission::PredictPlayer)
	{
		FCastEvent Failure;
		Failure.FailReason = ECastFailReason::NetRole;
		return Failure;
	}
	if (!AbilityClass->IsChildOf(UPlayerCombatAbility::StaticClass()))
	{
		FCastEvent Failure;
		Failure.FailReason = ECastFailReason::InvalidAbility;
		return Failure;
	}
	TSubclassOf<UPlayerCombatAbility> const PlayerAbilityClass = AbilityClass.Get();
	return UsePlayerAbility(PlayerAbilityClass);
}

FCastEvent UPlayerAbilityHandler::UsePlayerAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass, bool const bFromQueue)
{
	ClearQueue();
	FCastEvent Result;

	UPlayerCombatAbility* PlayerAbility = FindActiveAbility(AbilityClass);
	Result.Ability = PlayerAbility;
	if (!IsValid(PlayerAbility))
	{
		Result.FailReason = ECastFailReason::InvalidAbility;
		return Result;
	}

	TArray<FAbilityCost> Costs;
	if (!CheckCanUseAbility(Result.Ability, Costs, Result.FailReason))
	{
		if (!bFromQueue && Result.FailReason == ECastFailReason::AlreadyCasting || Result.FailReason == ECastFailReason::OnGlobalCooldown)
		{
			if (TryQueueAbility(AbilityClass))
			{
				Result.FailReason = ECastFailReason::Queued;
			}
		}
		return Result;
	}
	//Prediction of ability usage from an autonomous proxy.
	if (AbilityPermission == EActorNetPermission::PredictPlayer)
	{
		Result.PredictionID = GenerateNewPredictionID();

		FAbilityRequest AbilityRequest;
		AbilityRequest.AbilityClass = AbilityClass;
		AbilityRequest.PredictionID = Result.PredictionID;
		AbilityRequest.ClientStartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
		
		FClientAbilityPrediction AbilityPrediction;
		AbilityPrediction.Ability = PlayerAbility;
		AbilityPrediction.PredictionID = Result.PredictionID;
		AbilityPrediction.ClientTime = GetGameStateRef()->GetServerWorldTimeSeconds();

		if (PlayerAbility->GetHasGlobalCooldown())
		{
			PredictStartGlobal(Result.PredictionID);
			AbilityPrediction.bPredictedGCD = true;
		}

		PlayerAbility->PredictCommitCharges(Result.PredictionID);
		AbilityPrediction.bPredictedCharges = true;

		if (Costs.Num() > 0 && IsValid(GetResourceHandlerRef()))
		{
			GetResourceHandlerRef()->CommitAbilityCosts(Result.Ability, Costs, Result.PredictionID);
			for (FAbilityCost const& Cost : Costs)
			{
				AbilityPrediction.PredictedCostClasses.Add(Cost.ResourceClass);
			}
		}
		
		switch (Result.Ability->GetCastType())
		{
		case EAbilityCastType::None :
			Result.FailReason = ECastFailReason::InvalidCastType;
			return Result;
		case EAbilityCastType::Instant :
			Result.ActionTaken = ECastAction::Success;
			PlayerAbility->PredictedTick(0, AbilityRequest.PredictionParams);
			OnAbilityTick.Broadcast(Result);
			break;
		case EAbilityCastType::Channel :
			Result.ActionTaken = ECastAction::Success;
			PredictStartCast(Result.Ability, Result.PredictionID);
			AbilityPrediction.bPredictedCastBar = true;
			if (Result.Ability->GetHasInitialTick())
			{
				PlayerAbility->PredictedTick(0, AbilityRequest.PredictionParams);
				OnAbilityTick.Broadcast(Result);
			}
			break;
		default :
			Result.FailReason = ECastFailReason::InvalidCastType;
			return Result;
		}
		
		UnackedAbilityPredictions.Add(AbilityPrediction.PredictionID, AbilityPrediction);
		ServerPredictAbility(AbilityRequest);
		
		return Result;
	}
	
	if (PlayerAbility->GetHasGlobalCooldown())
	{
		StartGlobal(PlayerAbility);
	}
	
	if (Costs.Num() > 0 && IsValid(GetResourceHandlerRef()))
	{
		GetResourceHandlerRef()->CommitAbilityCosts(Result.Ability, Costs, Result.PredictionID);
	}

	FCombatParameters PredictionParams;
	FCombatParameters BroadcastParams;
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = ECastFailReason::InvalidCastType;
		return Result;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		PlayerAbility->PredictedTick(0, PredictionParams);
		PlayerAbility->ServerTick(0, PredictionParams, BroadcastParams);
		OnAbilityTick.Broadcast(Result);
		MulticastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		StartCast(PlayerAbility, Result.PredictionID);
		if (Result.Ability->GetHasInitialTick())
		{
			PlayerAbility->PredictedTick(0, PredictionParams);
			PlayerAbility->ServerTick(0, PredictionParams, BroadcastParams);
			OnAbilityTick.Broadcast(Result);
			MulticastAbilityTick(Result, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = ECastFailReason::InvalidCastType;
		return Result;
	}
	
	return Result;
}

void UPlayerAbilityHandler::ServerPredictAbility_Implementation(FAbilityRequest const& AbilityRequest)
{
	FCastEvent Result;
	Result.PredictionID = AbilityRequest.PredictionID;

	UPlayerCombatAbility* PlayerAbility = FindActiveAbility(AbilityRequest.AbilityClass);
	Result.Ability = PlayerAbility;
	if (!IsValid(PlayerAbility))
	{
		Result.FailReason = ECastFailReason::InvalidAbility;
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	TArray<FAbilityCost> Costs;
	CheckCanUseAbility(PlayerAbility, Costs, Result.FailReason);

	FServerAbilityResult ServerResult;
	ServerResult.AbilityClass = AbilityRequest.AbilityClass;
	ServerResult.PredictionID = AbilityRequest.PredictionID;
	//TODO: This should probably be manually calculated using ping value and server time, instead of trusting the client.
	ServerResult.ClientStartTime = AbilityRequest.ClientStartTime;

	if (PlayerAbility->GetHasGlobalCooldown())
	{
		StartGlobal(PlayerAbility, true);
		ServerResult.bActivatedGlobal = true;
		ServerResult.GlobalLength = CalculateGlobalCooldownLength(PlayerAbility);
	}

	int32 const PreviousCharges = PlayerAbility->GetCurrentCharges();
	PlayerAbility->CommitCharges(Result.PredictionID);
	int32 const NewCharges = PlayerAbility->GetCurrentCharges();
	ServerResult.ChargesSpent = PreviousCharges - NewCharges;

	if (Costs.Num() > 0 && IsValid(GetResourceHandlerRef()))
	{
		GetResourceHandlerRef()->CommitAbilityCosts(PlayerAbility, Costs, Result.PredictionID);
		ServerResult.AbilityCosts = Costs;
	}

	FCombatParameters BroadcastParams;
	
	switch (PlayerAbility->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = ECastFailReason::InvalidCastType;
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		PlayerAbility->ServerTick(0, AbilityRequest.PredictionParams, BroadcastParams);
		OnAbilityTick.Broadcast(Result);
		MulticastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		ServerResult.CastLength = CalculateCastLength(PlayerAbility);
		StartCast(PlayerAbility, Result.PredictionID);
		ServerResult.bActivatedCastBar = true;
		ServerResult.bInterruptible = CastingState.bInterruptible;
		if (PlayerAbility->GetHasInitialTick())
		{
			PlayerAbility->ServerTick(0, AbilityRequest.PredictionParams, BroadcastParams);
			OnAbilityTick.Broadcast(Result);
			MulticastAbilityTick(Result, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = ECastFailReason::InvalidCastType;
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	ClientSucceedPredictedAbility(ServerResult);
}

void UPlayerAbilityHandler::ClientSucceedPredictedAbility_Implementation(FServerAbilityResult const& ServerResult)
{
	if (!IsValid(ServerResult.AbilityClass) || ServerResult.PredictionID == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server sent invalid ability confirmation."));
		return;
	}

	UpdatePredictedCastFromServer(ServerResult);
	UpdatePredictedGlobalFromServer(ServerResult);
	TArray<TSubclassOf<UResource>> MispredictedCosts;
	UPlayerCombatAbility* Ability;
	
	FClientAbilityPrediction* OriginalPrediction = UnackedAbilityPredictions.Find(ServerResult.PredictionID);
	if (OriginalPrediction != nullptr)
	{
		MispredictedCosts = OriginalPrediction->PredictedCostClasses;
		for (FAbilityCost const& Cost : ServerResult.AbilityCosts)
		{
			MispredictedCosts.Remove(Cost.ResourceClass);
		}
		Ability = OriginalPrediction->Ability;
		UnackedAbilityPredictions.Remove(ServerResult.PredictionID);
	}
	else
	{
		Ability = FindActiveAbility(ServerResult.AbilityClass);
	}
	if (IsValid(Ability))
	{
		Ability->UpdatePredictedChargesFromServer(ServerResult.PredictionID, ServerResult.ChargesSpent);
	}
	if (IsValid(GetResourceHandlerRef()))
	{	
		GetResourceHandlerRef()->UpdatePredictedCostsFromServer(ServerResult, MispredictedCosts);
	}
}

void UPlayerAbilityHandler::ClientFailPredictedAbility_Implementation(int32 const PredictionID, ECastFailReason const FailReason)
{
	if (GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.PredictionID <= PredictionID)
	{
		EndGlobalCooldown();
		GlobalCooldownState.PredictionID = PredictionID;
	}
	if (CastingState.bIsCasting && CastingState.PredictionID <= PredictionID)
	{
		EndCast();
		CastingState.PredictionID = PredictionID;
	}
	FClientAbilityPrediction* OriginalPrediction = UnackedAbilityPredictions.Find(PredictionID);
	if (OriginalPrediction == nullptr)
	{
		//Do nothing. Replication will eventually handle things.
		return;
	}
	if (IsValid(GetResourceHandlerRef()) && !OriginalPrediction->PredictedCostClasses.Num() == 0)
	{
		GetResourceHandlerRef()->RollbackFailedCosts(OriginalPrediction->PredictedCostClasses, PredictionID);
	}
	if (OriginalPrediction->bPredictedCharges)
	{
		if (IsValid(OriginalPrediction->Ability))
		{
			OriginalPrediction->Ability->UpdatePredictedChargesFromServer(PredictionID, 0);
			OriginalPrediction->Ability->AbilityMisprediction(PredictionID, FailReason);
		}
	}
	UnackedAbilityPredictions.Remove(PredictionID);
	OnAbilityMispredicted.Broadcast(PredictionID, FailReason);
}

void UPlayerAbilityHandler::SubscribeToAbilityMispredicted(FAbilityMispredictionCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnAbilityMispredicted.AddUnique(Callback);
}

void UPlayerAbilityHandler::UnsubscribeFromAbilityMispredicted(FAbilityMispredictionCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnAbilityMispredicted.Remove(Callback);
}

#pragma endregion
#pragma region Casting

void UPlayerAbilityHandler::StartCast(UCombatAbility* Ability, int32 const PredictionID)
{
	if (PredictionID == 0)
	{
		Super::StartCast(Ability);
		return;
	}
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CastStartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	float CastLength = CalculateCastLength(Ability);
	CastingState.PredictionID = PredictionID;
	float const PingCompensation = FMath::Clamp(USaiyoraCombatLibrary::GetActorPing(GetOwner()), 0.0f, MaxPingCompensation);
	CastLength = FMath::Max(MinimumCastLength, CastLength - PingCompensation);
	CastingState.CastEndTime = CastingState.CastStartTime + CastLength;
	CastingState.bInterruptible = Ability->GetInterruptible();
	GetWorld()->GetTimerManager().SetTimer(CastHandle, this, &UPlayerAbilityHandler::CompleteCast, CastLength, false);
	GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UPlayerAbilityHandler::AuthTickPredictedCast,
		(CastLength / Ability->GetNumberOfTicks()), true);
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}


void UPlayerAbilityHandler::PredictStartCast(UCombatAbility* Ability, int32 const PredictionID)
{
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.PredictionID = PredictionID;
	CastingState.CastStartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	CastingState.CastEndTime = 0.0f;
	CastingState.bInterruptible = Ability->GetInterruptible();
	CastingState.ElapsedTicks = 0;
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UPlayerAbilityHandler::UpdatePredictedCastFromServer(FServerAbilityResult const& ServerResult)
{
	FCastingState const PreviousState = CastingState;
	if (CastingState.PredictionID > ServerResult.PredictionID)
	{
		//Don't override if we are already predicting a newer cast or cancel.
		return;
	}
	CastingState.PredictionID = ServerResult.PredictionID;
	if (!ServerResult.bActivatedCastBar || ServerResult.ClientStartTime + ServerResult.CastLength < GetGameStateRef()->GetServerWorldTimeSeconds())
	{
		EndCast();
		return;
	}
	CastingState.bIsCasting = true;
	CastingState.CastStartTime = ServerResult.ClientStartTime;
	CastingState.CastEndTime = CastingState.CastStartTime + ServerResult.CastLength;
	CastingState.CurrentCast = FindActiveAbility(ServerResult.AbilityClass);
	CastingState.bInterruptible = ServerResult.bInterruptible;

	//Check for any missed ticks. We will update the last missed tick (if any, this should be rare) after setting everything else.
	float const TickInterval = ServerResult.CastLength / CastingState.CurrentCast->GetNumberOfTicks();
	CastingState.ElapsedTicks = FMath::FloorToInt((GetGameStateRef()->GetServerWorldTimeSeconds() - CastingState.CastStartTime) / TickInterval);

	//Clear any cast or tick handles that existed, this shouldn't happen.
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	GetWorld()->GetTimerManager().ClearTimer(TickHandle);

	//Set new handles for predicting the cast end and predicting the next tick.
	GetWorld()->GetTimerManager().SetTimer(CastHandle, this, &UPlayerAbilityHandler::CompleteCast, CastingState.CastEndTime - GetGameStateRef()->GetServerWorldTimeSeconds(), false);
	//First iteration of the tick timer will get time remaining until the next tick (to account for travel time). Subsequent ticks use regular interval.
	GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UPlayerAbilityHandler::PredictAbilityTick, TickInterval, true,
		(CastingState.CastStartTime + (TickInterval * (CastingState.ElapsedTicks + 1))) - GetGameStateRef()->GetServerWorldTimeSeconds());

	//Immediately perform the last missed tick if one happened during the wait time between ability prediction and confirmation.
	if (CastingState.ElapsedTicks > 0)
	{
		HandleMissedPredictedTick(CastingState.ElapsedTicks);
	}
	
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UPlayerAbilityHandler::CompleteCast()
{
	FCastEvent CompletionEvent;
	CompletionEvent.Ability = CastingState.CurrentCast;
	CompletionEvent.PredictionID = CastingState.PredictionID;
	CompletionEvent.ActionTaken = ECastAction::Complete;
	CompletionEvent.Tick = CastingState.ElapsedTicks;
	if (IsValid(CompletionEvent.Ability))
	{
		CastingState.CurrentCast->CompleteCast();
		OnAbilityComplete.Broadcast(CompletionEvent.Ability);
		if (GetOwnerRole() == ROLE_Authority)
		{
			MulticastAbilityComplete(CompletionEvent);
		}
	}
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	if (!GetWorld()->GetTimerManager().IsTimerActive(TickHandle))
	{
		EndCast();
	}
}

//Predict a tick on the client and send tick parameters to the server.
void UPlayerAbilityHandler::PredictAbilityTick()
{
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		return;
	}
	UPlayerCombatAbility* CurrentPlayerCast = Cast<UPlayerCombatAbility>(CastingState.CurrentCast);
	if (!IsValid(CurrentPlayerCast))
	{
		return;
	}
	CastingState.ElapsedTicks++;
	FCombatParameters PredictionParams;
	CurrentPlayerCast->PredictedTick(CastingState.ElapsedTicks, PredictionParams);
	if (CurrentPlayerCast->GetTickNeedsPredictionParams(CastingState.ElapsedTicks))
	{
		FAbilityRequest TickRequest;
		TickRequest.Tick = CastingState.ElapsedTicks;
		TickRequest.AbilityClass = CastingState.CurrentCast->GetClass();
		TickRequest.PredictionID = CastingState.PredictionID;
		TickRequest.PredictionParams = PredictionParams;
		ServerHandlePredictedTick(TickRequest);
	}
	FCastEvent TickEvent;
	TickEvent.Ability = CastingState.CurrentCast;
	TickEvent.Tick = CastingState.ElapsedTicks;
	TickEvent.ActionTaken = ECastAction::Tick;
	TickEvent.PredictionID = CastingState.PredictionID;
	OnAbilityTick.Broadcast(TickEvent);
	if (CastingState.ElapsedTicks >= CurrentPlayerCast->GetNumberOfTicks())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		if (!GetWorld()->GetTimerManager().IsTimerActive(CastHandle))
		{
			EndCast();
		}
	}
}

//Execute a tick that was skipped on the client while waiting for the server to validate cast length and tick interval.
void UPlayerAbilityHandler::HandleMissedPredictedTick(int32 const TickNumber)
{
	if (!CastingState.bIsCasting || IsValid(CastingState.CurrentCast))
	{
		return;
	}
	UPlayerCombatAbility* CurrentPlayerCast = Cast<UPlayerCombatAbility>(CastingState.CurrentCast);
	if (!IsValid(CurrentPlayerCast))
	{
		return;
	}
	FCombatParameters PredictionParams;
	CurrentPlayerCast->PredictedTick(TickNumber, PredictionParams);
	if (CurrentPlayerCast->GetTickNeedsPredictionParams(TickNumber))
	{
		FAbilityRequest TickRequest;
		TickRequest.Tick = TickNumber;
		TickRequest.AbilityClass = CastingState.CurrentCast->GetClass();
		TickRequest.PredictionID = CastingState.PredictionID;
		TickRequest.PredictionParams = PredictionParams;
		ServerHandlePredictedTick(TickRequest);
	}
	FCastEvent TickEvent;
	TickEvent.Ability = CastingState.CurrentCast;
	TickEvent.Tick = TickNumber;
	TickEvent.ActionTaken = ECastAction::Tick;
	TickEvent.PredictionID = CastingState.PredictionID;
	OnAbilityTick.Broadcast(TickEvent);
}

//Receive parameters from the client and either store the parameters (if the tick has yet to happen) or check if the tick has already happened, pass the parameters, and execute the tick.
void UPlayerAbilityHandler::ServerHandlePredictedTick_Implementation(FAbilityRequest const& TickRequest)
{
	if (CastingState.bIsCasting && IsValid(CastingState.CurrentCast) && CastingState.PredictionID == TickRequest.PredictionID && CastingState.CurrentCast->GetClass() == TickRequest.AbilityClass && CastingState.ElapsedTicks < TickRequest.Tick)
	{
		FPredictedTick Tick;
		Tick.TickNumber = TickRequest.Tick;
		Tick.PredictionID = TickRequest.PredictionID;
		ParamsAwaitingTicks.Add(Tick, TickRequest.PredictionParams);
		return;
	}
	for (FPredictedTick const& Tick : TicksAwaitingParams)
	{
		if (Tick.PredictionID == TickRequest.PredictionID && Tick.TickNumber == TickRequest.Tick)
		{
			UPlayerCombatAbility* Ability = FindActiveAbility(TickRequest.AbilityClass);
			if (IsValid(Ability))
			{
				FCombatParameters BroadcastParams;
				Ability->ServerTick(TickRequest.Tick, TickRequest.PredictionParams, BroadcastParams);
				FCastEvent TickEvent;
				TickEvent.Ability = Ability;
				TickEvent.Tick = TickRequest.Tick;
				TickEvent.ActionTaken = ECastAction::Tick;
				TickEvent.PredictionID = TickRequest.PredictionID;
				OnAbilityTick.Broadcast(TickEvent);
				MulticastAbilityTick(TickEvent, BroadcastParams);
			}
			TicksAwaitingParams.RemoveSingleSwap(Tick);
			return;
		}
	}
}

//Expire any ticks awaiting parameters once the following tick happens.
void UPlayerAbilityHandler::PurgeExpiredPredictedTicks()
{
	TArray<FPredictedTick> ExpiredTicks;
	for (FPredictedTick const& WaitingTick : TicksAwaitingParams)
	{
		if (WaitingTick.PredictionID == CastingState.PredictionID && WaitingTick.TickNumber < CastingState.ElapsedTicks - 1)
		{
			ExpiredTicks.Add(WaitingTick);
		}
		else if (WaitingTick.PredictionID < CastingState.PredictionID && CastingState.ElapsedTicks >= 1)
		{
			ExpiredTicks.Add(WaitingTick);
		}
	}
	for (FPredictedTick const& ExpiredTick : ExpiredTicks)
	{
		TicksAwaitingParams.Remove(ExpiredTick);
	}
	ExpiredTicks.Empty();
	for (TTuple<FPredictedTick, FCombatParameters> const& WaitingParams : ParamsAwaitingTicks)
	{
		if (WaitingParams.Key.PredictionID == CastingState.PredictionID && WaitingParams.Key.TickNumber < CastingState.ElapsedTicks - 1)
		{
			ExpiredTicks.Add(WaitingParams.Key);
		}
		else if (WaitingParams.Key.PredictionID < CastingState.PredictionID && CastingState.ElapsedTicks >= 1)
		{
			ExpiredTicks.Add(WaitingParams.Key);
		}
	}
	for (FPredictedTick const& ExpiredTick : ExpiredTicks)
	{
		ParamsAwaitingTicks.Remove(ExpiredTick);
	}
}

//Server function called on tick timer to determine whether the tick can happen or if we need to wait on client-sent parameters.
void UPlayerAbilityHandler::AuthTickPredictedCast()
{
	if (!GetCastingState().bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		return;
	}
	UPlayerCombatAbility* CurrentPlayerCast = Cast<UPlayerCombatAbility>(CastingState.CurrentCast);
	if (!IsValid(CurrentPlayerCast))
	{
		return;
	}
	CastingState.ElapsedTicks++;
	FPredictedTick Tick;
	Tick.PredictionID = CastingState.PredictionID;
	Tick.TickNumber = CastingState.ElapsedTicks;
	FCombatParameters BroadcastParams;
	if (!CurrentPlayerCast->GetTickNeedsPredictionParams(CastingState.ElapsedTicks))
	{
		CurrentPlayerCast->UCombatAbility::ServerTick(CastingState.ElapsedTicks, BroadcastParams);
		FCastEvent TickEvent;
		TickEvent.Ability = CurrentPlayerCast;
		TickEvent.Tick = CastingState.ElapsedTicks;
		TickEvent.ActionTaken = ECastAction::Tick;
		TickEvent.PredictionID = CastingState.PredictionID;
		OnAbilityTick.Broadcast(TickEvent);
		MulticastAbilityTick(TickEvent, BroadcastParams);
	}
	else if (ParamsAwaitingTicks.Contains(Tick))
	{
		CurrentPlayerCast->ServerTick(CastingState.ElapsedTicks, ParamsAwaitingTicks.FindRef(Tick), BroadcastParams);
		FCastEvent TickEvent;
		TickEvent.Ability = CurrentPlayerCast;
		TickEvent.Tick = CastingState.ElapsedTicks;
		TickEvent.ActionTaken = ECastAction::Tick;
		TickEvent.PredictionID = CastingState.PredictionID;
		OnAbilityTick.Broadcast(TickEvent);
		MulticastAbilityTick(TickEvent, BroadcastParams);
		ParamsAwaitingTicks.Remove(Tick);
	}
	else
	{
		TicksAwaitingParams.Add(Tick);
	}
	PurgeExpiredPredictedTicks();
	if (CastingState.ElapsedTicks >= CurrentPlayerCast->GetNumberOfTicks())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		if (!GetWorld()->GetTimerManager().IsTimerActive(CastHandle))
		{
			EndCast();
		}
	}
}

#pragma endregion
#pragma region Cancel

FCancelEvent UPlayerAbilityHandler::CancelCurrentCast()
{
	if (AbilityPermission != EActorNetPermission::ListenServer && AbilityPermission != EActorNetPermission::PredictPlayer)
	{
		FCancelEvent Fail;
		Fail.FailReason = ECancelFailReason::NetRole;
		return Fail;
	}
	return PredictCancelAbility();
}

FCancelEvent UPlayerAbilityHandler::PredictCancelAbility()
{
	FCancelEvent Result;
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		Result.FailReason = ECancelFailReason::NotCasting;
		return Result;
	}
	UPlayerCombatAbility* CurrentPlayerCast = Cast<UPlayerCombatAbility>(CastingState.CurrentCast);
	if (!IsValid(CurrentPlayerCast))
	{
		Result.FailReason = ECancelFailReason::NotCasting;
		return Result;
	}
	Result.CancelledAbility = CurrentPlayerCast;
	Result.CancelTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.CancelledCastID = CastingState.PredictionID;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.bSuccess = true;
	if (AbilityPermission == EActorNetPermission::PredictPlayer)
	{
		Result.CancelID = GenerateNewPredictionID();
		FCancelRequest CancelRequest;
		CancelRequest.CancelTime = Result.CancelTime;
		CancelRequest.CancelID = Result.CancelID;
		CancelRequest.CancelledCastID = Result.CancelledCastID;
		CurrentPlayerCast->PredictedCancel(CancelRequest.PredictionParams);
		ServerPredictCancelAbility(CancelRequest);
		OnAbilityCancelled.Broadcast(Result);
		CastingState.PredictionID = Result.CancelID;
		EndCast();
		return Result;
	}
	FCombatParameters BroadcastParams;
	CurrentPlayerCast->UCombatAbility::ServerCancel(BroadcastParams);
	OnAbilityCancelled.Broadcast(Result);
	MulticastAbilityCancel(Result, BroadcastParams);
	EndCast();
	return Result;
}

void UPlayerAbilityHandler::ServerPredictCancelAbility_Implementation(FCancelRequest const& CancelRequest)
{
	int32 const CastID = CastingState.PredictionID;
	if (CancelRequest.CancelID > CastingState.PredictionID)
	{
		CastingState.PredictionID = CancelRequest.CancelID;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast) || CastID != CancelRequest.CancelledCastID)
	{
		return;
	}
	UPlayerCombatAbility* CurrentPlayerCast = Cast<UPlayerCombatAbility>(CastingState.CurrentCast);
	if (!IsValid(CurrentPlayerCast))
	{
		return;
	}
	FCancelEvent Result;
	Result.CancelledAbility = CurrentPlayerCast;
	Result.CancelTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	Result.CancelID = CancelRequest.CancelID;
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.CancelledCastID = CastingState.PredictionID;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	FCombatParameters BroadcastParams;
	CurrentPlayerCast->ServerCancel(CancelRequest.PredictionParams, BroadcastParams);
	OnAbilityCancelled.Broadcast(Result);
	MulticastAbilityCancel(Result, BroadcastParams);
	EndCast();
}

#pragma endregion
#pragma region Interrupt

FInterruptEvent UPlayerAbilityHandler::InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource,
	bool const bIgnoreRestrictions)
{
	switch (AbilityPermission)
	{
	case EActorNetPermission::ListenServer :
		return Super::InterruptCurrentCast(AppliedBy, InterruptSource, bIgnoreRestrictions);
	case EActorNetPermission::Server :
		{
			int32 const CastID = CastingState.PredictionID;
			FInterruptEvent Result = Super::InterruptCurrentCast(AppliedBy, InterruptSource, bIgnoreRestrictions);
			if (!Result.bSuccess)
			{
				return Result;
			}
			Result.CancelledCastID = CastID;
			ClientInterruptCast(Result);
		}
	default :
		{
			FInterruptEvent Failure;
			Failure.FailReason = EInterruptFailReason::NetRole;
			return Failure;
		}
	}
}

void UPlayerAbilityHandler::ClientInterruptCast_Implementation(FInterruptEvent const& InterruptEvent)
{
	if (!CastingState.bIsCasting || CastingState.PredictionID > InterruptEvent.CancelledCastID || !IsValid(CastingState.CurrentCast) || CastingState.CurrentCast != InterruptEvent.InterruptedAbility)
	{
		//We have already moved on.
		return;
	}
	CastingState.CurrentCast->InterruptCast(InterruptEvent);
	OnAbilityInterrupted.Broadcast(InterruptEvent);
	EndCast();
}
#pragma endregion
#pragma region GlobalCooldown

void UPlayerAbilityHandler::StartGlobal(UCombatAbility* Ability, bool const bPredicted)
{
	if (!bPredicted)
	{
		Super::StartGlobal(Ability);
		return;
	}
	FGlobalCooldown const PreviousGlobal = GlobalCooldownState;
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.StartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	float GlobalLength = CalculateGlobalCooldownLength(Ability);
	float const PingCompensation = FMath::Clamp(USaiyoraCombatLibrary::GetActorPing(GetOwner()), 0.0f, MaxPingCompensation);
	GlobalLength = FMath::Max(MinimumGlobalCooldownLength, GlobalLength - PingCompensation);
	GlobalCooldownState.EndTime = GlobalCooldownState.StartTime + GlobalLength;
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UPlayerAbilityHandler::EndGlobalCooldown, GlobalLength, false);
	OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
}

void UPlayerAbilityHandler::EndGlobalCooldown()
{
	if (GlobalCooldownState.bGlobalCooldownActive)
	{
		Super::EndGlobalCooldown();
		if (!GlobalCooldownState.bGlobalCooldownActive && (AbilityPermission == EActorNetPermission::ListenServer || AbilityPermission == EActorNetPermission::PredictPlayer))
		{
			CheckForQueuedAbilityOnGlobalEnd();
		}
	}
}

void UPlayerAbilityHandler::PredictStartGlobal(int32 const PredictionID)
{
	FGlobalCooldown const PreviousState = GlobalCooldownState;
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.PredictionID = PredictionID;
	GlobalCooldownState.StartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	GlobalCooldownState.EndTime = 0.0f;
	OnGlobalCooldownChanged.Broadcast(PreviousState, GlobalCooldownState);
}

void UPlayerAbilityHandler::UpdatePredictedGlobalFromServer(FServerAbilityResult const& ServerResult)
{
	FGlobalCooldown const PreviousState = GlobalCooldownState;
	if (GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.PredictionID > ServerResult.PredictionID)
	{
		//Ignore if we have predicted a newer global.
		return;
	}
	GlobalCooldownState.PredictionID = ServerResult.PredictionID;
	if (!ServerResult.bActivatedGlobal || ServerResult.ClientStartTime + ServerResult.GlobalLength < GetGameStateRef()->GetServerWorldTimeSeconds())
	{
		EndGlobalCooldown();
		return;
	}
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.StartTime = ServerResult.ClientStartTime;
	GlobalCooldownState.EndTime = GlobalCooldownState.StartTime + ServerResult.GlobalLength;
	GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UPlayerAbilityHandler::EndGlobalCooldown, GlobalCooldownState.EndTime - GetGameStateRef()->GetServerWorldTimeSeconds(), false);
	OnGlobalCooldownChanged.Broadcast(PreviousState, GlobalCooldownState);
}

#pragma endregion 
#pragma region Queueing

bool UPlayerAbilityHandler::TryQueueAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass)
{
	ClearQueue();
	if (!GlobalCooldownState.bGlobalCooldownActive && !CastingState.bIsCasting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tried to queue an ability when not casting or on global cooldown."));
		return false;
	}
	if (!IsValid(AbilityClass))
	{
		return false;
	}
	if (CastingState.bIsCasting && CastingState.CastEndTime == 0.0f)
	{
		//Don't queue an ability if the cast time hasn't been acked yet.
		return false;
	}
	if (AbilityClass->GetDefaultObject<UPlayerCombatAbility>()->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime == 0.0f)
	{
		//Don't queue an ability if the gcd time hasn't been acked yet.
		return false;
	}
	float const GlobalTimeRemaining = AbilityClass->GetDefaultObject<UPlayerCombatAbility>()->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive ? GetGlobalCooldownTimeRemaining() : 0.0f;
	float const CastTimeRemaining = CastingState.bIsCasting ? GetCastTimeRemaining() : 0.0f;
	if (GlobalTimeRemaining > AbilityQueWindowSec || CastTimeRemaining > AbilityQueWindowSec)
	{
		//Don't queue if either the gcd or cast time will last longer than the queue window.
		return false;
	}
	if (GlobalTimeRemaining == CastTimeRemaining)
	{
		QueueStatus = EQueueStatus::WaitForBoth;
		QueuedAbility = AbilityClass;
		SetQueueExpirationTimer();
	}
	else if (GlobalTimeRemaining > CastTimeRemaining)
	{
		QueueStatus = EQueueStatus::WaitForGlobal;
		QueuedAbility = AbilityClass;
		SetQueueExpirationTimer();
	}
	else
	{
		QueueStatus = EQueueStatus::WaitForCast;
		QueuedAbility = AbilityClass;
		SetQueueExpirationTimer();
	}
	return true;
}

void UPlayerAbilityHandler::ClearQueue()
{
	QueueStatus = EQueueStatus::Empty;
	QueuedAbility = nullptr;
	GetWorld()->GetTimerManager().ClearTimer(QueueClearHandle);
}

void UPlayerAbilityHandler::SetQueueExpirationTimer()
{
	GetWorld()->GetTimerManager().SetTimer(QueueClearHandle, this, &UPlayerAbilityHandler::ClearQueue, AbilityQueWindowSec, false);
}

void UPlayerAbilityHandler::CheckForQueuedAbilityOnGlobalEnd()
{
	if (GlobalCooldownState.bGlobalCooldownActive)
	{
		return;
	}
	TSubclassOf<UPlayerCombatAbility> AbilityClass;
	switch (QueueStatus)
	{
		case EQueueStatus::Empty :
			return;
		case EQueueStatus::WaitForBoth :
			QueueStatus = EQueueStatus::WaitForCast;
			return;
		case EQueueStatus::WaitForCast :
			return;
		case EQueueStatus::WaitForGlobal :
			AbilityClass = QueuedAbility;
			ClearQueue();
			if (IsValid(AbilityClass))
			{
				UsePlayerAbility(AbilityClass, true);
			}
			return;
		default :
			return;
	}
}

void UPlayerAbilityHandler::CheckForQueuedAbilityOnCastEnd()
{
	if (CastingState.bIsCasting)
	{
		return;
	}
	TSubclassOf<UPlayerCombatAbility> AbilityClass;
	switch (QueueStatus)
	{
	case EQueueStatus::Empty :
		return;
	case EQueueStatus::WaitForBoth :
		QueueStatus = EQueueStatus::WaitForGlobal;
		return;
	case EQueueStatus::WaitForCast :
		AbilityClass = QueuedAbility;
		ClearQueue();
		if (IsValid(AbilityClass))
		{
			UsePlayerAbility(AbilityClass, true);
		}
		return;
	case EQueueStatus::WaitForGlobal :
		return;
	default :
		return;
	}
}

#pragma endregion
#pragma region Specialization
/*
void UPlayerAbilityHandler::ChangeSpecialization(TSubclassOf<UPlayerSpecialization> const NewSpecialization)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	UPlayerSpecialization* const PreviousSpec = CurrentSpec;
	if (IsValid(CurrentSpec))
	{
		for (TSubclassOf<UCombatAbility> const AbilityClass : CurrentSpec->GetGrantedAbilities())
		{
			if (IsValid(AbilityClass))
			{
				UnlearnAbility(AbilityClass);
			}
		}
		CurrentSpec->UnlearnSpecObject();
		CurrentSpec = nullptr;
	}
	RemoveAllAbilities();
	if (IsValid(NewSpecialization))
	{
		CurrentSpec = NewObject<UPlayerSpecialization>(GetOwner(), NewSpecialization);
		if (IsValid(CurrentSpec))
		{
			CurrentSpec->InitializeSpecObject(this);
			for (TSubclassOf<UCombatAbility> const AbilityClass : CurrentSpec->GetGrantedAbilities())
			{
				if (IsValid(AbilityClass))
				{
					LearnAbility(AbilityClass);
				}
			}
			if (!SpecLoadouts.Contains(NewSpecialization))
			{
				FPlayerAbilityLoadout NewLoadout;
				CurrentSpec->CreateNewDefaultLoadout(NewLoadout);
				SpecLoadouts.Add(NewSpecialization, NewLoadout);
			}
			CurrentLoadout = SpecLoadouts.FindRef(NewSpecialization);
			for (TSubclassOf<UCombatAbility> const AbilityClass : CurrentLoadout.GetAllAbilities())
			{
				AddNewAbility(AbilityClass);
			}
		}
	}
	if (CurrentSpec != PreviousSpec)
	{
		OnSpecChanged.Broadcast(CurrentSpec);
	}
}

void UPlayerAbilityHandler::SubscribeToSpecChanged(FSpecializationCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnSpecChanged.AddUnique(Callback);
}

void UPlayerAbilityHandler::UnsubscribeFromSpecChanged(FSpecializationCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnSpecChanged.Remove(Callback);
}

void UPlayerAbilityHandler::NotifyOfNewSpecObject(UPlayerSpecialization* NewSpecialization)
{
	UPlayerSpecialization* const PreviousSpec = CurrentSpec;
	if (IsValid(CurrentSpec))
	{
		CurrentSpec->UnlearnSpecObject();
		CurrentSpec = nullptr;
	}
	if (IsValid(NewSpecialization))
	{
		if (IsValid(CurrentSpec))
		{
			if (!SpecLoadouts.Contains(NewSpecialization->GetClass()))
			{
				FPlayerAbilityLoadout NewLoadout;
				CurrentSpec->CreateNewDefaultLoadout(NewLoadout);
				SpecLoadouts.Add(NewSpecialization->GetClass(), NewLoadout);
			}
			CurrentLoadout = SpecLoadouts.FindRef(NewSpecialization->GetClass());
		}
	}
	if (CurrentSpec != PreviousSpec)
	{
		OnSpecChanged.Broadcast(CurrentSpec);
	}
}
*/
#pragma endregion
