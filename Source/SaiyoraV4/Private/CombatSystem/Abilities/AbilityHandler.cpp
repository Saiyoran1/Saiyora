// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityHandler.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ResourceHandler.h"
#include "StatHandler.h"
#include "CrowdControlHandler.h"
#include "SaiyoraCombatInterface.h"
#include "DamageHandler.h"
#include "UObjectGlobals.h"

const float UAbilityHandler::MinimumGlobalCooldownLength = 0.5f;
const float UAbilityHandler::MinimumCastLength = 0.5f;
const float UAbilityHandler::MinimumCooldownLength = 0.5f;
const float UAbilityHandler::AbilityQueWindowSec = 0.2f;
const FGameplayTag UAbilityHandler::CastLengthStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CastLength")), false);
const FGameplayTag UAbilityHandler::GlobalCooldownLengthStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GlobalCooldownLength")), false);
const FGameplayTag UAbilityHandler::CooldownLengthStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CooldownLength")), false);

#pragma region SetupFunctions
UAbilityHandler::UAbilityHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAbilityHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UAbilityHandler, CastingState, COND_SkipOwner);
}

bool UAbilityHandler::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	
	bWroteSomething |= Channel->ReplicateSubobjectList(ActiveAbilities, *Bunch, *RepFlags);
	bWroteSomething |= Channel->ReplicateSubobjectList(RecentlyRemovedAbilities, *Bunch, *RepFlags);
	return bWroteSomething;
}

void UAbilityHandler::BeginPlay()
{
	Super::BeginPlay();
	
	GameStateRef = GetWorld()->GetGameState<ASaiyoraGameState>();
	if (!GameStateRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Game State Ref in Ability Component."));
	}
	if (GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		ResourceHandler = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwner());
		StatHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
		CrowdControlHandler = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwner());
		DamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(StatHandler))
		{
			FAbilityModCondition StatCooldownMod;
			StatCooldownMod.BindUFunction(this, FName(TEXT("ModifyCooldownFromStat")));
			AddCooldownModifier(StatCooldownMod);
			FAbilityModCondition StatGlobalCooldownMod;
			StatGlobalCooldownMod.BindUFunction(this, FName(TEXT("ModifyGlobalCooldownFromStat")));
			AddGlobalCooldownModifier(StatGlobalCooldownMod);
			FAbilityModCondition StatCastLengthMod;
			StatCastLengthMod.BindUFunction(this, FName(TEXT("ModifyCastLengthFromStat")));
			AddCastLengthModifier(StatCastLengthMod);
		}
		if (IsValid(DamageHandler))
		{
			FLifeStatusCallback DeathCallback;
			DeathCallback.BindUFunction(this, FName(TEXT("CancelCastOnDeath")));
			DamageHandler->SubscribeToLifeStatusChanged(DeathCallback);
		}
		if (IsValid(CrowdControlHandler))
		{
			FCrowdControlCallback CcCallback;
			CcCallback.BindUFunction(this, FName(TEXT("CancelCastOnCrowdControl")));
			CrowdControlHandler->SubscribeToCrowdControlChanged(CcCallback);
		}
		for (TSubclassOf<UCombatAbility> const AbilityClass : DefaultAbilities)
		{
			AddNewAbility(AbilityClass);
		}
	}
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FGlobalCooldownCallback GcdQueueCallback;
		GcdQueueCallback.BindUFunction(this, FName(TEXT("CheckForQueuedAbilityOnGlobalEnd")));
		SubscribeToGlobalCooldownChanged(GcdQueueCallback);
		FCastingStateCallback CastQueueCallback;
		CastQueueCallback.BindUFunction(this, FName(TEXT("CheckForQueuedAbilityOnCastEnd")));
		SubscribeToCastStateChanged(CastQueueCallback);
	}
}
#pragma endregion
#pragma region IntercomponentDelegates
//INTERCOMPONENT DELEGATES

FCombatModifier UAbilityHandler::ModifyCooldownFromStat(UCombatAbility* Ability)
{
	FCombatModifier Mod;
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(CooldownLengthStatTag))
		{
			Mod.ModifierType = EModifierType::Multiplicative;
			Mod.ModifierValue = StatHandler->GetStatValue(CooldownLengthStatTag);
		}
	}
	return Mod;
}

FCombatModifier UAbilityHandler::ModifyGlobalCooldownFromStat(UCombatAbility* Ability)
{
	FCombatModifier Mod;
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(GlobalCooldownLengthStatTag))
		{
			Mod.ModifierType = EModifierType::Multiplicative;
			Mod.ModifierValue = StatHandler->GetStatValue(GlobalCooldownLengthStatTag);
		}
	}
	return Mod;
}

FCombatModifier UAbilityHandler::ModifyCastLengthFromStat(UCombatAbility* Ability)
{
	FCombatModifier Mod;
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(CastLengthStatTag))
		{
			Mod.ModifierType = EModifierType::Multiplicative;
			Mod.ModifierValue = StatHandler->GetStatValue(CastLengthStatTag);
		}
	}
	return Mod;
}

void UAbilityHandler::CancelCastOnDeath(ELifeStatus PreviousStatus, ELifeStatus NewStatus)
{
	if (GetCastingState().bIsCasting)
	{
		switch (NewStatus)
		{
			case ELifeStatus::Invalid :
				CancelCurrentCast();
				break;
			case ELifeStatus::Alive :
				break;
			case ELifeStatus::Dead :
				if (!GetCastingState().CurrentCast->GetCastableWhileDead())
				{
					CancelCurrentCast();
					return;
				}
				break;
			default :
				break;
		}
	}
}

void UAbilityHandler::CancelCastOnCrowdControl(FCrowdControlStatus const& PreviousStatus, FCrowdControlStatus const& NewStatus)
{
	if (GetCastingState().bIsCasting && NewStatus.bActive == true)
	{
		if (GetCastingState().CurrentCast->GetRestrictedCrowdControls().Contains(NewStatus.CrowdControlClass))
		{
			CancelCurrentCast();
		}
	}
}
#pragma endregion
#pragma region AbilityManagement
//ABILITY MANAGEMENT

UCombatAbility* UAbilityHandler::AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return nullptr;
	}
	if (IsValid(FindActiveAbility(AbilityClass)))
	{
		return nullptr;
	}
	UCombatAbility* NewAbility = NewObject<UCombatAbility>(GetOwner(), AbilityClass);
	if (IsValid(NewAbility))
	{
		NewAbility->InitializeAbility(this);
		ActiveAbilities.Add(NewAbility);
		OnAbilityAdded.Broadcast(NewAbility);
	}
	return NewAbility;
}

void UAbilityHandler::NotifyOfReplicatedAddedAbility(UCombatAbility* NewAbility)
{
	if (IsValid(NewAbility))
	{
		if (IsValid(FindActiveAbility(NewAbility->GetClass())))
		{
			return;
		}
		NewAbility->InitializeAbility(this);
		ActiveAbilities.Add(NewAbility);
		OnAbilityAdded.Broadcast(NewAbility);
	}
}

void UAbilityHandler::RemoveAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	UCombatAbility* AbilityToRemove = FindActiveAbility(AbilityClass);
	if (!IsValid(AbilityToRemove))
	{
		return;
	}
	AbilityToRemove->DeactivateAbility();
	RecentlyRemovedAbilities.Add(AbilityToRemove);
	ActiveAbilities.RemoveSingleSwap(AbilityToRemove);
	OnAbilityRemoved.Broadcast(AbilityToRemove);
	FTimerHandle RemovalHandle;
	FTimerDelegate RemovalDelegate;
	RemovalDelegate.BindUFunction(this, FName(TEXT("CleanupRemovedAbility")), AbilityToRemove);
	GetWorld()->GetTimerManager().SetTimer(RemovalHandle, RemovalDelegate, 1.0f, false);
}

void UAbilityHandler::NotifyOfReplicatedRemovedAbility(UCombatAbility* RemovedAbility)
{
	if (ActiveAbilities.RemoveSingleSwap(RemovedAbility) > 0)
	{
		OnAbilityRemoved.Broadcast(RemovedAbility);
	}
}

void UAbilityHandler::CleanupRemovedAbility(UCombatAbility* Ability)
{
	RecentlyRemovedAbilities.RemoveSingleSwap(Ability);
}

UCombatAbility* UAbilityHandler::FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass))
	{
		return nullptr;
	}
	for (UCombatAbility* Ability : ActiveAbilities)
	{
		if (IsValid(Ability) && Ability->IsA(AbilityClass) && Ability->GetInitialized() && !Ability->GetDeactivated())
		{
			return Ability;
		}
	}
	return nullptr;
}

void UAbilityHandler::AddAbilityRestriction(FAbilityRestriction const& Restriction)
{
	if (!Restriction.IsBound())
	{
		return;
	}
	AbilityRestrictions.AddUnique(Restriction);
}

void UAbilityHandler::RemoveAbilityRestriction(FAbilityRestriction const& Restriction)
{
	if (!Restriction.IsBound())
	{
		return;
	}
	AbilityRestrictions.RemoveSingleSwap(Restriction);
}

bool UAbilityHandler::CheckAbilityRestricted(UCombatAbility* Ability)
{
	for (FAbilityRestriction const& Restriction : AbilityRestrictions)
	{
		if (Restriction.Execute(Ability))
		{
			return true;
		}
	}
	return false;
}
#pragma endregion
#pragma region SubscriptionsAndCallbacks
//SUBSCRIPTION AND CALLBACKS

void UAbilityHandler::SubscribeToAbilityAdded(FAbilityInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityAdded.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityAdded(FAbilityInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityAdded.Remove(Callback);
}

void UAbilityHandler::SubscribeToAbilityRemoved(FAbilityInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityRemoved.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityRemoved(FAbilityInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityRemoved.Remove(Callback);
}

void UAbilityHandler::SubscribeToAbilityTicked(FAbilityCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityTick.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityTicked(FAbilityCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityTick.Remove(Callback);
}

void UAbilityHandler::SubscribeToAbilityCompleted(FAbilityInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityComplete.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityCompleted(FAbilityInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityComplete.Remove(Callback);
}

void UAbilityHandler::SubscribeToAbilityCancelled(FAbilityCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityCancelled.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityCancelled(FAbilityCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityCancelled.Remove(Callback);
}

void UAbilityHandler::SubscribeToAbilityInterrupted(FInterruptCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityInterrupted.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityInterrupted(FInterruptCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityInterrupted.Remove(Callback);
}

void UAbilityHandler::SubscribeToCastStateChanged(FCastingStateCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnCastStateChanged.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromCastStateChanged(FCastingStateCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnCastStateChanged.Remove(Callback);
}

void UAbilityHandler::SubscribeToGlobalCooldownChanged(FGlobalCooldownCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnGlobalCooldownChanged.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromGlobalCooldownChanged(FGlobalCooldownCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnGlobalCooldownChanged.Remove(Callback);
}

void UAbilityHandler::BroadcastAbilityComplete_Implementation(FCastEvent const& CastEvent)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		if (IsValid(CastEvent.Ability))
		{
			CastEvent.Ability->CompleteCast();
			OnAbilityComplete.Broadcast(CastEvent.Ability);
		}
	}
}

void UAbilityHandler::BroadcastAbilityCancel_Implementation(FCancelEvent const& CancelEvent)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		if (IsValid(CancelEvent.CancelledAbility))
		{
			CancelEvent.CancelledAbility->CancelCast();
			OnAbilityCancelled.Broadcast(CancelEvent);
		}
	}
}

void UAbilityHandler::BroadcastAbilityInterrupt_Implementation(FInterruptEvent const& InterruptEvent)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		if (IsValid(InterruptEvent.InterruptedAbility))
		{
			InterruptEvent.InterruptedAbility->InterruptCast(InterruptEvent);
			OnAbilityInterrupted.Broadcast(InterruptEvent);
		}
	}
}

void UAbilityHandler::BroadcastAbilityTick_Implementation(FCastEvent const& CastEvent, bool const OwnerNeeds, FCombatParameters const& BroadcastParams)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy || (OwnerNeeds && GetOwnerRole() == ROLE_AutonomousProxy))
	{
		if (IsValid(CastEvent.Ability))
		{
			CastEvent.Ability->SimulatedTick(CastEvent.Tick, BroadcastParams);
			OnAbilityTick.Broadcast(CastEvent);
		}
	}
}
#pragma endregion
#pragma region Casting
//CASTING

void UAbilityHandler::AddCastLengthModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return;
	}
	CastLengthMods.AddUnique(Modifier);
}

void UAbilityHandler::RemoveCastLengthModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return;
	}
	CastLengthMods.RemoveSingleSwap(Modifier);
}

float UAbilityHandler::CalculateCastLength(UCombatAbility* Ability)
{
	if (Ability->GetCastType() != EAbilityCastType::Channel)
	{
		return 0.0f;
	}
	float CastLength = Ability->GetDefaultCastLength();
	if (!Ability->HasStaticCastLength())
	{
		TArray<FCombatModifier> Mods;
		for (FAbilityModCondition const& Mod : CastLengthMods)
		{
			Mods.Add(Mod.Execute(Ability));
		}
		CastLength = FCombatModifier::CombineModifiers(Mods, CastLength);
	}
	CastLength = FMath::Max(MinimumCastLength, CastLength);
	return CastLength;
}

void UAbilityHandler::AuthStartCast(UCombatAbility* Ability, bool const bPredicted, int32 const PredictionID)
{
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CastStartTime = GameStateRef->GetWorldTime();
	CastingState.CastEndTime = CastingState.CastStartTime + CalculateCastLength(Ability);
	CastingState.bInterruptible = Ability->GetInterruptible();
	if (bPredicted && PredictionID != 0)
	{
		CastingState.PredictionID = PredictionID;
	}
	FTimerDelegate CastDelegate;
	CastDelegate.BindUFunction(this, FName(TEXT("CompleteCast")));
	GetWorld()->GetTimerManager().SetTimer(CastHandle, CastDelegate, CastingState.CastEndTime - GameStateRef->GetWorldTime(), false);
	FTimerDelegate TickDelegate;
	if (bPredicted)
	{
		TickDelegate.BindUFunction(this, FName(TEXT("AuthTickPredictedCast")));
	}
	else
	{
		TickDelegate.BindUFunction(this, FName(TEXT("TickCurrentAbility")));
	}
	GetWorld()->GetTimerManager().SetTimer(TickHandle, TickDelegate, (CastingState.CastEndTime - CastingState.CastStartTime) / Ability->GetNumberOfTicks(), true);
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

//Server tick of a non-predicted cast.
void UAbilityHandler::TickCurrentAbility()
{
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		return;
	}
	CastingState.ElapsedTicks++;
	FCombatParameters BroadcastParams;
	CastingState.CurrentCast->ServerNonPredictedTick(CastingState.ElapsedTicks, BroadcastParams);
	FCastEvent TickEvent;
	TickEvent.Ability = CastingState.CurrentCast;
	TickEvent.Tick = CastingState.ElapsedTicks;
	TickEvent.ActionTaken = ECastAction::Tick;
	TickEvent.PredictionID = CastingState.PredictionID;
	OnAbilityTick.Broadcast(TickEvent);
	BroadcastAbilityTick(TickEvent, true, BroadcastParams);
	if (CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNumberOfTicks())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		if (!GetWorld()->GetTimerManager().IsTimerActive(CastHandle))
		{
			EndCast();
		}
	}
}

void UAbilityHandler::CompleteCast()
{
	FCastEvent CompletionEvent;
	CompletionEvent.Ability = CastingState.CurrentCast;
	CompletionEvent.PredictionID = CastingState.PredictionID;
	CompletionEvent.ActionTaken = ECastAction::Complete;
	CompletionEvent.Tick = CastingState.ElapsedTicks;
	if (IsValid(CompletionEvent.Ability))
	{
		GetCastingState().CurrentCast->CompleteCast();
		OnAbilityComplete.Broadcast(CompletionEvent.Ability);
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

FCancelEvent UAbilityHandler::CancelCurrentCast()
{
	FCancelEvent Result;
	Result.CancelledAbility = GetCastingState().CurrentCast;
	if (GetCastingState().bIsCasting && IsValid(Result.CancelledAbility))
	{
		Result.CancelTime = GameStateRef->GetWorldTime();
		Result.CancelID = 0;
		Result.CancelledCastStart = GetCastingState().CastStartTime;
		Result.CancelledCastEnd = GetCastingState().CastEndTime;
		Result.CancelledCastID = GetCastingState().PredictionID;
		Result.ElapsedTicks = GetCastingState().ElapsedTicks;
		Result.CancelledAbility->CancelCast();
		OnAbilityCancelled.Broadcast(Result);
		if (GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy)
		{
			ClientCancelCast(Result);
		}
		BroadcastAbilityCancel(Result);
		EndCast();
	}
	return Result;
}

void UAbilityHandler::ClientCancelCast_Implementation(FCancelEvent const& CancelEvent)
{
	if (!CastingState.bIsCasting || CastingState.PredictionID > CancelEvent.CancelledCastID || !IsValid(CastingState.CurrentCast) || CastingState.CurrentCast != CancelEvent.CancelledAbility)
	{
		//We have already moved on.
		return;
	}
	CastingState.CurrentCast->CancelCast();
	OnAbilityCancelled.Broadcast(CancelEvent);
	EndCast();
}

FInterruptEvent UAbilityHandler::InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource)
{
	FInterruptEvent Result;
	Result.InterruptAppliedBy = AppliedBy;
	Result.InterruptSource = InterruptSource;
	Result.InterruptAppliedTo = GetOwner();
	Result.InterruptedAbility = GetCastingState().CurrentCast;
	
	if (!GetCastingState().bIsCasting || !IsValid(Result.InterruptedAbility))
	{
		Result.FailReason = FString(TEXT("Not currently casting."));
		return Result;
	}
	if (!GetCastingState().bInterruptible)
	{
		Result.FailReason = FString(TEXT("Cast was not interruptible."));
		return Result;
	}

	Result.InterruptedCastStart = GetCastingState().CastStartTime;
	Result.InterruptedCastEnd = GetCastingState().CastEndTime;
	Result.InterruptedAbility = GetCastingState().CurrentCast;
	Result.ElapsedTicks = GetCastingState().ElapsedTicks;
	Result.CancelledCastID = GetCastingState().PredictionID;
	Result.InterruptTime = GetGameStateRef()->GetWorldTime();
	Result.bSuccess = true;
	Result.InterruptedAbility->InterruptCast(Result);
	OnAbilityInterrupted.Broadcast(Result);
	if (GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy)
	{
		ClientInterruptCast(Result);
	}
	BroadcastAbilityInterrupt(Result);
	EndCast();
	
	return Result;
}

void UAbilityHandler::ClientInterruptCast_Implementation(FInterruptEvent const& InterruptEvent)
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

void UAbilityHandler::EndCast()
{
	FCastingState const PreviousState = GetCastingState();
	CastingState.bIsCasting = false;
	CastingState.CurrentCast = nullptr;
	CastingState.bInterruptible = false;
	CastingState.ElapsedTicks = 0;
	CastingState.CastStartTime = 0.0f;
	CastingState.CastEndTime = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	GetWorld()->GetTimerManager().ClearTimer(TickHandle);
	OnCastStateChanged.Broadcast(PreviousState, GetCastingState());
}

void UAbilityHandler::OnRep_CastingState(FCastingState const& PreviousState)
{
	if (PreviousState.bIsCasting != CastingState.bIsCasting
        || PreviousState.CurrentCast != CastingState.CurrentCast
        || PreviousState.CastStartTime != CastingState.CastStartTime
        || PreviousState.CastEndTime != CastingState.CastEndTime)
	{
		OnCastStateChanged.Broadcast(PreviousState, CastingState);
	}
}
#pragma endregion
#pragma region AbilityCooldown
//ABILITY COOLDOWN

void UAbilityHandler::AddCooldownModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return;
	}
	CooldownLengthMods.AddUnique(Modifier);
}

void UAbilityHandler::RemoveCooldownModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return;
	}
	CooldownLengthMods.RemoveSingleSwap(Modifier);
}

float UAbilityHandler::CalculateAbilityCooldown(UCombatAbility* Ability)
{
	float CooldownLength = Ability->GetDefaultCooldown();
	if (!Ability->GetHasStaticCooldown())
	{
		TArray<FCombatModifier> ModArray;
		for (FAbilityModCondition const& Condition : CooldownLengthMods)
		{
			ModArray.Add(Condition.Execute(Ability));
		}
		CooldownLength = FCombatModifier::CombineModifiers(ModArray, CooldownLength);
	}
	CooldownLength = FMath::Max(CooldownLength, MinimumCooldownLength);
	return CooldownLength;
}
#pragma endregion
#pragma region GlobalCooldown
//GLOBAL COOLDOWN

void UAbilityHandler::AddGlobalCooldownModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return;
	}
	GlobalCooldownMods.AddUnique(Modifier);
}

void UAbilityHandler::RemoveGlobalCooldownModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return;
	}
	GlobalCooldownMods.RemoveSingleSwap(Modifier);
}

float UAbilityHandler::CalculateGlobalCooldownLength(UCombatAbility* Ability)
{
	float GlobalLength = Ability->GetDefaultGlobalCooldownLength();
	if (!Ability->HasStaticGlobalCooldown())
	{
		TArray<FCombatModifier> Mods;
		for (FAbilityModCondition const& Mod : GlobalCooldownMods)
		{
			Mods.Add(Mod.Execute(Ability));
		}
		GlobalLength = FCombatModifier::CombineModifiers(Mods, GlobalLength);
	}
	GlobalLength = FMath::Max(MinimumGlobalCooldownLength, GlobalLength);
	return GlobalLength;
}

void UAbilityHandler::AuthStartGlobal(UCombatAbility* Ability)
{
	FGlobalCooldown const PreviousGlobal = GlobalCooldownState;
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.StartTime = GameStateRef->GetWorldTime();
	GlobalCooldownState.EndTime = GlobalCooldownState.StartTime + CalculateGlobalCooldownLength(Ability);
	FTimerDelegate GCDDelegate;
	GCDDelegate.BindUFunction(this, FName(TEXT("EndGlobalCooldown")));
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, GCDDelegate, GlobalCooldownState.EndTime - GameStateRef->GetWorldTime(), false);
	OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
}

void UAbilityHandler::EndGlobalCooldown()
{
	if (GlobalCooldownState.bGlobalCooldownActive)
	{
		FGlobalCooldown const PreviousGlobal;
		GlobalCooldownState.bGlobalCooldownActive = false;
		GlobalCooldownState.StartTime = 0.0f;
		GlobalCooldownState.EndTime = 0.0f;
		GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
		OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
	}
}
#pragma endregion
#pragma region AbilityUsage
//ABILITY USAGE

FCastEvent UAbilityHandler::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FCastEvent Result;
	switch (GetOwnerRole())
	{
	case ROLE_Authority :
		return AuthUseAbility(AbilityClass);
	case ROLE_AutonomousProxy :
		return PredictUseAbility(AbilityClass, false);
	case ROLE_SimulatedProxy :
		Result.FailReason = FString(TEXT("Tried to use ability from simulated proxy."));
		return Result;
	default :
		Result.FailReason = FString(TEXT("GetOwnerRole defaulted."));
		return Result;
	}
}

FCastEvent UAbilityHandler::AuthUseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FCastEvent Result;
	Result.PredictionID = 0;

	if (GetOwner()->GetRemoteRole() != ROLE_SimulatedProxy)
	{
		Result.FailReason = FString(TEXT("Tried to initiate an ability from server with an actor that isn't server-owned."));
		return Result;
	}
	
	Result.Ability = FindActiveAbility(AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Did not find valid active ability."));
		return Result;
	}

	if (!Result.Ability->GetCastableWhileDead() && IsValid(DamageHandler) && DamageHandler->GetLifeStatus() != ELifeStatus::Alive)
	{
		Result.FailReason = FString(TEXT("Cannot activate while dead."));
		return Result;
	}
	
	if (Result.Ability->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive)
	{
		Result.FailReason = FString(TEXT("Already on global cooldown."));
		return Result;
	}
	
	if (GetCastingState().bIsCasting)
	{
		Result.FailReason = FString(TEXT("Already casting."));
		return Result;
	}
	
	TArray<FAbilityCost> Costs;
	Result.Ability->GetAbilityCosts(Costs);
	if (Costs.Num() > 0)
	{
		if (!IsValid(ResourceHandler))
		{
			Result.FailReason = FString(TEXT("No valid resource handler found."));
			return Result;
		}
		if (!ResourceHandler->CheckAbilityCostsMet(Result.Ability, Costs))
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

	if (IsValid(CrowdControlHandler))
	{
		for (TSubclassOf<UCrowdControl> const CcClass : Result.Ability->GetRestrictedCrowdControls())
		{
			if (CrowdControlHandler->GetActiveCcTypes().Contains(CcClass))
			{
				Result.FailReason = FString(TEXT("Cast restricted by crowd control."));
				return Result;
			}
		}
	}
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		AuthStartGlobal(Result.Ability);
	}
	
	Result.Ability->CommitCharges(Result.PredictionID);
	
	if (Costs.Num() > 0 && IsValid(ResourceHandler))
	{
		ResourceHandler->AuthCommitAbilityCosts(Result.Ability, Result.PredictionID, Costs);
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
		OnAbilityTick.Broadcast(Result);
		BroadcastAbilityTick(Result, true, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		AuthStartCast(Result.Ability, false, 0);
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerNonPredictedTick(0, BroadcastParams);
			OnAbilityTick.Broadcast(Result);
			BroadcastAbilityTick(Result, true, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = FString(TEXT("Defaulted on cast type."));
		return Result;
	}
	
	return Result;
}

#pragma endregion
#pragma region Prediction

void UAbilityHandler::GenerateNewPredictionID(FCastEvent& CastEvent)
{
	ClientPredictionID++;
	CastEvent.PredictionID = ClientPredictionID;
}

FCastEvent UAbilityHandler::PredictUseAbility(TSubclassOf<UCombatAbility> const AbilityClass, bool const bFromQueue)
{
	FCastEvent Result;
	
	Result.Ability = FindActiveAbility(AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Did not find valid active ability."));
		return Result;
	}

	if (!Result.Ability->GetCastableWhileDead() && IsValid(DamageHandler) && DamageHandler->GetLifeStatus() != ELifeStatus::Alive)
	{
		Result.FailReason = FString(TEXT("Cannot activate while dead."));
		return Result;
	}
	
	if (Result.Ability->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive)
	{
		Result.FailReason = FString(TEXT("Already on global cooldown."));
		if (!bFromQueue)
		{
			TryQueueAbility(AbilityClass);
		}
		return Result;
	}
	
	if (GetCastingState().bIsCasting)
	{
		Result.FailReason = FString(TEXT("Already casting."));
		if (!bFromQueue)
		{
			TryQueueAbility(AbilityClass);
		}
		return Result;
	}
	
	TArray<FAbilityCost> Costs;
	Result.Ability->GetAbilityCosts(Costs);
	if (Costs.Num() > 0)
	{
		if (!IsValid(ResourceHandler))
		{
			Result.FailReason = FString(TEXT("No valid resource handler found."));
			return Result;
		}
		if (!ResourceHandler->CheckAbilityCostsMet(Result.Ability, Costs))
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

	if (IsValid(CrowdControlHandler))
	{
		for (TSubclassOf<UCrowdControl> const CcClass : Result.Ability->GetRestrictedCrowdControls())
		{
			if (CrowdControlHandler->GetActiveCcTypes().Contains(CcClass))
			{
				Result.FailReason = FString(TEXT("Cast restricted by crowd control."));
				return Result;
			}
		}
	}

	GenerateNewPredictionID(Result);

	FAbilityRequest AbilityRequest;
	AbilityRequest.AbilityClass = AbilityClass;
	AbilityRequest.PredictionID = Result.PredictionID;
	AbilityRequest.ClientStartTime = GameStateRef->GetWorldTime();

	FClientAbilityPrediction AbilityPrediction;
	AbilityPrediction.Ability = Result.Ability;
	AbilityPrediction.PredictionID = Result.PredictionID;
	AbilityPrediction.ClientTime = GameStateRef->GetWorldTime();
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		PredictStartGlobal(Result.PredictionID);
		AbilityPrediction.bPredictedGCD = true;
	}

	Result.Ability->PredictCommitCharges(Result.PredictionID);
	AbilityPrediction.bPredictedCharges = true;
	
	if (Costs.Num() > 0 && IsValid(ResourceHandler))
	{
		ResourceHandler->PredictCommitAbilityCosts(Result.Ability, Result.PredictionID, Costs);
		for (FAbilityCost const& Cost : Costs)
		{
			AbilityPrediction.PredictedCostTags.AddTag(Cost.ResourceTag);
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
		Result.FailReason = FString(TEXT("Defaulted on cast type."));
		return Result;
	}
	
	UnackedAbilityPredictions.Add(AbilityPrediction.PredictionID, AbilityPrediction);
	ServerPredictAbility(AbilityRequest);
	
	return Result;
}

void UAbilityHandler::ServerPredictAbility_Implementation(FAbilityRequest const& AbilityRequest)
{
	FCastEvent Result;
	Result.PredictionID = AbilityRequest.PredictionID;
	
	Result.Ability = FindActiveAbility(AbilityRequest.AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Did not find valid active ability."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	}

	if (!Result.Ability->GetCastableWhileDead() && IsValid(DamageHandler) && DamageHandler->GetLifeStatus() != ELifeStatus::Alive)
	{
		Result.FailReason = FString(TEXT("Cannot activate while dead."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	}
	
	if (Result.Ability->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive)
	{
		Result.FailReason = FString(TEXT("Already on global cooldown."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	}
	
	if (GetCastingState().bIsCasting)
	{
		Result.FailReason = FString(TEXT("Already casting."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	}

	TArray<FAbilityCost> Costs;
	Result.Ability->GetAbilityCosts(Costs);
	if (Costs.Num() > 0)
	{
		if (!IsValid(ResourceHandler))
		{
			Result.FailReason = FString(TEXT("No valid resource handler found."));
			ClientFailPredictedAbility(AbilityRequest.PredictionID);
			return;
		}
		if (!ResourceHandler->CheckAbilityCostsMet(Result.Ability, Costs))
		{
			Result.FailReason = FString(TEXT("Ability costs not met."));
			ClientFailPredictedAbility(AbilityRequest.PredictionID);
			return;
		}
	}

	if (!Result.Ability->CheckChargesMet())
	{
		Result.FailReason = FString(TEXT("Charges not met."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	}

	if (!Result.Ability->CheckCustomCastConditionsMet())
	{
		Result.FailReason = FString(TEXT("Custom cast conditions not met."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	}

	if (CheckAbilityRestricted(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Cast restriction returned true."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	}

	if (IsValid(CrowdControlHandler))
	{
		for (TSubclassOf<UCrowdControl> const CcClass : Result.Ability->GetRestrictedCrowdControls())
		{
			if (CrowdControlHandler->GetActiveCcTypes().Contains(CcClass))
			{
				Result.FailReason = FString(TEXT("Cast restricted by crowd control."));
				ClientFailPredictedAbility(AbilityRequest.PredictionID);
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
		AuthStartGlobal(Result.Ability);
		ServerResult.bActivatedGlobal = true;
		ServerResult.GlobalLength = GlobalCooldownState.EndTime - GlobalCooldownState.StartTime;
	}

	int32 const PreviousCharges = Result.Ability->GetCurrentCharges();
	Result.Ability->CommitCharges(Result.PredictionID);
	int32 const NewCharges = Result.Ability->GetCurrentCharges();
	ServerResult.ChargesSpent = PreviousCharges - NewCharges;
	ServerResult.bSpentCharges = (ServerResult.ChargesSpent != 0);

	if (Costs.Num() > 0 && IsValid(ResourceHandler))
	{
		ResourceHandler->AuthCommitAbilityCosts(Result.Ability, Result.PredictionID, Costs);
		ServerResult.AbilityCosts = Costs;
	}

	FCombatParameters BroadcastParams;
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = FString(TEXT("No cast type was set."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->ServerPredictedTick(0, AbilityRequest.PredictionParams, BroadcastParams);
		OnAbilityTick.Broadcast(Result);
		BroadcastAbilityTick(Result, false, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		AuthStartCast(Result.Ability, true, Result.PredictionID);
		ServerResult.bActivatedCastBar = true;
		ServerResult.CastLength = CastingState.CastEndTime - CastingState.CastStartTime;
		ServerResult.bInterruptible = CastingState.bInterruptible;
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerPredictedTick(0, AbilityRequest.PredictionParams, BroadcastParams);
			OnAbilityTick.Broadcast(Result);
			BroadcastAbilityTick(Result, false, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = FString(TEXT("Defaulted on cast type."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID);
		return;
	}

	ClientSucceedPredictedAbility(ServerResult);
}

void UAbilityHandler::ClientSucceedPredictedAbility_Implementation(FServerAbilityResult const& ServerResult)
{
	if (!IsValid(ServerResult.AbilityClass) || ServerResult.PredictionID == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server sent invalid ability confirmation."));
		return;
	}

	UpdatePredictedCastFromServer(ServerResult);
	UpdatePredictedGlobalFromServer(ServerResult);
	TArray<FGameplayTag> MispredictedCosts;
	UCombatAbility* Ability;
	
	FClientAbilityPrediction* OriginalPrediction = UnackedAbilityPredictions.Find(ServerResult.PredictionID);
	if (OriginalPrediction != nullptr)
	{
		OriginalPrediction->PredictedCostTags.GetGameplayTagArray(MispredictedCosts);
		for (FAbilityCost const& Cost : ServerResult.AbilityCosts)
		{
			MispredictedCosts.Remove(Cost.ResourceTag);
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
	if (IsValid(ResourceHandler))
	{	
		ResourceHandler->UpdatePredictedCostsFromServer(ServerResult, MispredictedCosts);
	}
}

void UAbilityHandler::ClientFailPredictedAbility_Implementation(int32 const PredictionID)
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
	if (OriginalPrediction->bPredictedCharges)
	{
		if (IsValid(OriginalPrediction->Ability))
		{
			OriginalPrediction->Ability->RollbackFailedCharges(PredictionID);
		}
	}
	if (IsValid(ResourceHandler) && !OriginalPrediction->PredictedCostTags.IsEmpty())
	{
		ResourceHandler->RollbackFailedCosts(OriginalPrediction->PredictedCostTags, PredictionID);
	}
	UnackedAbilityPredictions.Remove(PredictionID);
}
#pragma endregion
#pragma region PredictedGlobal
void UAbilityHandler::PredictStartGlobal(int32 const PredictionID)
{
	FGlobalCooldown const PreviousState = GlobalCooldownState;
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.PredictionID = PredictionID;
	GlobalCooldownState.StartTime = GameStateRef->GetWorldTime();
	GlobalCooldownState.EndTime = 0.0f;
	OnGlobalCooldownChanged.Broadcast(PreviousState, GlobalCooldownState);
}

void UAbilityHandler::UpdatePredictedGlobalFromServer(FServerAbilityResult const& ServerResult)
{
	FGlobalCooldown const PreviousState = GlobalCooldownState;
	if (GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.PredictionID > ServerResult.PredictionID)
	{
		//Ignore if we have predicted a newer global.
		return;
	}
	GlobalCooldownState.PredictionID = ServerResult.PredictionID;
	if (!ServerResult.bActivatedGlobal || ServerResult.ClientStartTime + ServerResult.GlobalLength < GameStateRef->GetWorldTime())
	{
		EndGlobalCooldown();
		return;
	}
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.StartTime = ServerResult.ClientStartTime;
	GlobalCooldownState.EndTime = GlobalCooldownState.StartTime + ServerResult.GlobalLength;
	GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
	FTimerDelegate GlobalDelegate;
	GlobalDelegate.BindUFunction(this, FName(TEXT("EndGlobalCooldown")));
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, GlobalDelegate, GlobalCooldownState.EndTime - GameStateRef->GetWorldTime(), false);
	OnGlobalCooldownChanged.Broadcast(PreviousState, GlobalCooldownState);
}
#pragma endregion
#pragma region PredictedCast
void UAbilityHandler::PredictStartCast(UCombatAbility* Ability, int32 const PredictionID)
{
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.PredictionID = PredictionID;
	CastingState.CastStartTime = GameStateRef->GetWorldTime();
	CastingState.CastEndTime = 0.0f;
	CastingState.bInterruptible = Ability->GetInterruptible();
	CastingState.ElapsedTicks = 0;
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UAbilityHandler::UpdatePredictedCastFromServer(FServerAbilityResult const& ServerResult)
{
	FCastingState const PreviousState = CastingState;
	if (CastingState.PredictionID > ServerResult.PredictionID)
	{
		//Don't override if we are already predicting a newer cast or cancel.
		return;
	}
	CastingState.PredictionID = ServerResult.PredictionID;
	if (!ServerResult.bActivatedCastBar || ServerResult.ClientStartTime + ServerResult.CastLength < GameStateRef->GetWorldTime())
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
	CastingState.ElapsedTicks = FMath::FloorToInt((GameStateRef->GetWorldTime() - CastingState.CastStartTime) / TickInterval);

	//Clear any cast or tick handles that existed, this shouldn't happen.
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	GetWorld()->GetTimerManager().ClearTimer(TickHandle);

	//Set new handles for predicting the cast end and predicting the next tick.
	FTimerDelegate CastDelegate;
	CastDelegate.BindUFunction(this, FName(TEXT("CompleteCast")));
	GetWorld()->GetTimerManager().SetTimer(CastHandle, CastDelegate, CastingState.CastEndTime - GameStateRef->GetWorldTime(), false);
	FTimerDelegate TickDelegate;
	TickDelegate.BindUFunction(this, FName(TEXT("PredictAbilityTick")));
	//First iteration of the tick timer will get time remaining until the next tick (to account for travel time). Subsequent ticks use regular interval.
	GetWorld()->GetTimerManager().SetTimer(TickHandle, TickDelegate, TickInterval, true,
		(CastingState.CastStartTime + (TickInterval * (CastingState.ElapsedTicks + 1))) - GameStateRef->GetWorldTime());

	//Immediately perform the last missed tick if one happened during the wait time between ability prediction and confirmation.
	if (CastingState.ElapsedTicks > 0)
	{
		HandleMissedPredictedTick(CastingState.ElapsedTicks);
	}
	
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}
#pragma endregion
#pragma region PredictedTicks
//Function called when the tick timer rolls over on clients that will predict the tick and send any tick parameters to the server.
void UAbilityHandler::PredictAbilityTick()
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
void UAbilityHandler::HandleMissedPredictedTick(int32 const TickNumber)
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
void UAbilityHandler::ServerHandlePredictedTick_Implementation(FAbilityRequest const& TickRequest)
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
				BroadcastAbilityTick(TickEvent, false, BroadcastParams);
			}
			TicksAwaitingParams.RemoveSingleSwap(Tick);
			return;
		}
	}
}

//Expire any ticks awaiting parameters once the following tick happens.
void UAbilityHandler::PurgeExpiredPredictedTicks()
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
void UAbilityHandler::AuthTickPredictedCast()
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
		CastingState.CurrentCast->ServerNonPredictedTick(CastingState.ElapsedTicks, BroadcastParams);
		FCastEvent TickEvent;
		TickEvent.Ability = CastingState.CurrentCast;
		TickEvent.Tick = CastingState.ElapsedTicks;
		TickEvent.ActionTaken = ECastAction::Tick;
		TickEvent.PredictionID = CastingState.PredictionID;
		OnAbilityTick.Broadcast(TickEvent);
		BroadcastAbilityTick(TickEvent, false, BroadcastParams);
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
		BroadcastAbilityTick(TickEvent, false, BroadcastParams);
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
#pragma region AbilityQueueing
void UAbilityHandler::TryQueueAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	ClearQueue();
	if (!GlobalCooldownState.bGlobalCooldownActive && !CastingState.bIsCasting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tried to queue an ability when not casting or on global cooldown."));
		return;
	}
	if (!IsValid(AbilityClass))
	{
		return;
	}
	if (CastingState.bIsCasting && CastingState.CastEndTime == 0.0f)
	{
		//Don't queue an ability if the cast time hasn't been acked yet.
		return;
	}
	if (AbilityClass->GetDefaultObject<UCombatAbility>()->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime == 0.0f)
	{
		//Don't queue an ability if the gcd time hasn't been acked yet.
		return;
	}
	float const GlobalTimeRemaining = AbilityClass->GetDefaultObject<UCombatAbility>()->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive ? GlobalCooldownState.EndTime - GameStateRef->GetWorldTime() : 0.0f;
	float const CastTimeRemaining = CastingState.bIsCasting ? CastingState.CastEndTime - GameStateRef->GetWorldTime() : 0.0f;
	if (GlobalTimeRemaining > AbilityQueWindowSec || CastTimeRemaining > AbilityQueWindowSec)
	{
		//Don't queue if either the gcd or cast time will last longer than the queue window.
		return;
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
}

void UAbilityHandler::ClearQueue()
{
	QueueStatus = EQueueStatus::Empty;
	QueuedAbility = nullptr;
	GetWorld()->GetTimerManager().ClearTimer(QueueClearHandle);
}

void UAbilityHandler::SetQueueExpirationTimer()
{
	FTimerDelegate QueueExpirationDelegate;
	QueueExpirationDelegate.BindUFunction(this, FName(TEXT("ClearQueue")));
	GetWorld()->GetTimerManager().SetTimer(QueueClearHandle, QueueExpirationDelegate, AbilityQueWindowSec, false);
}

void UAbilityHandler::CheckForQueuedAbilityOnGlobalEnd(FGlobalCooldown const& OldGlobalCooldown,
	FGlobalCooldown const& NewGlobalCooldown)
{
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
			ClearQueue();
			if (IsValid(QueuedAbility))
			{
				PredictUseAbility(QueuedAbility, true);
			}
			return;
		default :
			return;
	}
}

void UAbilityHandler::CheckForQueuedAbilityOnCastEnd(FCastingState const& OldState, FCastingState const& NewState)
{
	switch (QueueStatus)
	{
	case EQueueStatus::Empty :
		return;
	case EQueueStatus::WaitForBoth :
		QueueStatus = EQueueStatus::WaitForGlobal;
		return;
	case EQueueStatus::WaitForCast :
		ClearQueue();
		if (IsValid(QueuedAbility))
		{
			PredictUseAbility(QueuedAbility, true);
		}
		return;
	case EQueueStatus::WaitForGlobal :
		return;
	default :
		return;
	}
}
#pragma endregion