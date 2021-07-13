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
#include "SaiyoraCombatLibrary.h"
#include "UObjectGlobals.h"

const float UAbilityHandler::MinimumGlobalCooldownLength = 0.5f;
const float UAbilityHandler::MinimumCastLength = 0.5f;
const float UAbilityHandler::MinimumCooldownLength = 0.5f;
const float UAbilityHandler::AbilityQueWindowSec = 0.2f;
const float UAbilityHandler::MaxPingCompensation = 0.2f;

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
			StatCooldownMod.BindDynamic(this, &UAbilityHandler::ModifyCooldownFromStat);
			AddCooldownModifier(StatCooldownMod);
			FAbilityModCondition StatGlobalCooldownMod;
			StatGlobalCooldownMod.BindDynamic(this, &UAbilityHandler::ModifyGlobalCooldownFromStat);
			AddGlobalCooldownModifier(StatGlobalCooldownMod);
			FAbilityModCondition StatCastLengthMod;
			StatCastLengthMod.BindDynamic(this, &UAbilityHandler::ModifyCastLengthFromStat);
			AddCastLengthModifier(StatCastLengthMod);
		}
		if (IsValid(DamageHandler))
		{
			FLifeStatusCallback DeathCallback;
			DeathCallback.BindDynamic(this, &UAbilityHandler::InterruptCastOnDeath);
			DamageHandler->SubscribeToLifeStatusChanged(DeathCallback);
		}
		if (IsValid(CrowdControlHandler))
		{
			FCrowdControlCallback CcCallback;
			CcCallback.BindDynamic(this, &UAbilityHandler::InterruptCastOnCrowdControl);
			CrowdControlHandler->SubscribeToCrowdControlChanged(CcCallback);
		}
		for (TSubclassOf<UCombatAbility> const AbilityClass : DefaultAbilities)
		{
			AddNewAbility(AbilityClass);
		}
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
		if (StatHandler->GetStatValid(CooldownLengthStatTag()))
		{
			Mod = USaiyoraCombatLibrary::MakeCombatModifier(EModifierType::Multiplicative, StatHandler->GetStatValue(CooldownLengthStatTag()));
		}
	}
	return Mod;
}

FCombatModifier UAbilityHandler::ModifyGlobalCooldownFromStat(UCombatAbility* Ability)
{
	FCombatModifier Mod;
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(GlobalCooldownLengthStatTag()))
		{
			Mod = USaiyoraCombatLibrary::MakeCombatModifier(EModifierType::Multiplicative, StatHandler->GetStatValue(GlobalCooldownLengthStatTag()));
		}
	}
	return Mod;
}

FCombatModifier UAbilityHandler::ModifyCastLengthFromStat(UCombatAbility* Ability)
{
	FCombatModifier Mod;
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(CastLengthStatTag()))
		{
			Mod = USaiyoraCombatLibrary::MakeCombatModifier(EModifierType::Multiplicative, StatHandler->GetStatValue(CastLengthStatTag()));
		}
	}
	return Mod;
}

void UAbilityHandler::InterruptCastOnDeath(ELifeStatus PreviousStatus, ELifeStatus NewStatus)
{
	if (GetCastingState().bIsCasting)
	{
		switch (NewStatus)
		{
			case ELifeStatus::Invalid :
				InterruptCurrentCast(GetOwner(), nullptr, true);
				break;
			case ELifeStatus::Alive :
				break;
			case ELifeStatus::Dead :
				if (!GetCastingState().CurrentCast->GetCastableWhileDead())
				{
					InterruptCurrentCast(GetOwner(), nullptr, true);
					return;
				}
				break;
			default :
				break;
		}
	}
}

void UAbilityHandler::InterruptCastOnCrowdControl(FCrowdControlStatus const& PreviousStatus, FCrowdControlStatus const& NewStatus)
{
	if (GetCastingState().bIsCasting && NewStatus.bActive == true)
	{
		if (GetCastingState().CurrentCast->GetRestrictedCrowdControls().Contains(NewStatus.CrowdControlClass))
		{
			InterruptCurrentCast(GetOwner(), nullptr, true);
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
	if (!IsValid(AbilityClass) || IsValid(FindActiveAbility(AbilityClass)))
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
	FTimerDelegate const RemovalDelegate = FTimerDelegate::CreateUObject(this, &UAbilityHandler::CleanupRemovedAbility, AbilityToRemove);
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

float UAbilityHandler::GetCurrentCastLength() const
{
	return CastingState.bIsCasting && CastingState.CastEndTime != 0.0 ?
		FMath::Max(0.0f, CastingState.CastEndTime - CastingState.CastStartTime) : 0.0f;
}

float UAbilityHandler::GetCastTimeRemaining() const
{
	return FMath::Max(GetWorld()->GetTimerManager().GetTimerRemaining(CastHandle), 0.0f);
}

float UAbilityHandler::GetCurrentGlobalCooldownLength() const
{
	return GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime != 0.0f ?
        FMath::Max(0.0f, GlobalCooldownState.EndTime - GlobalCooldownState.StartTime) : 0.0f;
}

float UAbilityHandler::GetGlobalCooldownTimeRemaining() const
{
	return FMath::Max(GetWorld()->GetTimerManager().GetTimerRemaining(GlobalCooldownHandle), 0.0f);
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
		if (Restriction.IsBound())
		{
			if (Restriction.Execute(Ability))
			{
				return true;
			}
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

void UAbilityHandler::SubscribeToAbilityMispredicted(FAbilityMispredictionCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnAbilityMispredicted.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityMispredicted(FAbilityMispredictionCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_AutonomousProxy || !Callback.IsBound())
	{
		return;
	}
	OnAbilityMispredicted.Remove(Callback);
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

void UAbilityHandler::BroadcastAbilityCancel_Implementation(FCancelEvent const& CancelEvent, FCombatParameters const& BroadcastParams)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		if (IsValid(CancelEvent.CancelledAbility))
		{
			CancelEvent.CancelledAbility->SimulatedCancel(BroadcastParams);
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

void UAbilityHandler::BroadcastAbilityTick_Implementation(FCastEvent const& CastEvent, FCombatParameters const& BroadcastParams)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
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
			if (Mod.IsBound())
			{
				Mods.Add(Mod.Execute(Ability));
			}
		}
		CastLength = FCombatModifier::CombineModifiers(Mods, CastLength);
	}
	CastLength = FMath::Max(MinimumCastLength, CastLength);
	return CastLength;
}

void UAbilityHandler::StartCast(UCombatAbility* Ability, bool const bPredicted, int32 const PredictionID)
{
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CastStartTime = GameStateRef->GetServerWorldTimeSeconds();
	float CastLength = CalculateCastLength(Ability);
	if (bPredicted && PredictionID != 0)
	{
		CastingState.PredictionID = PredictionID;
		float const PingCompensation = FMath::Clamp(USaiyoraCombatLibrary::GetActorPing(GetOwner()), 0.0f, MaxPingCompensation);
		CastLength = FMath::Max(MinimumCastLength, CastLength - PingCompensation);
	}
	CastingState.CastEndTime = CastingState.CastStartTime + CastLength;
	CastingState.bInterruptible = Ability->GetInterruptible();
	GetWorld()->GetTimerManager().SetTimer(CastHandle, this, &UAbilityHandler::CompleteCast, CastLength, false);
	if (bPredicted)
	{
		GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityHandler::AuthTickPredictedCast,
			(CastLength / Ability->GetNumberOfTicks()), true);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityHandler::TickCurrentAbility,
			(CastLength / Ability->GetNumberOfTicks()), true);
	}
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
	CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, BroadcastParams);
	FCastEvent TickEvent;
	TickEvent.Ability = CastingState.CurrentCast;
	TickEvent.Tick = CastingState.ElapsedTicks;
	TickEvent.ActionTaken = ECastAction::Tick;
	OnAbilityTick.Broadcast(TickEvent);
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

void UAbilityHandler::CompleteCast()
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
	FCancelEvent const Fail;
	switch (GetOwnerRole())
	{
		case ROLE_Authority :
			return CancelAbility();
		case ROLE_AutonomousProxy :
			return PredictCancelAbility();
		case ROLE_SimulatedProxy :
			return Fail;
		case ROLE_None :
			return Fail;
		default :
			return Fail;
	}
}

FCancelEvent UAbilityHandler::CancelAbility()
{
	FCancelEvent Result;
	if (GetOwner()->GetRemoteRole() == ROLE_AutonomousProxy)
	{
		return Result;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		return Result;
	}
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GameStateRef->GetServerWorldTimeSeconds();
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	FCombatParameters CancelParams;
	Result.CancelledAbility->ServerCancel(CancelParams);
	OnAbilityCancelled.Broadcast(Result);
	BroadcastAbilityCancel(Result, CancelParams);
	EndCast();
	return Result;
}

FCancelEvent UAbilityHandler::PredictCancelAbility()
{
	FCancelEvent Result;
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		return Result;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		return Result;
	}
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GameStateRef->GetServerWorldTimeSeconds();
	GenerateNewPredictionID(Result);
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.CancelledCastID = CastingState.PredictionID;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	FCancelRequest CancelRequest;
	CancelRequest.CancelTime = Result.CancelTime;
	CancelRequest.CancelID = Result.CancelID;
	CancelRequest.CancelledCastID = Result.CancelledCastID;
	Result.CancelledAbility->PredictedCancel(CancelRequest.PredictionParams);
	ServerPredictCancelAbility(CancelRequest);
	OnAbilityCancelled.Broadcast(Result);
	CastingState.PredictionID = Result.CancelID;
	EndCast();
	Result.bSuccess = true;
	return Result;
}

void UAbilityHandler::ServerPredictCancelAbility_Implementation(FCancelRequest const& CancelRequest)
{
	
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast) || CastingState.PredictionID != CancelRequest.CancelledCastID)
	{
		if (CancelRequest.CancelID > CastingState.PredictionID)
		{
			CastingState.PredictionID = CancelRequest.CancelID;
		}
		return;
	}
	if (CancelRequest.CancelID > CastingState.PredictionID)
	{
		CastingState.PredictionID = CancelRequest.CancelID;
	}
	FCancelEvent Result;
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GameStateRef->GetServerWorldTimeSeconds();
	Result.CancelID = CancelRequest.CancelID;
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.CancelledCastID = CastingState.PredictionID;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	FCombatParameters BroadcastParams;
	Result.CancelledAbility->ServerPredictedCancel(CancelRequest.PredictionParams, BroadcastParams);
	OnAbilityCancelled.Broadcast(Result);
	BroadcastAbilityCancel(Result, BroadcastParams);
	EndCast();
}

FInterruptEvent UAbilityHandler::InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions)
{
	FInterruptEvent Result;
	if (GetOwnerRole() != ROLE_Authority)
	{
		return Result;
	}
	if (!GetCastingState().bIsCasting || !IsValid(Result.InterruptedAbility))
	{
		Result.FailReason = FString(TEXT("Not currently casting."));
		return Result;
	}
	
	Result.InterruptAppliedBy = AppliedBy;
	Result.InterruptSource = InterruptSource;
	Result.InterruptAppliedTo = GetOwner();
	Result.InterruptedAbility = CastingState.CurrentCast;
	Result.InterruptedCastStart = GetCastingState().CastStartTime;
	Result.InterruptedCastEnd = GetCastingState().CastEndTime;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.CancelledCastID = CastingState.PredictionID;
	Result.InterruptTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	
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

void UAbilityHandler::AddInterruptRestriction(FInterruptRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (!Restriction.IsBound())
	{
		return;
	}
	InterruptRestrictions.AddUnique(Restriction);
}

void UAbilityHandler::RemoveInterruptRestriction(FInterruptRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (!Restriction.IsBound())
	{
		return;
	}
	InterruptRestrictions.RemoveSingleSwap(Restriction);
}

bool UAbilityHandler::CheckInterruptRestricted(FInterruptEvent const& InterruptEvent)
{
	for (FInterruptRestriction const& Restriction : InterruptRestrictions)
	{
		if (Restriction.IsBound() && Restriction.Execute(InterruptEvent))
		{
			return true;
		}
	}
	return false;
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
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		CheckForQueuedAbilityOnCastEnd();
	}
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
			if (Condition.IsBound())
			{
				ModArray.Add(Condition.Execute(Ability));
			}
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
			if (Mod.IsBound())
			{
				Mods.Add(Mod.Execute(Ability));
			}
		}
		GlobalLength = FCombatModifier::CombineModifiers(Mods, GlobalLength);
	}
	GlobalLength = FMath::Max(MinimumGlobalCooldownLength, GlobalLength);
	return GlobalLength;
}

void UAbilityHandler::StartGlobal(UCombatAbility* Ability, bool const bPredicted)
{
	FGlobalCooldown const PreviousGlobal = GlobalCooldownState;
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.StartTime = GameStateRef->GetServerWorldTimeSeconds();
	float GlobalLength = CalculateGlobalCooldownLength(Ability);
	if (bPredicted)
	{
		float const PingCompensation = FMath::Clamp(USaiyoraCombatLibrary::GetActorPing(GetOwner()), 0.0f, MaxPingCompensation);
		GlobalLength = FMath::Max(MinimumGlobalCooldownLength, GlobalLength - PingCompensation);
	}
	GlobalCooldownState.EndTime = GlobalCooldownState.StartTime + GlobalLength;
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UAbilityHandler::EndGlobalCooldown, GlobalLength, false);
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
		if (GetOwnerRole() == ROLE_AutonomousProxy)
		{
			CheckForQueuedAbilityOnGlobalEnd();
		}
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
		Result.FailReason = ECastFailReason::NetRole;
		return Result;
	default :
		Result.FailReason = ECastFailReason::NetRole;
		return Result;
	}
}

FCastEvent UAbilityHandler::AuthUseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FCastEvent Result;

	if (GetOwnerRole() != ROLE_Authority || GetOwner()->GetRemoteRole() != ROLE_SimulatedProxy)
	{
		Result.FailReason = ECastFailReason::NetRole;
		return Result;
	}
	
	Result.Ability = FindActiveAbility(AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = ECastFailReason::InvalidAbility;
		return Result;
	}

	TArray<FAbilityCost> Costs;
	if (!CheckCanCastAbility(Result.Ability, Costs, Result.FailReason))
	{
		return Result;
	}
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		StartGlobal(Result.Ability);
	}
	
	Result.Ability->CommitCharges();
	
	if (Costs.Num() > 0 && IsValid(ResourceHandler))
	{
		ResourceHandler->CommitAbilityCosts(Result.Ability, Costs);
	}

	FCombatParameters BroadcastParams;
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = ECastFailReason::InvalidCastType;
		return Result;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->ServerTick(0, BroadcastParams);
		OnAbilityTick.Broadcast(Result);
		BroadcastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		StartCast(Result.Ability);
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerTick(0, BroadcastParams);
			OnAbilityTick.Broadcast(Result);
			BroadcastAbilityTick(Result, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = ECastFailReason::InvalidCastType;
		return Result;
	}
	
	return Result;
}

bool UAbilityHandler::CheckCanCastAbility(UCombatAbility* Ability, TArray<FAbilityCost>& OutCosts, ECastFailReason& OutFailReason)
{
	if (!IsValid(Ability))
	{
		OutFailReason = ECastFailReason::InvalidAbility;
		return false;
	}
	if (!Ability->GetCastableWhileDead() && IsValid(DamageHandler) && DamageHandler->GetLifeStatus() != ELifeStatus::Alive)
	{
		OutFailReason = ECastFailReason::Dead;
		return false;
	}
	
	if (Ability->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive)
	{
		OutFailReason = ECastFailReason::OnGlobalCooldown;
		return false;
	}
	
	if (CastingState.bIsCasting)
	{
		OutFailReason = ECastFailReason::AlreadyCasting;
		return false;
	}
	
	Ability->GetAbilityCosts(OutCosts);
	if (OutCosts.Num() > 0)
	{
		if (!IsValid(ResourceHandler))
		{
			OutFailReason = ECastFailReason::NoResourceHandler;
			return false;
		}
		if (!ResourceHandler->CheckAbilityCostsMet(Ability, OutCosts))
		{
			OutFailReason = ECastFailReason::CostsNotMet;
			return false;
		}
	}

	if (!Ability->CheckChargesMet())
	{
		OutFailReason = ECastFailReason::ChargesNotMet;
		return false;
	}

	if (!Ability->CheckCustomCastConditionsMet())
	{
		OutFailReason = ECastFailReason::AbilityConditionsNotMet;
		return false;
	}

	if (CheckAbilityRestricted(Ability))
	{
		OutFailReason = ECastFailReason::CustomRestriction;
		return false;
	}

	if (IsValid(CrowdControlHandler))
	{
		for (TSubclassOf<UCrowdControl> const CcClass : Ability->GetRestrictedCrowdControls())
		{
			if (CrowdControlHandler->GetActiveCcTypes().Contains(CcClass))
			{
				OutFailReason = ECastFailReason::CrowdControl;
				return false;
			}
		}
	}

	return true;
}

#pragma endregion
#pragma region Prediction

void UAbilityHandler::ServerPredictAbility_Implementation(FAbilityRequest const& AbilityRequest)
{
	FCastEvent Result;
	Result.PredictionID = AbilityRequest.PredictionID;
	
	Result.Ability = FindActiveAbility(AbilityRequest.AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Did not find valid active ability."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	if (!Result.Ability->GetCastableWhileDead() && IsValid(DamageHandler) && DamageHandler->GetLifeStatus() != ELifeStatus::Alive)
	{
		Result.FailReason = FString(TEXT("Cannot activate while dead."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}
	
	if (Result.Ability->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive)
	{
		Result.FailReason = FString(TEXT("Already on global cooldown."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	if (GetCastingState().bIsCasting)
	{
		Result.FailReason = FString(TEXT("Already casting."));
		ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
		return;
	}

	TArray<FAbilityCost> Costs;
	Result.Ability->GetAbilityCosts(Costs);
	if (Costs.Num() > 0)
	{
		if (!IsValid(ResourceHandler))
		{
			Result.FailReason = FString(TEXT("No valid resource handler found."));
			ClientFailPredictedAbility(AbilityRequest.PredictionID, Result.FailReason);
			return;
		}
		if (!ResourceHandler->CheckAbilityCostsMet(Result.Ability, Costs))
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

	if (IsValid(CrowdControlHandler))
	{
		for (TSubclassOf<UCrowdControl> const CcClass : Result.Ability->GetRestrictedCrowdControls())
		{
			if (CrowdControlHandler->GetActiveCcTypes().Contains(CcClass))
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
		StartGlobal(Result.Ability, true);
		ServerResult.bActivatedGlobal = true;
		ServerResult.GlobalLength = CalculateGlobalCooldownLength(Result.Ability);
	}

	int32 const PreviousCharges = Result.Ability->GetCurrentCharges();
	Result.Ability->CommitCharges(Result.PredictionID);
	int32 const NewCharges = Result.Ability->GetCurrentCharges();
	ServerResult.ChargesSpent = PreviousCharges - NewCharges;
	ServerResult.bSpentCharges = (ServerResult.ChargesSpent != 0);

	if (Costs.Num() > 0 && IsValid(ResourceHandler))
	{
		ResourceHandler->CommitAbilityCosts(Result.Ability, Costs, Result.PredictionID);
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
		OnAbilityTick.Broadcast(Result);
		BroadcastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		ServerResult.CastLength = CalculateCastLength(Result.Ability);
		StartCast(Result.Ability, true, Result.PredictionID);
		ServerResult.bActivatedCastBar = true;
		ServerResult.bInterruptible = CastingState.bInterruptible;
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerPredictedTick(0, AbilityRequest.PredictionParams, BroadcastParams);
			OnAbilityTick.Broadcast(Result);
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

void UAbilityHandler::ClientSucceedPredictedAbility_Implementation(FServerAbilityResult const& ServerResult)
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
	if (IsValid(ResourceHandler))
	{	
		ResourceHandler->UpdatePredictedCostsFromServer(ServerResult, MispredictedCosts);
	}
}

void UAbilityHandler::ClientFailPredictedAbility_Implementation(int32 const PredictionID, FString const& FailReason)
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
	if (IsValid(ResourceHandler) && !OriginalPrediction->PredictedCostClasses.Num() == 0)
	{
		ResourceHandler->RollbackFailedCosts(OriginalPrediction->PredictedCostClasses, PredictionID);
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
				BroadcastAbilityTick(TickEvent, BroadcastParams);
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
		CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, BroadcastParams);
		FCastEvent TickEvent;
		TickEvent.Ability = CastingState.CurrentCast;
		TickEvent.Tick = CastingState.ElapsedTicks;
		TickEvent.ActionTaken = ECastAction::Tick;
		TickEvent.PredictionID = CastingState.PredictionID;
		OnAbilityTick.Broadcast(TickEvent);
		BroadcastAbilityTick(TickEvent, BroadcastParams);
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
		BroadcastAbilityTick(TickEvent, BroadcastParams);
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