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

#pragma region SetupFunctions
UAbilityHandler::UAbilityHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAbilityHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityHandler, CastingState);
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

	GameStateRef = GetWorld()->GetGameState<AGameState>();
	if (!IsValid(GameStateRef))
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
	if (IsValid(StatHandler))
	{
		FAbilityModCondition StatCooldownMod;
		StatCooldownMod.BindDynamic(this, &UAbilityHandler::ModifyCooldownFromStat);
		AddConditionalCooldownModifier(StatCooldownMod);
		FAbilityModCondition StatGlobalCooldownMod;
		StatGlobalCooldownMod.BindDynamic(this, &UAbilityHandler::ModifyGlobalCooldownFromStat);
		AddConditionalGlobalCooldownModifier(StatGlobalCooldownMod);
		FAbilityModCondition StatCastLengthMod;
		StatCastLengthMod.BindDynamic(this, &UAbilityHandler::ModifyCastLengthFromStat);
		AddConditionalCastLengthModifier(StatCastLengthMod);
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(CrowdControlHandler))
		{
			FCrowdControlCallback CcCallback;
			CcCallback.BindDynamic(this, &UAbilityHandler::InterruptCastOnCrowdControl);
			CrowdControlHandler->SubscribeToCrowdControlChanged(CcCallback);
		}
		if (IsValid(DamageHandler))
		{
			FLifeStatusCallback DeathCallback;
			DeathCallback.BindDynamic(this, &UAbilityHandler::InterruptCastOnDeath);
			DamageHandler->SubscribeToLifeStatusChanged(DeathCallback);
		}
		SetupInitialAbilities();
	}	
}
#pragma endregion
#pragma region IntercomponentDelegates
//INTERCOMPONENT DELEGATES

FCombatModifier UAbilityHandler::ModifyCooldownFromStat(TSubclassOf<UCombatAbility> AbilityClass)
{
	FCombatModifier Mod;
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(CooldownLengthStatTag()))
		{
			Mod = FCombatModifier(StatHandler->GetStatValue(CooldownLengthStatTag()), EModifierType::Multiplicative);
		}
	}
	return Mod;
}

FCombatModifier UAbilityHandler::ModifyGlobalCooldownFromStat(TSubclassOf<UCombatAbility> AbilityClass)
{
	FCombatModifier Mod;
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(GlobalCooldownLengthStatTag()))
		{
			Mod = FCombatModifier(StatHandler->GetStatValue(GlobalCooldownLengthStatTag()), EModifierType::Multiplicative);
		}
	}
	return Mod;
}

FCombatModifier UAbilityHandler::ModifyCastLengthFromStat(TSubclassOf<UCombatAbility> AbilityClass)
{
	FCombatModifier Mod;
	if (IsValid(StatHandler))
	{
		if (StatHandler->GetStatValid(CastLengthStatTag()))
		{
			Mod = FCombatModifier(StatHandler->GetStatValue(CastLengthStatTag()), EModifierType::Multiplicative);
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

void UAbilityHandler::SetupInitialAbilities()
{
	for (TSubclassOf<UCombatAbility> const AbilityClass : DefaultAbilities)
	{
		AddNewAbility(AbilityClass);
	}
}

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
		if (!AbilityValues.Contains(AbilityClass))
		{
			FAbilityValues& Values = AbilityValues.Add(AbilityClass);
			Values.Initialize(AbilityClass, this);
		}
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
		if (!AbilityValues.Contains(NewAbility->GetClass()))
		{
			FAbilityValues& Values = AbilityValues.Add(NewAbility->GetClass());
			Values.Initialize(NewAbility->GetClass(), this);
		}
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

void UAbilityHandler::MulticastAbilityComplete_Implementation(FCastEvent const& CastEvent)
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

void UAbilityHandler::MulticastAbilityCancel_Implementation(FCancelEvent const& CancelEvent, FCombatParameters const& BroadcastParams)
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

void UAbilityHandler::MulticastAbilityInterrupt_Implementation(FInterruptEvent const& InterruptEvent)
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

void UAbilityHandler::MulticastAbilityTick_Implementation(FCastEvent const& CastEvent, FCombatParameters const& BroadcastParams)
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
#pragma region AbilityUsage
//ABILITY USAGE

FCastEvent UAbilityHandler::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FCastEvent Result;

	if (GetOwnerRole() != ROLE_Authority)
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

	TMap<TSubclassOf<UResource>, float> Costs;
	if (!CheckCanUseAbility(Result.Ability, Costs, Result.FailReason))
	{
		return Result;
	}
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		StartGlobal(AbilityClass);
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
		MulticastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		StartCast(Result.Ability);
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerTick(0, BroadcastParams);
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

bool UAbilityHandler::CheckCanUseAbility(UCombatAbility* Ability, TMap<TSubclassOf<UResource>, float>& OutCosts, ECastFailReason& OutFailReason)
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

	FAbilityValues* Values = AbilityValues.Find(Ability->GetClass());
	if (!Values)
	{
		OutFailReason = ECastFailReason::InvalidAbility;
		return false;
	}
	
	Values->GetCosts(OutCosts);
	if (OutCosts.Num() > 0)
	{
		if (!IsValid(ResourceHandler))
		{
			OutFailReason = ECastFailReason::NoResourceHandler;
			return false;
		}
		if (!ResourceHandler->CheckAbilityCostsMet(OutCosts))
		{
			OutFailReason = ECastFailReason::CostsNotMet;
			return false;
		}
	}

	if (Ability->GetCurrentCharges() < GetChargeCost(Ability->GetClass()))
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
#pragma region Casting
//CASTING

float UAbilityHandler::GetCastLength(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass))
	{
		return 0.0f;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (!Values)
	{
		UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
		if (!IsValid(DefaultAbility) || DefaultAbility->GetCastType() != EAbilityCastType::Channel)
		{
			return 0.0f;
		}
		return DefaultAbility->GetDefaultCastLength();
	}
	return Values->GetCastLength();
}

int32 UAbilityHandler::AddConditionalCastLengthModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return -1;
	}
	int32 const ModID = FModifierCollection::GetID();
	ConditionalCastLengthModifiers.Add(ModID, Modifier);
	return ModID;
}

void UAbilityHandler::RemoveConditionalCastLengthModifier(int32 const ModifierID)
{
	if (ModifierID == -1)
	{
		return;
	}
	ConditionalCastLengthModifiers.Remove(ModifierID);
}

int32 UAbilityHandler::AddClassCastLengthModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	FCombatModifier const& Modifier)
{
	if (!IsValid(AbilityClass) || Modifier.GetModType() == EModifierType::Invalid)
	{
		return -1;
	}
	FAbilityValues& Values = AbilityValues.FindOrAdd(AbilityClass);
	return Values.AddCastLengthModifier(Modifier);
}

void UAbilityHandler::RemoveClassCastLengthModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	int32 const ModifierID)
{
	if (!IsValid(AbilityClass) || ModifierID == -1)
	{
		return;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (Values)
	{
		Values->RemoveCastLengthModifier(ModifierID);
	}
}

void UAbilityHandler::StartCast(UCombatAbility* Ability)
{
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CastStartTime = GameStateRef->GetServerWorldTimeSeconds();
	float const CastLength = GetCastLength(Ability->GetClass());
	CastingState.CastEndTime = CastingState.CastStartTime + CastLength;
	CastingState.bInterruptible = Ability->GetInterruptible();
	GetWorld()->GetTimerManager().SetTimer(CastHandle, this, &UAbilityHandler::CompleteCast, CastLength, false);
	GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityHandler::TickCurrentAbility,
		(CastLength / Ability->GetNumberOfTicks()), true);
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

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
	MulticastAbilityTick(TickEvent, BroadcastParams);
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
	CompletionEvent.ActionTaken = ECastAction::Complete;
	CompletionEvent.Tick = CastingState.ElapsedTicks;
	if (IsValid(CompletionEvent.Ability))
	{
		CastingState.CurrentCast->CompleteCast();
		OnAbilityComplete.Broadcast(CompletionEvent.Ability);
		MulticastAbilityComplete(CompletionEvent);
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
	if (GetOwnerRole() != ROLE_Authority)
	{
		Result.FailReason = ECancelFailReason::NetRole;
		return Result;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		Result.FailReason = ECancelFailReason::NotCasting;
		return Result;
	}
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GameStateRef->GetServerWorldTimeSeconds();
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.bSuccess = true;
	FCombatParameters CancelParams;
	Result.CancelledAbility->ServerCancel(CancelParams);
	OnAbilityCancelled.Broadcast(Result);
	MulticastAbilityCancel(Result, CancelParams);
	EndCast();
	return Result;
}

FInterruptEvent UAbilityHandler::InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions)
{
	FInterruptEvent Result;
	if (GetOwnerRole() != ROLE_Authority)
	{
		Result.FailReason = EInterruptFailReason::NetRole;
		return Result;
	}
	if (!GetCastingState().bIsCasting || !IsValid(Result.InterruptedAbility))
	{
		Result.FailReason = EInterruptFailReason::NotCasting;
		return Result;
	}
	
	Result.InterruptAppliedBy = AppliedBy;
	Result.InterruptSource = InterruptSource;
	Result.InterruptAppliedTo = GetOwner();
	Result.InterruptedAbility = CastingState.CurrentCast;
	Result.InterruptedCastStart = GetCastingState().CastStartTime;
	Result.InterruptedCastEnd = GetCastingState().CastEndTime;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.InterruptTime = GameStateRef->GetServerWorldTimeSeconds();
	
	if (!bIgnoreRestrictions)
	{
		if (!CastingState.CurrentCast->GetInterruptible() || CheckInterruptRestricted(Result))
		{
			Result.FailReason = EInterruptFailReason::Restricted;
			return Result;
		}
	}
	
	Result.bSuccess = true;
	Result.InterruptedAbility->InterruptCast(Result);
	OnAbilityInterrupted.Broadcast(Result);
	MulticastAbilityInterrupt(Result);
	EndCast();
	
	return Result;
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

int32 UAbilityHandler::AddConditionalCooldownModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return -1;
	}
	int32 const ModID = FModifierCollection::GetID();
	ConditionalCooldownModifiers.Add(ModID, Modifier);
	return ModID;
}

void UAbilityHandler::RemoveConditionalCooldownModifier(int32 const ModifierID)
{
	if (ModifierID == -1)
	{
		return;
	}
	ConditionalCooldownModifiers.Remove(ModifierID);
}

int32 UAbilityHandler::AddClassCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	FCombatModifier const& Modifier)
{
	if (!IsValid(AbilityClass) || Modifier.GetModType() == EModifierType::Invalid)
	{
		return -1;
	}
	FAbilityValues& Values = AbilityValues.FindOrAdd(AbilityClass);
	return Values.AddCooldownModifier(Modifier);
}

void UAbilityHandler::RemoveClassCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	int32 const ModifierID)
{
	if (!IsValid(AbilityClass) || ModifierID == -1)
	{
		return;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (Values)
	{
		Values->RemoveCooldownModifier(ModifierID);
	}
}

int32 UAbilityHandler::GetMaxCharges(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass))
	{
		return 0;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (!Values)
	{
		UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
		if (!IsValid(DefaultAbility))
		{
			return 0;
		}
		return DefaultAbility->GetDefaultMaxCharges();
	}
	return Values->GetMaxCharges();
}

float UAbilityHandler::GetCooldownLength(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (!Values)
	{
		UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
		if (!IsValid(DefaultAbility))
		{
			return 0.0f;
		}
		return DefaultAbility->GetDefaultCooldown();
	}
	return Values->GetCooldownLength();
}

int32 UAbilityHandler::AddClassMaxChargeModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	FCombatModifier const& Modifier)
{
	if (!IsValid(AbilityClass) || Modifier.GetModType() == EModifierType::Invalid)
	{
		return -1;
	}
	FAbilityValues& Values = AbilityValues.FindOrAdd(AbilityClass);
	int32 const ModID = Values.AddMaxChargeModifier(Modifier);
	if (ModID != -1)
	{
		UCombatAbility* AffectedAbility = FindActiveAbility(AbilityClass);
		if (IsValid(AffectedAbility))
		{
			AffectedAbility->NotifyOfMaxChargesChanged(Values.GetMaxCharges());
		}
	}
	return ModID;
}

void UAbilityHandler::RemoveClassMaxChargeModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	int32 const ModifierID)
{
	if (!IsValid(AbilityClass) || ModifierID == -1)
	{
		return;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (Values)
	{
		Values->RemoveMaxChargeModifier(ModifierID);
	}
}

int32 UAbilityHandler::GetChargeCost(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass))
	{
		return 0;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (!Values)
	{
		UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
		if (!IsValid(DefaultAbility))
		{
			return 0;
		}
		return DefaultAbility->GetDefaultChargeCost();
	}
	return Values->GetChargeCost();
}

int32 UAbilityHandler::AddClassChargeCostModifier(TSubclassOf<UCombatAbility> const AbilityClass, FCombatModifier const& Modifier)
{
	if (!IsValid(AbilityClass) || Modifier.GetModType() == EModifierType::Invalid)
	{
		return -1;
	}
	FAbilityValues& Values = AbilityValues.FindOrAdd(AbilityClass);
	return Values.AddChargeCostModifier(Modifier);
}

void UAbilityHandler::RemoveClassChargeCostModifier(TSubclassOf<UCombatAbility> const AbilityClass, int32 const ModifierID)
{
	if (!IsValid(AbilityClass) || ModifierID == -1)
	{
		return;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (Values)
	{
		Values->RemoveChargesPerCastModifier(ModifierID);
	}
}

int32 UAbilityHandler::GetChargesPerCooldown(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass))
	{
		return 0;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (!Values)
	{
		UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
		if (!IsValid(DefaultAbility))
		{
			return 0;
		}
		return DefaultAbility->GetDefaultChargesPerCooldown();
	}
	return Values->GetChargesPerCooldown();
}


int32 UAbilityHandler::AddClassChargesPerCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	FCombatModifier const& Modifier)
{
	if (!IsValid(AbilityClass) || Modifier.GetModType() == EModifierType::Invalid)
	{
		return -1;
	}
	FAbilityValues& Values = AbilityValues.FindOrAdd(AbilityClass);
	return Values.AddChargesPerCooldownModifier(Modifier);
}

void UAbilityHandler::RemoveClassChargesPerCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	int32 const ModifierID)
{
	if (!IsValid(AbilityClass) || ModifierID == -1)
	{
		return;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (Values)
	{
		Values->RemoveChargesPerCooldownModifier(ModifierID);
	}
}
#pragma endregion
#pragma region GlobalCooldown
//GLOBAL COOLDOWN

int32 UAbilityHandler::AddConditionalGlobalCooldownModifier(FAbilityModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return -1;
	}
	int32 const ModID = FModifierCollection::GetID();
	ConditionalGlobalCooldownModifiers.Add(ModID, Modifier);
	return ModID;
}

void UAbilityHandler::RemoveConditionalGlobalCooldownModifier(int32 const ModifierID)
{
	if (ModifierID == -1)
	{
		return;
	}
	ConditionalGlobalCooldownModifiers.Remove(ModifierID);
}

int32 UAbilityHandler::AddClassGlobalCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	FCombatModifier const& Modifier)
{
	if (!IsValid(AbilityClass) || Modifier.GetModType() == EModifierType::Invalid)
	{
		return -1;	
	}
	FAbilityValues& Values = AbilityValues.FindOrAdd(AbilityClass);
	return Values.AddGlobalCooldownModifier(Modifier);
}

void UAbilityHandler::RemoveClassGlobalCooldownModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	int32 const ModifierID)
{
	if (!IsValid(AbilityClass) || ModifierID == -1)
	{
		return;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (Values)
	{
		Values->RemoveGlobalCooldownModifier(ModifierID);
	}
}

float UAbilityHandler::GetGlobalCooldownLength(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (!Values)
	{
		UE_LOG(LogTemp, Warning, TEXT("Did not find values struct for AbilityClass."));
		UCombatAbility const* DefaultAbility = AbilityClass->GetDefaultObject<UCombatAbility>();
		if (IsValid(DefaultAbility))
		{
			return DefaultAbility->GetDefaultGlobalCooldownLength();
		}
		return 0.0f;
	}
	return Values->GetGlobalCooldownLength();
}

void UAbilityHandler::StartGlobal(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FGlobalCooldown const PreviousGlobal = GlobalCooldownState;
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.StartTime = GameStateRef->GetServerWorldTimeSeconds();
	float const GlobalLength = GetGlobalCooldownLength(AbilityClass);
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
	}
}
#pragma endregion
#pragma region AbilityCost

int32 UAbilityHandler::AddClassAbilityCostModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier)
{
	if (!IsValid(AbilityClass) || !IsValid(ResourceClass) || Modifier.GetModType() == EModifierType::Invalid)
	{
		return -1;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (!Values)
	{
		return -1;
	}
	return Values->AddCostModifier(ResourceClass, Modifier);
}

void UAbilityHandler::RemoveClassAbilityCostModifier(TSubclassOf<UCombatAbility> const AbilityClass,
	TSubclassOf<UResource> const ResourceClass, int32 const ModifierID)
{
	if (!IsValid(AbilityClass) || !IsValid(ResourceClass) || ModifierID == -1)
	{
		return;
	}
	FAbilityValues* Values = AbilityValues.Find(AbilityClass);
	if (Values)
	{
		Values->RemoveCostModifier(ResourceClass, ModifierID);
	}
}

int32 UAbilityHandler::AddConditionalCostModifier(FAbilityResourceModCondition const& Modifier)
{
	if (!Modifier.IsBound())
	{
		return -1;
	}
	int32 const ModID = FModifierCollection::GetID();
	ConditionalCostModifiers.Add(ModID, Modifier);
	return ModID;
}

void UAbilityHandler::RemoveConditionalCostModifier(int32 const ModifierID)
{
	if (ModifierID == -1)
	{
		return;
	}
	ConditionalCostModifiers.Remove(ModifierID);
}

#pragma endregion
