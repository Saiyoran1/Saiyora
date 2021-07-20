// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerAbilityHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlaneComponent.h"
#include "ResourceHandler.h"
#include "DamageHandler.h"
#include "UnrealNetwork.h"

int32 UPlayerAbilityHandler::ClientPredictionID = 0;

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

	switch (GetOwnerRole())
	{
		case ROLE_Authority :
			if (GetNetMode() == NM_ListenServer)
			{
				AbilityPermission = EAbilityPermission::AuthPlayer;
			}
			else
			{
				AbilityPermission = EAbilityPermission::Auth;			
			}
			break;
		case ROLE_AutonomousProxy :
			AbilityPermission = EAbilityPermission::PredictPlayer;
			break;
		default:
			break;
	}
	
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

void UPlayerAbilityHandler::LearnAbility(TSubclassOf<UCombatAbility> const NewAbility)
{
	if (!IsValid(NewAbility) || Spellbook.Contains(NewAbility))
	{
		return;
	}
	Spellbook.Add(NewAbility);
	OnSpellbookUpdated.Broadcast();
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
			Loadout.AncientLoadout.Add(Bind, Ability);
			break;
		case EActionBarType::Modern :
			Loadout.ModernLoadout.Add(Bind, Ability);
			break;
		case EActionBarType::Hidden :
			Loadout.HiddenLoadout.Add(Bind, Ability);
			break;
		default :
			return;
	}
	OnAbilityBindUpdated.Broadcast(Bind, Bar, FindActiveAbility(Ability));
}

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

#pragma endregion
#pragma region AbilityUsage

FCastEvent UPlayerAbilityHandler::AbilityInput(int32 const BindNumber, bool const bHidden)
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
		AbilityClass = Loadout.HiddenLoadout.FindRef(BindNumber);
	}
	else
	{
		switch (CurrentBar)
		{
		case EActionBarType::Ancient :
			AbilityClass = Loadout.AncientLoadout.FindRef(BindNumber);
			break;
		case EActionBarType::Modern :
			AbilityClass = Loadout.ModernLoadout.FindRef(BindNumber);
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
}

FCastEvent UPlayerAbilityHandler::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	switch (AbilityPermission)
	{
	case EAbilityPermission::AuthPlayer :
		return Super::UseAbility(AbilityClass);
	case EAbilityPermission::PredictPlayer :
		return PredictUseAbility(AbilityClass);
	default:
		{
			FCastEvent Failure;
			Failure.FailReason = ECastFailReason::NetRole;
			return Failure;
		}
	}
	
}

FCastEvent UPlayerAbilityHandler::PredictUseAbility(TSubclassOf<UCombatAbility> const AbilityClass, bool const bFromQueue)
{
	ClearQueue();
	FCastEvent Result;
	
	Result.Ability = FindActiveAbility(AbilityClass);
	if (!IsValid(Result.Ability))
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

	Result.PredictionID = GenerateNewPredictionID();

	FAbilityRequest AbilityRequest;
	AbilityRequest.AbilityClass = AbilityClass;
	AbilityRequest.PredictionID = Result.PredictionID;
	AbilityRequest.ClientStartTime = GetGameStateRef()->GetServerWorldTimeSeconds();

	FClientAbilityPrediction AbilityPrediction;
	AbilityPrediction.Ability = Result.Ability;
	AbilityPrediction.PredictionID = Result.PredictionID;
	AbilityPrediction.ClientTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		PredictStartGlobal(Result.PredictionID);
		AbilityPrediction.bPredictedGCD = true;
	}

	Result.Ability->PredictCommitCharges(Result.PredictionID);
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
		Result.Ability->PredictedTick(0, AbilityRequest.PredictionParams);
		OnAbilityTick.Broadcast(Result);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		PredictStartCast(Result.Ability, Result.PredictionID);
		AbilityPrediction.bPredictedCastBar = true;
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->PredictedTick(0, AbilityRequest.PredictionParams);
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

void UPlayerAbilityHandler::ServerPredictAbility_Implementation(FAbilityRequest const& AbilityRequest)
{
	FCastEvent Result;
	Result.PredictionID = AbilityRequest.PredictionID;
	
	Result.Ability = FindActiveAbility(AbilityRequest.AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = ECastFailReason::InvalidAbility;
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	TArray<FAbilityCost> Costs;
	CheckCanUseAbility(Result.Ability, Costs, Result.FailReason);

	FServerAbilityResult ServerResult;
	ServerResult.AbilityClass = AbilityRequest.AbilityClass;
	ServerResult.PredictionID = AbilityRequest.PredictionID;
	//TODO: This should REALLY be manually calculated using ping value and server time.
	ServerResult.ClientStartTime = AbilityRequest.ClientStartTime;

	if (Result.Ability->GetHasGlobalCooldown())
	{
		StartGlobal(Result.Ability, true);
		ServerResult.bActivatedGlobal = true;
		ServerResult.GlobalLength = CalculateGlobalCooldownLength(Result.Ability);
	}

	int32 const PreviousCharges = Result.Ability->GetCurrentCharges();
	Result.Ability->CommitCharges(Result.PredictionID);
	int32 const NewCharges = Result.Ability->GetCurrentCharges();
	ServerResult.ChargesSpent = PreviousCharges - NewCharges;
	ServerResult.bSpentCharges = (ServerResult.ChargesSpent != 0);

	if (Costs.Num() > 0 && IsValid(GetResourceHandlerRef()))
	{
		GetResourceHandlerRef()->CommitAbilityCosts(Result.Ability, Costs, Result.PredictionID);
		ServerResult.AbilityCosts = Costs;
	}

	FCombatParameters BroadcastParams;
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = ECastFailReason::InvalidCastType;
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->ServerPredictedTick(0, AbilityRequest.PredictionParams, BroadcastParams);
		OnAbilityTick.Broadcast(Result);
		MulticastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		ServerResult.CastLength = CalculateCastLength(Result.Ability);
		StartCast(Result.Ability, Result.PredictionID);
		ServerResult.bActivatedCastBar = true;
		ServerResult.bInterruptible = CastingState.bInterruptible;
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerPredictedTick(0, AbilityRequest.PredictionParams, BroadcastParams);
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
	UCombatAbility* Ability;
	
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
		Ability->UpdatePredictedChargesFromServer(ServerResult);
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
			OriginalPrediction->Ability->RollbackFailedCharges(PredictionID);
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
	CastingState.ElapsedTicks++;
	FAbilityRequest TickRequest;
	CastingState.CurrentCast->PredictedTick(CastingState.ElapsedTicks, TickRequest.PredictionParams);
	if (CastingState.CurrentCast->GetTickNeedsPredictionParams(CastingState.ElapsedTicks))
	{
		TickRequest.Tick = CastingState.ElapsedTicks;
		TickRequest.AbilityClass = CastingState.CurrentCast->GetClass();
		TickRequest.PredictionID = CastingState.PredictionID;
		ServerHandlePredictedTick(TickRequest);
	}
	FCastEvent TickEvent;
	TickEvent.Ability = CastingState.CurrentCast;
	TickEvent.Tick = CastingState.ElapsedTicks;
	TickEvent.ActionTaken = ECastAction::Tick;
	TickEvent.PredictionID = CastingState.PredictionID;
	OnAbilityTick.Broadcast(TickEvent);
	if (CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNumberOfTicks())
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
	FAbilityRequest TickRequest;
	CastingState.CurrentCast->PredictedTick(TickNumber, TickRequest.PredictionParams);
	if (CastingState.CurrentCast->GetTickNeedsPredictionParams(TickNumber))
	{
		TickRequest.Tick = TickNumber;
		TickRequest.AbilityClass = CastingState.CurrentCast->GetClass();
		TickRequest.PredictionID = CastingState.PredictionID;
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
			UCombatAbility* Ability = FindActiveAbility(TickRequest.AbilityClass);
			if (IsValid(Ability))
			{
				FCombatParameters BroadcastParams;
				FCastEvent TickEvent;
				TickEvent.Ability = Ability;
				TickEvent.Tick = TickRequest.Tick;
				TickEvent.ActionTaken = ECastAction::Tick;
				TickEvent.PredictionID = TickRequest.PredictionID;
				Ability->ServerPredictedTick(TickRequest.Tick, TickRequest.PredictionParams, BroadcastParams);
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
	CastingState.ElapsedTicks++;
	FPredictedTick Tick;
	Tick.PredictionID = CastingState.PredictionID;
	Tick.TickNumber = CastingState.ElapsedTicks;
	FCombatParameters BroadcastParams;
	if (!CastingState.CurrentCast->GetTickNeedsPredictionParams(CastingState.ElapsedTicks))
	{
		CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, BroadcastParams);
		FCastEvent TickEvent;
		TickEvent.Ability = CastingState.CurrentCast;
		TickEvent.Tick = CastingState.ElapsedTicks;
		TickEvent.ActionTaken = ECastAction::Tick;
		TickEvent.PredictionID = CastingState.PredictionID;
		OnAbilityTick.Broadcast(TickEvent);
		MulticastAbilityTick(TickEvent, BroadcastParams);
	}
	else if (ParamsAwaitingTicks.Contains(Tick))
	{
		CastingState.CurrentCast->ServerPredictedTick(CastingState.ElapsedTicks, ParamsAwaitingTicks.FindRef(Tick), BroadcastParams);
		FCastEvent TickEvent;
		TickEvent.Ability = CastingState.CurrentCast;
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
	if (CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNumberOfTicks())
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
	switch (AbilityPermission)
	{
		case EAbilityPermission::AuthPlayer :
			return Super::CancelCurrentCast();
		case EAbilityPermission::PredictPlayer :
			return PredictCancelAbility();
		default :
			{
				FCancelEvent Fail;
				Fail.FailReason = ECancelFailReason::NetRole;
				return Fail;
			}
	}
}

FCancelEvent UPlayerAbilityHandler::PredictCancelAbility()
{
	FCancelEvent Result;
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		Result.FailReason = ECancelFailReason::NetRole;
		return Result;
	}
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	Result.CancelID = GenerateNewPredictionID();
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.CancelledCastID = CastingState.PredictionID;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	FCancelRequest CancelRequest;
	CancelRequest.CancelTime = Result.CancelTime;
	CancelRequest.CancelID = Result.CancelID;
	CancelRequest.CancelledCastID = Result.CancelledCastID;
	Result.bSuccess = true;
	Result.CancelledAbility->PredictedCancel(CancelRequest.PredictionParams);
	ServerPredictCancelAbility(CancelRequest);
	OnAbilityCancelled.Broadcast(Result);
	CastingState.PredictionID = Result.CancelID;
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
	FCancelEvent Result;
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	Result.CancelID = CancelRequest.CancelID;
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.CancelledCastID = CastingState.PredictionID;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	FCombatParameters BroadcastParams;
	Result.CancelledAbility->ServerPredictedCancel(CancelRequest.PredictionParams, BroadcastParams);
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
	case EAbilityPermission::AuthPlayer :
		return Super::InterruptCurrentCast(AppliedBy, InterruptSource, bIgnoreRestrictions);
	case EAbilityPermission::Auth :
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
		if (!GlobalCooldownState.bGlobalCooldownActive && AbilityPermission != EAbilityPermission::None)
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

bool UPlayerAbilityHandler::TryQueueAbility(TSubclassOf<UCombatAbility> const AbilityClass)
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
	if (AbilityClass->GetDefaultObject<UCombatAbility>()->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime == 0.0f)
	{
		//Don't queue an ability if the gcd time hasn't been acked yet.
		return false;
	}
	float const GlobalTimeRemaining = AbilityClass->GetDefaultObject<UCombatAbility>()->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive ? GetGlobalCooldownTimeRemaining() : 0.0f;
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
	TSubclassOf<UCombatAbility> AbilityClass;
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
				PredictUseAbility(AbilityClass, true);
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
	TSubclassOf<UCombatAbility> AbilityClass;
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
			PredictUseAbility(AbilityClass, true);
		}
		return;
	case EQueueStatus::WaitForGlobal :
		return;
	default :
		return;
	}
}

#pragma endregion