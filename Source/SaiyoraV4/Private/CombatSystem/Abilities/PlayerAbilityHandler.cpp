// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerAbilityHandler.h"

#include "PlayerAbilitySave.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlaneComponent.h"
#include "SaiyoraPlayerCharacter.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "DamageHandler.h"
#include "ResourceHandler.h"
#include "CrowdControlHandler.h"

const int32 UPlayerAbilityHandler::AbilitiesPerBar = 6;
const float UPlayerAbilityHandler::AbilityQueWindowSec = 0.2f;
const float UPlayerAbilityHandler::MaxPingCompensation = 0.2f;

UPlayerAbilityHandler::UPlayerAbilityHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UPlayerAbilityHandler::InitializeComponent()
{
	OwnerAsPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerAsPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner of PlayerAbilityComponent was not a Pawn!"));
		return;
	}
	bOwnerIsLocal = OwnerAsPawn->IsLocallyControlled();
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		return;
	}
	/*
	ACharacter* OwningPlayer = Cast<ACharacter>(GetOwner());
	if (IsValid(OwningPlayer))
	{
		PlayerStateRef = OwningPlayer->GetPlayerState();
	}
	if (IsValid(PlayerStateRef))
	{
		//TODO: Not sure PlayerName is the best way to handle this, considering I have no idea how to set it.
		FString SaveName = PlayerStateRef->GetPlayerName();
		SaveName.Append(TEXT("Abilities"));
		AbilitySave = Cast<UPlayerAbilitySave>(UGameplayStatics::LoadGameFromSlot(SaveName, 0));
	}
	if (IsValid(AbilitySave))
	{
		if (GetOwnerRole() == ROLE_Authority)
		{
			AbilitySave->GetUnlockedAbilities(Spellbook);
		}
		AbilitySave->GetLastSavedLoadout(Loadout);
	}
	else
	{
		AbilitySave = NewObject<UPlayerAbilitySave>(GetOwner(), UPlayerAbilitySave::StaticClass());
		AbilitySave->GetLastSavedLoadout(Loadout);
	}
	FAbilityRestriction PlaneAbilityRestriction;
	PlaneAbilityRestriction.BindUFunction(this, FName(TEXT("CheckForCorrectAbilityPlane")));
	AddAbilityRestriction(PlaneAbilityRestriction);
	*/
}

int32 UPlayerAbilityHandler::GenerateNewPredictionID()
{
	ClientPredictionID++;
	return ClientPredictionID;
}

FCastEvent UPlayerAbilityHandler::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!bOwnerIsLocal)
	{
		FCastEvent Failure;
		Failure.FailReason = FString(TEXT("Tried to use ability without being local player."));
		return Failure;
	}
	switch (GetOwnerRole())
	{
		case ROLE_Authority :
			FCastEvent Auth;
			return Auth;//TODO: Listen Server ability usage.
		case ROLE_AutonomousProxy :
			return PredictUseAbility(AbilityClass, false);
		case ROLE_SimulatedProxy :
			FCastEvent Sim;
			Sim.FailReason = FString(TEXT("Tried to use ability on simualted proxy."));
			return Sim;
		default :
			FCastEvent Failure;
			Failure.FailReason = FString(TEXT("Defaulted on OwnerRole during ability usage."));
			return Failure;
	}
}

FCastEvent UPlayerAbilityHandler::PredictUseAbility(TSubclassOf<UCombatAbility> const AbilityClass,
	bool const bFromQueue)
{
	ClearQueue();
	FCastEvent Result;
	
	Result.Ability = FindActiveAbility(AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Did not find valid active ability."));
		return Result;
	}

	if (!Result.Ability->GetCastableWhileDead() && IsValid(GetDamageHandler()) && GetDamageHandler()->GetLifeStatus() != ELifeStatus::Alive)
	{
		Result.FailReason = FString(TEXT("Cannot activate while dead."));
		return Result;
	}
	
	if (Result.Ability->GetHasGlobalCooldown() && GlobalCooldown.bGlobalCooldownActive)
	{
		Result.FailReason = FString(TEXT("Already on global cooldown."));
		if (!bFromQueue)
		{
			if (TryQueueAbility(AbilityClass))
			{
				Result.FailReason = FString(TEXT("Queued ability because of Global Cooldown."));
			}
		}
		return Result;
	}
	
	if (CastingState.bIsCasting)
	{
		Result.FailReason = FString(TEXT("Already casting."));
		if (!bFromQueue)
		{
			if (TryQueueAbility(AbilityClass))
			{
				Result.FailReason = FString(TEXT("Queued ability because of Cast Time."));
			}
		}
		return Result;
	}
	
	TArray<FAbilityCost> Costs;
	Result.Ability->GetAbilityCosts(Costs);
	if (Costs.Num() > 0)
	{
		if (!IsValid(GetResourceHandler()))
		{
			Result.FailReason = FString(TEXT("No valid resource handler found."));
			return Result;
		}
		if (!GetResourceHandler()->CheckAbilityCostsMet(Result.Ability, Costs))
		{
			Result.FailReason = FString(TEXT("Ability costs not met."));
			return Result;
		}
	}

	if (!Result.Ability->CheckChargesMet())
	{
		Result.FailReason = FString(TEXT("Charges not met."));
		return Result;
	}

	if (!Result.Ability->CheckCustomCastConditionsMet())
	{
		Result.FailReason = FString(TEXT("Custom cast conditions not met."));
		return Result;
	}

	if (CheckAbilityRestricted(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Cast restriction returned true."));
		return Result;
	}

	if (IsValid(GetCrowdControlHandler()))
	{
		for (TSubclassOf<UCrowdControl> const CcClass : Result.Ability->GetRestrictedCrowdControls())
		{
			if (GetCrowdControlHandler()->GetActiveCcTypes().Contains(CcClass))
			{
				Result.FailReason = FString(TEXT("Cast restricted by crowd control."));
				return Result;
			}
		}
	}

	int32 const PredictionID = GenerateNewPredictionID();

	FAbilityRequest AbilityRequest;
	AbilityRequest.AbilityClass = AbilityClass;
	AbilityRequest.PredictionID = PredictionID;
	AbilityRequest.ClientStartTime = GetGameState()->GetServerWorldTimeSeconds();

	FClientAbilityPrediction AbilityPrediction;
	AbilityPrediction.Ability = Result.Ability;
	AbilityPrediction.PredictionID = PredictionID;
	AbilityPrediction.ClientTime = GetGameState()->GetServerWorldTimeSeconds();
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		PredictStartGlobal(PredictionID);
		AbilityPrediction.bPredictedGCD = true;
	}

	Result.Ability->PredictCommitCharges(PredictionID);
	AbilityPrediction.bPredictedCharges = true;
	
	if (Costs.Num() > 0 && IsValid(GetResourceHandler()))
	{
		GetResourceHandler()->PredictCommitAbilityCosts(Result.Ability, PredictionID, Costs);
		for (FAbilityCost const& Cost : Costs)
		{
			AbilityPrediction.PredictedCostClasses.Add(Cost.ResourceClass);
		}
	}
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = FString(TEXT("No cast type was set."));
		return Result;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->PredictedTick(0, AbilityRequest.PredictionParams);
		FireOnAbilityTick(Result);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		PredictStartCast(Result.Ability, PredictionID);
		AbilityPrediction.bPredictedCastBar = true;
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->PredictedTick(0, AbilityRequest.PredictionParams);
			FireOnAbilityTick(Result);
		}
		break;
	default :
		Result.FailReason = FString(TEXT("Defaulted on cast type."));
		return Result;
	}
	
	UnackedAbilityPredictions.Add(AbilityPrediction.PredictionID, AbilityPrediction);
	ServerPredictAbility(AbilityRequest);
	
	return Result;
}

void UPlayerAbilityHandler::ClientFailPredictedAbility_Implementation(int32 const PredictionID, FString const& FailReason)
{
	if (GlobalCooldown.bGlobalCooldownActive && GlobalCooldown.PredictionID <= PredictionID)
	{
		EndGlobalCooldown();
		GlobalCooldown.PredictionID = PredictionID;
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
	if (IsValid(GetResourceHandler()) && !OriginalPrediction->PredictedCostClasses.Num() == 0)
	{
		GetResourceHandler()->RollbackFailedCosts(OriginalPrediction->PredictedCostClasses, PredictionID);
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

void UPlayerAbilityHandler::ServerPredictAbility_Implementation(FAbilityRequest const& AbilityRequest)
{
	FCastEvent Result;
	
	Result.Ability = FindActiveAbility(AbilityRequest.AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Did not find valid active ability."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	if (!Result.Ability->GetCastableWhileDead() && IsValid(GetDamageHandler()) && GetDamageHandler()->GetLifeStatus() != ELifeStatus::Alive)
	{
		Result.FailReason = FString(TEXT("Cannot activate while dead."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}
	
	if (Result.Ability->GetHasGlobalCooldown() && GlobalCooldown.bGlobalCooldownActive)
	{
		Result.FailReason = FString(TEXT("Already on global cooldown."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	if (CastingState.bIsCasting)
	{
		Result.FailReason = FString(TEXT("Already casting."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	TArray<FAbilityCost> Costs;
	Result.Ability->GetAbilityCosts(Costs);
	if (Costs.Num() > 0)
	{
		if (!IsValid(GetResourceHandler()))
		{
			Result.FailReason = FString(TEXT("No valid resource handler found."));
			ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
			return;
		}
		if (!GetResourceHandler()->CheckAbilityCostsMet(Result.Ability, Costs))
		{
			Result.FailReason = FString(TEXT("Ability costs not met."));
			ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
			return;
		}
	}

	if (!Result.Ability->CheckChargesMet())
	{
		Result.FailReason = FString(TEXT("Charges not met."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	if (!Result.Ability->CheckCustomCastConditionsMet())
	{
		Result.FailReason = FString(TEXT("Custom cast conditions not met."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	if (CheckAbilityRestricted(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Cast restriction returned true."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	if (IsValid(GetCrowdControlHandler()))
	{
		for (TSubclassOf<UCrowdControl> const CcClass : Result.Ability->GetRestrictedCrowdControls())
		{
			if (GetCrowdControlHandler()->GetActiveCcTypes().Contains(CcClass))
			{
				Result.FailReason = FString(TEXT("Cast restricted by crowd control."));
				ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
				return;
			}
		}
	}

	FServerAbilityResult ServerResult;
	ServerResult.AbilityClass = AbilityRequest.AbilityClass;
	ServerResult.PredictionID = AbilityRequest.PredictionID;
	//TODO: This should REALLY be manually calculated using ping value and server time.
	ServerResult.ClientStartTime = AbilityRequest.ClientStartTime;

	if (Result.Ability->GetHasGlobalCooldown())
	{
		StartGlobal(Result.Ability);
		ServerResult.bActivatedGlobal = true;
		//Recalculate global length to get the global length without ping compensation.
		ServerResult.GlobalLength = CalculateGlobalCooldownLength(Result.Ability);
	}

	int32 const PreviousCharges = Result.Ability->GetCurrentCharges();
	Result.Ability->CommitCharges(AbilityRequest.PredictionID);
	int32 const NewCharges = Result.Ability->GetCurrentCharges();
	ServerResult.ChargesSpent = PreviousCharges - NewCharges;
	ServerResult.bSpentCharges = (ServerResult.ChargesSpent != 0);

	if (Costs.Num() > 0 && IsValid(GetResourceHandler()))
	{
		GetResourceHandler()->CommitAbilityCosts(Result.Ability, Costs, AbilityRequest.PredictionID);
		ServerResult.AbilityCosts = Costs;
	}

	FCombatParameters BroadcastParams;
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = FString(TEXT("No cast type was set."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->ServerPredictedTick(0, AbilityRequest.PredictionParams, BroadcastParams);
		FireOnAbilityTick(Result);
		BroadcastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		ServerResult.CastLength = CalculateCastLength(Result.Ability);
		StartCast(Result.Ability, AbilityRequest.PredictionID);
		ServerResult.bActivatedCastBar = true;
		ServerResult.bInterruptible = CastingState.bInterruptible;
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerPredictedTick(0, AbilityRequest.PredictionParams, BroadcastParams);
			FireOnAbilityTick(Result);
			BroadcastAbilityTick(Result, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = FString(TEXT("Defaulted on cast type."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	ClientSucceedPredictedAbility(ServerResult);
}

//GLOBAL COOLDOWN

void UPlayerAbilityHandler::PredictStartGlobal(int32 const PredictionID)
{
	FPredictedGlobalCooldown const PreviousState = GlobalCooldown;
	GlobalCooldown.bGlobalCooldownActive = true;
	GlobalCooldown.PredictionID = PredictionID;
	GlobalCooldown.StartTime = GetGameState()->GetServerWorldTimeSeconds();
	GlobalCooldown.EndTime = 0.0f;
	FireOnGlobalCooldownChanged(PreviousState.ToGlobalCooldown(), GlobalCooldown.ToGlobalCooldown());
}

void UPlayerAbilityHandler::StartGlobal(UCombatAbility* Ability)
{
	FGlobalCooldown const PreviousGlobal = GlobalCooldown.ToGlobalCooldown();
	GlobalCooldown.bGlobalCooldownActive = true;
	GlobalCooldown.StartTime = GetGameState()->GetServerWorldTimeSeconds();
	float GlobalLength = CalculateGlobalCooldownLength(Ability);
	float const PingCompensation = FMath::Clamp(USaiyoraCombatLibrary::GetActorPing(GetOwner()), 0.0f, MaxPingCompensation);
	GlobalLength = FMath::Max(MinimumGlobalCooldownLength, GlobalLength - PingCompensation);
	GlobalCooldown.EndTime = GlobalCooldown.StartTime + GlobalLength;
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UPlayerAbilityHandler::EndGlobalCooldown, GlobalLength, false);
	FireOnGlobalCooldownChanged(PreviousGlobal, GlobalCooldown.ToGlobalCooldown());
}

void UPlayerAbilityHandler::EndGlobalCooldown()
{
	GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
	if (GlobalCooldown.bGlobalCooldownActive)
    {
    	FGlobalCooldown const PreviousGlobal = GlobalCooldown.ToGlobalCooldown();
    	GlobalCooldown.bGlobalCooldownActive = false;
    	GlobalCooldown.StartTime = 0.0f;
    	GlobalCooldown.EndTime = 0.0f;
    	FireOnGlobalCooldownChanged(PreviousGlobal, GlobalCooldown.ToGlobalCooldown());
    	if (GetOwnerRole() == ROLE_AutonomousProxy)
    	{
    		CheckForQueuedAbilityOnGlobalEnd();
    	}
    }
}

//CASTING

void UPlayerAbilityHandler::PredictStartCast(UCombatAbility* Ability, int32 const PredictionID)
{
	FPredictedCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.PredictionID = PredictionID;
	CastingState.CastStartTime = GetGameState()->GetServerWorldTimeSeconds();
	CastingState.CastEndTime = 0.0f;
	CastingState.bInterruptible = Ability->GetInterruptible();
	CastingState.ElapsedTicks = 0;
	FireOnCastStateChanged(PreviousState.ToCastingState(), CastingState.ToCastingState());
}

void UPlayerAbilityHandler::StartCast(UCombatAbility* Ability, int32 const PredictionID)
{
	FCastingState const PreviousState = CastingState.ToCastingState();
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CastStartTime = GetGameState()->GetServerWorldTimeSeconds();
	float CastLength = CalculateCastLength(Ability);
	CastingState.PredictionID = PredictionID;
	float const PingCompensation = FMath::Clamp(USaiyoraCombatLibrary::GetActorPing(GetOwner()), 0.0f, MaxPingCompensation);
	CastLength = FMath::Max(MinimumCastLength, CastLength - PingCompensation);
	CastingState.CastEndTime = CastingState.CastStartTime + CastLength;
	CastingState.bInterruptible = Ability->GetInterruptible();
	GetWorld()->GetTimerManager().SetTimer(CastHandle, this, &UPlayerAbilityHandler::CompleteCast, CastLength, false);
	GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UPlayerAbilityHandler::TickCast, (CastLength / Ability->GetNumberOfTicks()), true);
	FireOnCastStateChanged(PreviousState, CastingState.ToCastingState());
}

void UPlayerAbilityHandler::EndCast()
{
	FCastingState const PreviousState = CastingState.ToCastingState();
	CastingState.bIsCasting = false;
	CastingState.CurrentCast = nullptr;
	CastingState.bInterruptible = false;
	CastingState.ElapsedTicks = 0;
	CastingState.CastStartTime = 0.0f;
	CastingState.CastEndTime = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	GetWorld()->GetTimerManager().ClearTimer(TickHandle);
	FireOnCastStateChanged(PreviousState, CastingState.ToCastingState());
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		CheckForQueuedAbilityOnCastEnd();
	}
}

void UPlayerAbilityHandler::CompleteCast()
{
	FCastEvent CompletionEvent;
    CompletionEvent.Ability = CastingState.CurrentCast;
    CompletionEvent.ActionTaken = ECastAction::Complete;
    CompletionEvent.Tick = CastingState.ElapsedTicks;
    if (IsValid(CompletionEvent.Ability))
    {
    	CastingState.CurrentCast->CompleteCast();
    	FireOnAbilityComplete(CastingState.CurrentCast);
    	if (GetOwnerRole() == ROLE_Authority)
    	{
    		BroadcastAbilityComplete(CompletionEvent);
    	}
    }
    GetWorld()->GetTimerManager().ClearTimer(CastHandle);
    if (!GetWorld()->GetTimerManager().IsTimerActive(TickHandle))
    {
    	EndCast();
    }
}

void UPlayerAbilityHandler::BroadcastAbilityTick_Implementation(FCastEvent const& TickEvent, FCombatParameters const& BroadcastParams)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		if (IsValid(TickEvent.Ability))
		{
			TickEvent.Ability->SimulatedTick(TickEvent.Tick, BroadcastParams);
			FireOnAbilityTick(TickEvent);
		}
	}
}

void UPlayerAbilityHandler::BroadcastAbilityComplete_Implementation(FCastEvent const& CompletionEvent)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		if (IsValid(CompletionEvent.Ability))
		{
			CompletionEvent.Ability->CompleteCast();
			FireOnAbilityComplete(CompletionEvent.Ability);
		}
	}
}

void UPlayerAbilityHandler::BroadcastAbilityCancel_Implementation(FCancelEvent const& CancelEvent,
	FCombatParameters const& BroadcastParams)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		if (IsValid(CancelEvent.CancelledAbility))
		{
			CancelEvent.CancelledAbility->SimulatedCancel(BroadcastParams);
			FireOnAbilityCancelled(CancelEvent);
		}
	}
}

void UPlayerAbilityHandler::BroadcastAbilityInterrupt_Implementation(FInterruptEvent const& InterruptEvent)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		if (IsValid(InterruptEvent.InterruptedAbility))
		{
			InterruptEvent.InterruptedAbility->InterruptCast(InterruptEvent);
			FireOnAbilityInterrupted(InterruptEvent);
		}
	}
}

//OTHER STUFF

bool UPlayerAbilityHandler::CheckForCorrectAbilityPlane(UCombatAbility* Ability)
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

void UPlayerAbilityHandler::BeginPlay()
{
	Super::BeginPlay();
	/*
	if (GetOwnerRole() == ROLE_AutonomousProxy)
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
			PlaneSwapCallback.BindUFunction(this, FName(TEXT("UpdateAbilityPlane")));
			PlaneComponent->SubscribeToPlaneSwap(PlaneSwapCallback);
		}
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (TTuple<int32, TSubclassOf<UCombatAbility>> const& AbilityPair : Loadout.AncientLoadout)
		{
			if (Spellbook.Contains(AbilityPair.Value))
			{
				AddNewAbility(AbilityPair.Value);
			}
		}
		for (TTuple<int32, TSubclassOf<UCombatAbility>> const& AbilityPair : Loadout.ModernLoadout)
		{
			if (Spellbook.Contains(AbilityPair.Value))
			{
				AddNewAbility(AbilityPair.Value);
			}
		}
		for (TTuple<int32, TSubclassOf<UCombatAbility>> const& AbilityPair : Loadout.HiddenLoadout)
		{
			if (Spellbook.Contains(AbilityPair.Value))
			{
				AddNewAbility(AbilityPair.Value);
			}
		}
	}
	*/
}

void UPlayerAbilityHandler::UpdateAbilityPlane(ESaiyoraPlane const PreviousPlane, ESaiyoraPlane const NewPlane, UObject* Source)
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

void UPlayerAbilityHandler::AbilityInput(int32 const BindNumber, bool const bHidden)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		return;
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
		UseAbility(AbilityClass);
	}
}

void UPlayerAbilityHandler::LearnAbility(TSubclassOf<UCombatAbility> const NewAbility)
{
	if (!IsValid(NewAbility) || Spellbook.Contains(NewAbility))
	{
		return;
	}
	Spellbook.Add(NewAbility);
	//TODO: Spellbook delegate? Can't use TSubclassOf vars in the delegate I think.
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
