
#include "CombatSystem/Abilities/NpcAbilityHandler.h"
#include "UnrealNetwork.h"
#include "DamageHandler.h"
#include "ResourceHandler.h"
#include "CrowdControlHandler.h"

void UNpcAbilityHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNpcAbilityHandler, CastingState);
}

bool UNpcAbilityHandler::GetIsCasting() const
{
	return CastingState.bIsCasting;
}

float UNpcAbilityHandler::GetCurrentCastLength() const
{
	return CastingState.bIsCasting ? CastingState.CastEndTime - CastingState.CastStartTime : -1.0f;
}

float UNpcAbilityHandler::GetCastTimeRemaining() const
{
	return CastingState.bIsCasting ? CastingState.CastEndTime - GetGameState()->GetServerWorldTimeSeconds() : -1.0f;
}

bool UNpcAbilityHandler::GetIsInterruptible() const
{
	return CastingState.bIsCasting && CastingState.bInterruptible;
}

UCombatAbility* UNpcAbilityHandler::GetCurrentCastAbility() const
{
	return CastingState.bIsCasting ? CastingState.CurrentCast : nullptr;
}

bool UNpcAbilityHandler::GetGlobalCooldownActive() const
{
	return GlobalCooldown.bGlobalCooldownActive;
}

float UNpcAbilityHandler::GetCurrentGlobalCooldownLength() const
{
	return GlobalCooldown.bGlobalCooldownActive ? GlobalCooldown.EndTime - GlobalCooldown.StartTime : -1.0f; 
}

float UNpcAbilityHandler::GetGlobalCooldownTimeRemaining() const
{
	return GlobalCooldown.bGlobalCooldownActive ? GlobalCooldown.EndTime - GetGameState()->GetServerWorldTimeSeconds() : -1.0f;
}

FCastEvent UNpcAbilityHandler::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FCastEvent Result;

	if (GetOwnerRole() != ROLE_Authority)
	{
		Result.FailReason = FString(TEXT("Tried to initiate an ability without authority."));
		return Result;
	}
	
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
		return Result;
	}
	
	if (CastingState.bIsCasting)
	{
		Result.FailReason = FString(TEXT("Already casting."));
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
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		StartGlobalCooldown(Result.Ability);
	}
	
	Result.Ability->CommitCharges();
	
	if (Costs.Num() > 0 && IsValid(GetResourceHandler()))
	{
		GetResourceHandler()->CommitAbilityCosts(Result.Ability, Costs);
	}

	FCombatParameters BroadcastParams;
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = FString(TEXT("No cast type was set."));
		return Result;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->ServerNonPredictedTick(0, BroadcastParams);
		FireOnAbilityTick(Result);
		BroadcastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		StartCast(Result.Ability);
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerNonPredictedTick(0, BroadcastParams);
			FireOnAbilityTick(Result);
			BroadcastAbilityTick(Result, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = FString(TEXT("Defaulted on cast type."));
		return Result;
	}
	
	return Result;
}

FCancelEvent UNpcAbilityHandler::CancelCurrentCast()
{
	FCancelEvent Result;
	if (GetOwnerRole() != ROLE_Authority)
	{
		Result.FailReason = FString(TEXT("Tried to cancel without authority."));
		return Result;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		Result.FailReason = FString(TEXT("Tried to cancel without a valid cast."));
		return Result;
	}
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GetGameState()->GetServerWorldTimeSeconds();
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	FCombatParameters CancelParams;
	Result.CancelledAbility->ServerNonPredictedCancel(CancelParams);
	FireOnAbilityCancelled(Result);
	BroadcastAbilityCancel(Result, CancelParams);
	EndCast();
	return Result;
}

FInterruptEvent UNpcAbilityHandler::InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource,
	bool const bIgnoreRestrictions)
{
	FInterruptEvent Result;
	if (GetOwnerRole() != ROLE_Authority)
	{
		Result.FailReason = FString(TEXT("Tried to interrupt without authority."));
		return Result;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		Result.FailReason = FString(TEXT("Tried to interrupt without a valid cast."));
		return Result;
	}
	
	Result.InterruptAppliedBy = AppliedBy;
	Result.InterruptSource = InterruptSource;
	Result.InterruptAppliedTo = GetOwner();
	Result.InterruptedAbility = CastingState.CurrentCast;
	Result.InterruptedCastStart = CastingState.CastStartTime;
	Result.InterruptedCastEnd = CastingState.CastEndTime;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.InterruptTime = GetGameState()->GetServerWorldTimeSeconds();
	
	if (!bIgnoreRestrictions)
	{
		if (!CastingState.CurrentCast->GetInterruptible() || CheckInterruptRestricted(Result))
		{
			Result.FailReason = FString(TEXT("Cast was not interruptible."));
			return Result;
		}
	}
	
	Result.bSuccess = true;
	Result.InterruptedAbility->InterruptCast(Result);
	FireOnAbilityInterrupted(Result);
	BroadcastAbilityInterrupt(Result);
	EndCast();
	
	return Result;
}

void UNpcAbilityHandler::OnRep_CastingState(FCastingState const& PreviousState)
{
	FireOnCastStateChanged(PreviousState, CastingState);
}

void UNpcAbilityHandler::StartGlobalCooldown(UCombatAbility* Ability)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	FGlobalCooldown const PreviousGlobal = GlobalCooldown;
	GlobalCooldown.bGlobalCooldownActive = true;
	GlobalCooldown.StartTime = GetGameState()->GetServerWorldTimeSeconds();
	float const GlobalLength = CalculateGlobalCooldownLength(Ability);
	GlobalCooldown.EndTime = GlobalCooldown.StartTime + GlobalLength;
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UNpcAbilityHandler::EndGlobalCooldown, GlobalLength, false);
	FireOnGlobalCooldownChanged(PreviousGlobal, GlobalCooldown);
}

void UNpcAbilityHandler::EndGlobalCooldown()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (GlobalCooldown.bGlobalCooldownActive)
	{
		FGlobalCooldown const PreviousGlobal;
		GlobalCooldown.bGlobalCooldownActive = false;
		GlobalCooldown.StartTime = 0.0f;
		GlobalCooldown.EndTime = 0.0f;
		GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
		FireOnGlobalCooldownChanged(PreviousGlobal, GlobalCooldown);
	}
}

void UNpcAbilityHandler::StartCast(UCombatAbility* Ability)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CastStartTime = GetGameState()->GetServerWorldTimeSeconds();
	float const CastLength = CalculateCastLength(Ability);
	CastingState.CastEndTime = CastingState.CastStartTime + CastLength;
	CastingState.bInterruptible = Ability->GetInterruptible();
	GetWorld()->GetTimerManager().SetTimer(CastHandle, this, &UNpcAbilityHandler::CompleteCast, CastLength, false);
	GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UNpcAbilityHandler::TickCast, (CastLength / Ability->GetNumberOfTicks()), true);
	FireOnCastStateChanged(PreviousState, CastingState);
}

void UNpcAbilityHandler::CompleteCast()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		return;
	}
	FCastEvent CompletionEvent;
	CompletionEvent.Ability = CastingState.CurrentCast;
	CompletionEvent.ActionTaken = ECastAction::Complete;
	CompletionEvent.Tick = CastingState.ElapsedTicks;
	CastingState.CurrentCast->CompleteCast();
	FireOnAbilityComplete(CastingState.CurrentCast);
	BroadcastAbilityComplete(CompletionEvent);
	if (!GetWorld()->GetTimerManager().IsTimerActive(TickHandle))
	{
		EndCast();
	}
}

void UNpcAbilityHandler::TickCast()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		return;
	}
	CastingState.ElapsedTicks++;
	FCombatParameters BroadcastParams;
	CastingState.CurrentCast->ServerNonPredictedTick(CastingState.ElapsedTicks, BroadcastParams);
	FCastEvent TickEvent;
	TickEvent.ActionTaken = ECastAction::Tick;
	TickEvent.Ability = CastingState.CurrentCast;
	TickEvent.Tick = CastingState.ElapsedTicks;
	FireOnAbilityTick(TickEvent);
	BroadcastAbilityTick(TickEvent, BroadcastParams);
	if (CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNumberOfTicks())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		if (!GetWorld()->GetTimerManager().IsTimerActive(CastHandle))
		{
			EndCast();
		}
	}
}

void UNpcAbilityHandler::EndCast()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = false;
	CastingState.CurrentCast = nullptr;
	CastingState.bInterruptible = false;
	CastingState.ElapsedTicks = 0;
	CastingState.CastStartTime = 0.0f;
	CastingState.CastEndTime = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	GetWorld()->GetTimerManager().ClearTimer(TickHandle);
	FireOnCastStateChanged(PreviousState, CastingState);
}

void UNpcAbilityHandler::BroadcastAbilityInterrupt_Implementation(FInterruptEvent const& InterruptEvent)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(InterruptEvent.InterruptedAbility))
		{
			//TODO: Simulated Interrupt?
			InterruptEvent.InterruptedAbility->InterruptCast(InterruptEvent);
			FireOnAbilityInterrupted(InterruptEvent);
		}
	}
}

void UNpcAbilityHandler::BroadcastAbilityCancel_Implementation(FCancelEvent const& CancelEvent, FCombatParameters const& Params)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(CancelEvent.CancelledAbility))
		{
			CancelEvent.CancelledAbility->SimulatedCancel(Params);
			FireOnAbilityCancelled(CancelEvent);
		}
	}
}

void UNpcAbilityHandler::BroadcastAbilityComplete_Implementation(FCastEvent const& CastEvent)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(CastEvent.Ability))
		{
			CastEvent.Ability->CompleteCast();
			FireOnAbilityComplete(CastEvent.Ability);
		}
	}
}

void UNpcAbilityHandler::BroadcastAbilityTick_Implementation(FCastEvent const& CastEvent, FCombatParameters const& Params)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(CastEvent.Ability))
		{
			CastEvent.Ability->SimulatedTick(CastEvent.Tick, Params);
			FireOnAbilityTick(CastEvent);
		}
	}
}
