// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerAbilityHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "CombatReactionComponent.h"
#include "ResourceHandler.h"
#include "PlayerCombatAbility.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "PlayerSpecialization.h"

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
	DOREPLIFETIME(UPlayerAbilityHandler, AncientSpecClass);
	DOREPLIFETIME(UPlayerAbilityHandler, ModernSpecClass);
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

	OwnerAsPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerAsPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerAbilityHandler put on non-Pawn class!"));
		return;
	}
	
	if (GetOwnerRole() != ROLE_SimulatedProxy)
	{
		UCombatReactionComponent* ReactionComponent = ISaiyoraCombatInterface::Execute_GetReactionComponent(GetOwner());
		if (IsValid(ReactionComponent))
		{
			switch (ReactionComponent->GetCurrentPlane())
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
			ReactionComponent->SubscribeToPlaneSwap(PlaneSwapCallback);
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
	if (!IsValid(AbilityClass))
	{
		return nullptr;
	}
	if (!AbilityClass->IsChildOf(UPlayerCombatAbility::StaticClass()))
	{
		return nullptr;
	}
	if (!Spellbook.Contains(AbilityClass))
	{
		return nullptr;
	}
	return Super::AddNewAbility(AbilityClass);
}

void UPlayerAbilityHandler::LearnAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (!IsValid(AbilityClass) || Spellbook.Contains(AbilityClass))
	{
		return;
	}
	Spellbook.Add(AbilityClass);
	OnSpellbookUpdated.Broadcast();
}

void UPlayerAbilityHandler::UnlearnAbility(TSubclassOf<UPlayerCombatAbility> const AbilityClass)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
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

/*void UPlayerAbilityHandler::UpdateAbilityBind(TSubclassOf<UCombatAbility> const Ability, int32 const Bind,
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
}*/

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
	//TODO: Check default specs (if any), call ChangeSpecialization to automatically learn all the associated abilities.
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
	UPlayerCombatAbility* PlayerCombatAbility = Cast<UPlayerCombatAbility>(Ability);
	if (!IsValid(PlayerCombatAbility))
	{
		return false;
	}
	if (PlayerCombatAbility->GetActionBar() == CurrentBar || PlayerCombatAbility->GetActionBar() == EActionBarType::Hidden || PlayerCombatAbility->GetActionBar() == EActionBarType::None)
	{
		return false;
	}
	return true;
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

FCastEvent UPlayerAbilityHandler::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!OwnerAsPawn->IsLocallyControlled())
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

bool UPlayerAbilityHandler::UseAbilityFromPredictedMovement(FAbilityRequest const& Request)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Request.AbilityClass) || Request.PredictionID == 0 || Request.Tick != 0)
	{
		//One note is that predictive movement DOES NOT WORK on non-initial ticks, because I am too incompetent to figure that out.
		return false;
	}
	//Check to see if the ability handler has processed this cast already.
	FCastEvent* PreviousResult = PredictedCastRecord.Find(Request.PredictionID);
	if (PreviousResult)
	{
		return PreviousResult->ActionTaken == ECastAction::Success;
	}
	//Process the cast as normal.
	ServerPredictAbility_Implementation(Request);
	//Recheck for the newly created cast record.
	PreviousResult = PredictedCastRecord.Find(Request.PredictionID);
	if (!PreviousResult)
	{
		//Somehow the processing failed to create a record of the cast.
		return false;
	}
	return PreviousResult->ActionTaken == ECastAction::Success;
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

	TMap<TSubclassOf<UResource>, float> Costs;
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
	if (GetOwnerRole() == ROLE_AutonomousProxy)
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

		if (PlayerAbility->HasGlobalCooldown())
		{
			PredictStartGlobal(Result.PredictionID);
			AbilityPrediction.bPredictedGCD = true;
		}

		PlayerAbility->PredictCommitCharges(Result.PredictionID);
		AbilityPrediction.bPredictedCharges = true;

		if (Costs.Num() > 0 && IsValid(GetResourceHandlerRef()))
		{
			GetResourceHandlerRef()->PredictAbilityCosts(Result.Ability, Costs, Result.PredictionID);
			for (TTuple<TSubclassOf<UResource>, float> const& Cost : Costs)
			{
				AbilityPrediction.PredictedCostClasses.Add(Cost.Key);
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
			Result.PredictionParams = AbilityRequest.PredictionParams;
			OnAbilityTick.Broadcast(Result);
			break;
		case EAbilityCastType::Channel :
			Result.ActionTaken = ECastAction::Success;
			PredictStartCast(PlayerAbility, Result.PredictionID);
			AbilityPrediction.bPredictedCastBar = true;
			if (PlayerAbility->GetHasInitialTick())
			{
				PlayerAbility->PredictedTick(0, AbilityRequest.PredictionParams);
				Result.PredictionParams = AbilityRequest.PredictionParams;
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
	
	PlayerAbility->CommitCharges();
	
	if (PlayerAbility->HasGlobalCooldown())
	{
		StartGlobal(PlayerAbility);
	}
	
	if (Costs.Num() > 0 && IsValid(GetResourceHandlerRef()))
	{
		GetResourceHandlerRef()->CommitAbilityCosts(Result.Ability, Costs);
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
		Result.PredictionParams = PredictionParams;
		Result.BroadcastParams = BroadcastParams;
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
			Result.PredictionParams = PredictionParams;
			Result.BroadcastParams = BroadcastParams;
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
	if (PredictedCastRecord.Find(AbilityRequest.PredictionID))
	{
		//We already have a valid cast for this ID from predictive movement.
		UE_LOG(LogTemp, Warning, TEXT("Predictive movement beat the ability handler RPC for cast %i."), AbilityRequest.PredictionID);
		return;
	}
	
	FCastEvent Result;
	Result.PredictionID = AbilityRequest.PredictionID;

	UPlayerCombatAbility* PlayerAbility = FindActiveAbility(AbilityRequest.AbilityClass);
	Result.Ability = PlayerAbility;
	if (!IsValid(PlayerAbility))
	{
		Result.FailReason = ECastFailReason::InvalidAbility;
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		PredictedCastRecord.Add(AbilityRequest.PredictionID, Result);
		return;
	}

	TMap<TSubclassOf<UResource>, float> Costs;
	if (!CheckCanUseAbility(PlayerAbility, Costs, Result.FailReason))
	{
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		PredictedCastRecord.Add(AbilityRequest.PredictionID, Result);
		return;
	}

	FServerAbilityResult ServerResult;
	ServerResult.AbilityClass = AbilityRequest.AbilityClass;
	ServerResult.PredictionID = AbilityRequest.PredictionID;
	//This should probably be manually calculated using ping value and server time, instead of trusting the client.
	//TODO: Update this once I've got Ping worked out and am confident in it being accurate.
	ServerResult.ClientStartTime = AbilityRequest.ClientStartTime;

	if (PlayerAbility->HasGlobalCooldown())
	{
		StartGlobal(PlayerAbility, true);
		ServerResult.bActivatedGlobal = true;
		ServerResult.GlobalLength = PlayerAbility->GetGlobalCooldownLength();
	}

	int32 const PreviousCharges = PlayerAbility->GetCurrentCharges();
	PlayerAbility->CommitCharges(Result.PredictionID);
	int32 const NewCharges = PlayerAbility->GetCurrentCharges();
	ServerResult.ChargesSpent = PreviousCharges - NewCharges;

	if (Costs.Num() > 0 && IsValid(GetResourceHandlerRef()))
	{
		GetResourceHandlerRef()->CommitAbilityCosts(PlayerAbility, Costs, Result.PredictionID);
		for (TTuple<TSubclassOf<UResource>, float> const& Cost : Costs)
		{
			ServerResult.AbilityCosts.Add(FReplicableAbilityCost(Cost.Key, Cost.Value));
		}
	}

	FCombatParameters BroadcastParams;
	
	switch (PlayerAbility->GetCastType())
	{
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		PlayerAbility->ServerTick(0, AbilityRequest.PredictionParams, BroadcastParams);
		Result.BroadcastParams = BroadcastParams;
		OnAbilityTick.Broadcast(Result);
		MulticastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		StartCast(PlayerAbility, Result.PredictionID);
		ServerResult.CastLength = PlayerAbility->GetCastLength();
		ServerResult.bActivatedCastBar = true;
		ServerResult.bInterruptible = CastingState.bInterruptible;
		if (PlayerAbility->GetHasInitialTick())
		{
			PlayerAbility->ServerTick(0, AbilityRequest.PredictionParams, BroadcastParams);
			Result.BroadcastParams = BroadcastParams;
			OnAbilityTick.Broadcast(Result);
			MulticastAbilityTick(Result, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = ECastFailReason::InvalidCastType;
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		PredictedCastRecord.Add(AbilityRequest.PredictionID, Result);
		return;
	}

	ClientSucceedPredictedAbility(ServerResult);
	PredictedCastRecord.Add(AbilityRequest.PredictionID, Result);
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
	
	UPlayerCombatAbility* Ability;
	TArray<TSubclassOf<UResource>> MispredictedCosts;
	FClientAbilityPrediction* OriginalPrediction = UnackedAbilityPredictions.Find(ServerResult.PredictionID);
	if (OriginalPrediction)
	{
		//Mispredicted costs in this context are resources that the client predicted expenditure of that didn't actually get spent on the server.
		MispredictedCosts = OriginalPrediction->PredictedCostClasses;
		for (FReplicableAbilityCost const& Cost : ServerResult.AbilityCosts)
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
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CastStartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	float CastLength = Ability->GetCastLength();
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
	//Notably, don't set the tick or end cast timer. The server's response will set these timers.
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
	TickEvent.PredictionParams = PredictionParams;
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
	TickEvent.PredictionParams = PredictionParams;
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
				TickEvent.PredictionParams = TickRequest.PredictionParams;
				TickEvent.BroadcastParams = BroadcastParams;
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
	if (OwnerAsPawn->IsLocallyControlled())
	{
		FCombatParameters PredictionParams;
		CurrentPlayerCast->PredictedTick(CastingState.ElapsedTicks, PredictionParams);
		CurrentPlayerCast->ServerTick(CastingState.ElapsedTicks, PredictionParams, BroadcastParams);
		FCastEvent TickEvent;
		TickEvent.Ability = CurrentPlayerCast;
		TickEvent.Tick = CastingState.ElapsedTicks;
		TickEvent.ActionTaken = ECastAction::Tick;
		TickEvent.PredictionID = CastingState.PredictionID;
		TickEvent.PredictionParams = PredictionParams;
		TickEvent.BroadcastParams = BroadcastParams;
		OnAbilityTick.Broadcast(TickEvent);
		MulticastAbilityTick(TickEvent, BroadcastParams);
	}
	else if (!CurrentPlayerCast->GetTickNeedsPredictionParams(CastingState.ElapsedTicks))
	{
		CurrentPlayerCast->UCombatAbility::ServerTick(CastingState.ElapsedTicks, BroadcastParams);
		FCastEvent TickEvent;
		TickEvent.Ability = CurrentPlayerCast;
		TickEvent.Tick = CastingState.ElapsedTicks;
		TickEvent.ActionTaken = ECastAction::Tick;
		TickEvent.PredictionID = CastingState.PredictionID;
		TickEvent.BroadcastParams = BroadcastParams;
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
		TickEvent.PredictionParams = ParamsAwaitingTicks.FindRef(Tick);
		TickEvent.BroadcastParams = BroadcastParams;
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
	if (!OwnerAsPawn->IsLocallyControlled())
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
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Result.CancelID = GenerateNewPredictionID();
		FCancelRequest CancelRequest;
		CancelRequest.CancelTime = Result.CancelTime;
		CancelRequest.CancelID = Result.CancelID;
		CancelRequest.CancelledCastID = Result.CancelledCastID;
		CurrentPlayerCast->PredictedCancel(CancelRequest.PredictionParams);
		Result.PredictionParams = CancelRequest.PredictionParams;
		ServerPredictCancelAbility(CancelRequest);
		OnAbilityCancelled.Broadcast(Result);
		CastingState.PredictionID = Result.CancelID;
		EndCast();
		return Result;
	}
	CurrentPlayerCast->UCombatAbility::ServerCancel(Result.BroadcastParams);
	OnAbilityCancelled.Broadcast(Result);
	MulticastAbilityCancel(Result);
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
	Result.PredictionParams = CancelRequest.PredictionParams;
	CurrentPlayerCast->ServerCancel(Result.PredictionParams, Result.BroadcastParams);
	OnAbilityCancelled.Broadcast(Result);
	MulticastAbilityCancel(Result);
	EndCast();
}

#pragma endregion
#pragma region Interrupt

FInterruptEvent UPlayerAbilityHandler::InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource,
	bool const bIgnoreRestrictions)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		FInterruptEvent Fail;
		Fail.FailReason = EInterruptFailReason::NetRole;
		return Fail;
	}
	if (OwnerAsPawn->IsLocallyControlled())
	{
		return Super::InterruptCurrentCast(AppliedBy, InterruptSource, bIgnoreRestrictions);
	}
	int32 const CastID = CastingState.PredictionID;
	FInterruptEvent Result = Super::InterruptCurrentCast(AppliedBy, InterruptSource, bIgnoreRestrictions);
	if (!Result.bSuccess)
	{
		return Result;
	}
	Result.CancelledCastID = CastID;
	ClientInterruptCast(Result);
	return Result;
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
	float GlobalLength = Ability->GetGlobalCooldownLength();
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
		if (!GlobalCooldownState.bGlobalCooldownActive && OwnerAsPawn->IsLocallyControlled())
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
	if (AbilityClass->GetDefaultObject<UPlayerCombatAbility>()->HasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime == 0.0f)
	{
		//Don't queue an ability if the gcd time hasn't been acked yet.
		return false;
	}
	float const GlobalTimeRemaining = AbilityClass->GetDefaultObject<UPlayerCombatAbility>()->HasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive ? GetGlobalCooldownTimeRemaining() : 0.0f;
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

void UPlayerAbilityHandler::ChangeSpecialization(TSubclassOf<UPlayerSpecialization> const NewSpecialization)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(NewSpecialization))
	{
		return;
	}
	UPlayerSpecialization* DefaultObject = NewSpecialization->GetDefaultObject<UPlayerSpecialization>();
	if (!IsValid(DefaultObject) || (DefaultObject->GetSpecBar() != EActionBarType::Ancient && DefaultObject->GetSpecBar() != EActionBarType::Modern))
	{
		return;
	}
	UPlayerSpecialization* PreviousSpec;
	switch (DefaultObject->GetSpecBar())
	{
		case EActionBarType::Ancient :
			PreviousSpec = AncientSpec;
			break;
		case EActionBarType::Modern :
			PreviousSpec = ModernSpec;
			break;
		default :
			return;
	}
	if (IsValid(PreviousSpec))
	{
		if (PreviousSpec->GetClass() == NewSpecialization)
		{
			return;
		}
		TSet<TSubclassOf<UPlayerCombatAbility>> GrantedAbilities;
		PreviousSpec->GetDefaultAbilities(GrantedAbilities);
		for (TSubclassOf<UPlayerCombatAbility> const AbilityClass : GrantedAbilities)
		{
			if (IsValid(AbilityClass))
			{
				UnlearnAbility(AbilityClass);
			}
		}
		PreviousSpec->UnlearnSpecObject();
	}
	UPlayerSpecialization* NewSpec;
	switch (DefaultObject->GetSpecBar())
	{
		case EActionBarType::Ancient :
			AncientSpec = NewObject<UPlayerSpecialization>(GetOwner(), NewSpecialization);
			NewSpec = AncientSpec;
			AncientSpecClass = AncientSpec->GetClass();
			break;
		case EActionBarType::Modern :
			ModernSpec = NewObject<UPlayerSpecialization>(GetOwner(), NewSpecialization);
			NewSpec = ModernSpec;
			ModernSpecClass = ModernSpec->GetClass();
			break;
		default :
			return;
	}
	if (IsValid(NewSpec))
	{
		NewSpec->InitializeSpecObject(this);
		TSet<TSubclassOf<UPlayerCombatAbility>> GrantedAbilities;
		NewSpec->GetDefaultAbilities(GrantedAbilities);
		for (TSubclassOf<UPlayerCombatAbility> const AbilityClass : GrantedAbilities)
		{
			if (IsValid(AbilityClass))
			{
				LearnAbility(AbilityClass);
				AddNewAbility(AbilityClass);
			}
		}
		OnSpecChanged.Broadcast(NewSpec->GetClass());
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

void UPlayerAbilityHandler::OnRep_AncientSpecClass(TSubclassOf<UPlayerSpecialization> const OldSpecClass)
{
	if (OldSpecClass == AncientSpecClass)
	{
		return;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		if (IsValid(AncientSpec))
		{
			AncientSpec->UnlearnSpecObject();
		}
		AncientSpec = NewObject<UPlayerSpecialization>(GetOwner(), AncientSpecClass);
		if (IsValid(AncientSpec))
		{
			AncientSpec->InitializeSpecObject(this);
		}
	}
	OnSpecChanged.Broadcast(AncientSpecClass);
}

void UPlayerAbilityHandler::OnRep_ModernSpecClass(TSubclassOf<UPlayerSpecialization> const OldSpecClass)
{
	if (OldSpecClass == ModernSpecClass)
	{
		return;
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		if (IsValid(ModernSpec))
		{
			ModernSpec->UnlearnSpecObject();
		}
		ModernSpec = NewObject<UPlayerSpecialization>(GetOwner(), ModernSpecClass);
		if (IsValid(ModernSpec))
		{
			ModernSpec->InitializeSpecObject(this);
		}
	}
	OnSpecChanged.Broadcast(ModernSpecClass);
}
#pragma endregion
