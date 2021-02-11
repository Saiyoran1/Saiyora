// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityHandler.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ResourceHandler.h"
#include "StatHandler.h"

const float UAbilityHandler::MinimumGlobalCooldownLength = 0.5f;
const float UAbilityHandler::MinimumCastLength = 0.5f;
const float UAbilityHandler::MinimumCooldownLength = 0.5f;
const FGameplayTag UAbilityHandler::CastLengthStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CastLength")), false);
const FGameplayTag UAbilityHandler::GlobalCooldownLengthStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.GlobalCooldownLength")), false);
const FGameplayTag UAbilityHandler::CooldownLengthStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat.CooldownLength")), false);

UAbilityHandler::UAbilityHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAbilityHandler::BeginPlay()
{
	Super::BeginPlay();
	
	GameStateRef = GetWorld()->GetGameState<ASaiyoraGameState>();
	if (!GameStateRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Game State Ref in Ability Component."));
	}
	ResourceHandler = GetOwner()->FindComponentByClass<UResourceHandler>();
	StatHandler = GetOwner()->FindComponentByClass<UStatHandler>();
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (TSubclassOf<UCombatAbility> const AbilityClass : DefaultAbilities)
		{
			AddNewAbility(AbilityClass);
		}
	}
}

void UAbilityHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityHandler, CastingState);
	DOREPLIFETIME_CONDITION(UAbilityHandler, GlobalCooldownState, COND_OwnerOnly);
}

bool UAbilityHandler::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	
	bWroteSomething |= Channel->ReplicateSubobjectList(ActiveAbilities, *Bunch, *RepFlags);
	bWroteSomething |= Channel->ReplicateSubobjectList(RecentlyRemovedAbilities, *Bunch, *RepFlags);
	return bWroteSomething;
}

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

void UAbilityHandler::SubscribeToAbilityStarted(FAbilityCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityStart.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityStarted(FAbilityCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityStart.Remove(Callback);
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

void UAbilityHandler::SubscribeToAbilityCompleted(FAbilityCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnAbilityComplete.AddUnique(Callback);
}

void UAbilityHandler::UnsubscribeFromAbilityCompleted(FAbilityCallback const& Callback)
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
		float AddMod = 0.0f;
		float MultMod = 1.0f;
		FCombatModifier TempMod;
		for (FAbilityModCondition const& Condition : CooldownLengthMods)
		{
			TempMod = Condition.Execute(Ability);
			switch (TempMod.ModifierType)
			{
			case EModifierType::Invalid :
				break;
			case EModifierType::Additive :
				AddMod += TempMod.ModifierValue;
				break;
			case EModifierType::Multiplicative :
				MultMod *= FMath::Max(0.0f, TempMod.ModifierValue);
				break;
			default :
				break;
			}
		}
		if (IsValid(GetStatHandlerRef()))
		{
			float const ModFromStat = GetStatHandlerRef()->GetStatValue(CooldownLengthStatTag);
			if (ModFromStat > 0.0f)
			{
				MultMod *= ModFromStat;
			}
		}
		CooldownLength = FMath::Max(0.0f, CooldownLength + AddMod) * MultMod;
	}
	CooldownLength = FMath::Max(CooldownLength, MinimumCooldownLength);
	return CooldownLength;
}

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

void UAbilityHandler::OnRep_GlobalCooldownState(FGlobalCooldown const& PreviousGlobal)
{
	OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
}

void UAbilityHandler::StartGlobalCooldown(UCombatAbility* Ability, int32 const CastID)
{
	FGlobalCooldown const PreviousGlobal = GetGlobalCooldownState();
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.StartTime = GameStateRef->GetWorldTime();
	GlobalCooldownState.EndTime = GetGlobalCooldownState().StartTime + CalculateGlobalCooldownLength(Ability);
	GlobalCooldownState.CastID = CastID;
	FTimerDelegate GCDDelegate;
	GCDDelegate.BindUFunction(this, FName(TEXT("EndGlobalCooldown")));
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, GCDDelegate, GetGlobalCooldownState().EndTime - GameStateRef->GetWorldTime(), false);
	OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GetGlobalCooldownState());
}

void UAbilityHandler::EndGlobalCooldown()
{
	FGlobalCooldown const PreviousGlobal;
	GlobalCooldownState.bGlobalCooldownActive = false;
	GlobalCooldownState.StartTime = 0.0f;
	GlobalCooldownState.EndTime = 0.0f;
	if (GetWorld()->GetTimerManager().IsTimerActive(GlobalCooldownHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
	}
	OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
}

float UAbilityHandler::CalculateGlobalCooldownLength(UCombatAbility* Ability)
{
	float GlobalLength = Ability->GetDefaultGlobalCooldownLength();
	if (!Ability->HasStaticGlobalCooldown())
	{
		float AddMod = 0.0f;
		float MultMod = 1.0f;
		FCombatModifier TempMod;
		for (FAbilityModCondition const& Condition : GlobalCooldownMods)
		{
			TempMod = Condition.Execute(Ability);
			switch (TempMod.ModifierType)
			{
			case EModifierType::Invalid :
				break;
			case EModifierType::Additive :
				AddMod += TempMod.ModifierValue;
				break;
			case EModifierType::Multiplicative :
				MultMod *= FMath::Max(0.0f, TempMod.ModifierValue);
				break;
			default :
				break;
			}
		}
		if (IsValid(GetStatHandlerRef()))
		{
			float const ModFromStat = GetStatHandlerRef()->GetStatValue(GlobalCooldownLengthStatTag);
			if (ModFromStat > 0.0f)
			{
				MultMod *= ModFromStat;
			}
		}
		GlobalLength = FMath::Max(0.0f, GlobalLength + AddMod) * MultMod;
	}
	GlobalLength = FMath::Max(MinimumGlobalCooldownLength, GlobalLength);
	return GlobalLength;
}

void UAbilityHandler::OnRep_CastingState(FCastingState const& PreviousState)
{
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UAbilityHandler::StartCast(UCombatAbility* Ability, int32 const CastID)
{
	FCastingState const PreviousState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CurrentCastID = CastID;
	CastingState.CastStartTime = GameStateRef->GetWorldTime();
	CastingState.CastEndTime = GetCastingState().CastStartTime + CalculateCastLength(Ability);
	CastingState.bInterruptible = Ability->GetInterruptible();
	FTimerDelegate CastDelegate;
	CastDelegate.BindUFunction(this, FName(TEXT("CompleteCast")));
	GetWorld()->GetTimerManager().SetTimer(CastHandle, CastDelegate, GetCastingState().CastEndTime - GameStateRef->GetWorldTime(), false);
	FTimerDelegate TickDelegate;
	TickDelegate.BindUFunction(this, FName(TEXT("TickCurrentCast")));
	GetWorld()->GetTimerManager().SetTimer(TickHandle, TickDelegate, (GetCastingState().CastEndTime - GetCastingState().CastStartTime) / Ability->GetNumberOfTicks(), true);
	OnCastStateChanged.Broadcast(PreviousState, GetCastingState());
}

void UAbilityHandler::CompleteCast()
{
	FCastEvent CompletionEvent;
	CompletionEvent.Ability = GetCastingState().CurrentCast;
	CompletionEvent.CastID = GetCastingState().CurrentCastID;
	CompletionEvent.ActionTaken = ECastAction::Complete;
	CompletionEvent.Tick = GetCastingState().ElapsedTicks;
	if (GetCastingState().CurrentCast)
	{
		GetCastingState().CurrentCast->CompleteCast();
	}
	BroadcastAbilityComplete(CompletionEvent);
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	if (!GetWorld()->GetTimerManager().IsTimerActive(TickHandle))
	{
		EndCast();
	}
}

void UAbilityHandler::TickCurrentCast()
{
	if (!GetCastingState().bIsCasting || !IsValid(GetCastingState().CurrentCast))
	{
		return;
	}
	CastingState.ElapsedTicks++;
	GetCastingState().CurrentCast->NonInitialTick(GetCastingState().ElapsedTicks);
	FCastEvent TickEvent;
	TickEvent.Ability = GetCastingState().CurrentCast;
	TickEvent.Tick = GetCastingState().ElapsedTicks;
	TickEvent.ActionTaken = ECastAction::Tick;
	TickEvent.CastID = GetCastingState().CurrentCastID;
	BroadcastAbilityTick(TickEvent);
	if (GetCastingState().ElapsedTicks >= GetCastingState().CurrentCast->GetNumberOfTicks())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		if (!GetWorld()->GetTimerManager().IsTimerActive(CastHandle))
		{
			EndCast();
		}
	}
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

float UAbilityHandler::CalculateCastLength(UCombatAbility* Ability)
{
	if (Ability->GetCastType() != EAbilityCastType::Channel)
	{
		return 0.0f;
	}
	float CastLength = Ability->GetDefaultCastLength();
	if (!Ability->HasStaticCastLength())
	{
		float AddMod = 0.0f;
		float MultMod = 1.0f;
		FCombatModifier TempMod;
		for (FAbilityModCondition const& Condition : CastLengthMods)
		{
			TempMod = Condition.Execute(Ability);
			switch (TempMod.ModifierType)
			{
			case EModifierType::Invalid :
				break;
			case EModifierType::Additive :
				AddMod += TempMod.ModifierValue;
				break;
			case EModifierType::Multiplicative :
				MultMod *= FMath::Max(0.0f, TempMod.ModifierValue);
				break;
			default :
				break;
			}
		}
		if (IsValid(GetStatHandlerRef()))
		{
			float const ModFromStat = GetStatHandlerRef()->GetStatValue(CastLengthStatTag);
			if (ModFromStat > 0.0f)
			{
				MultMod *= ModFromStat;
			}
		}
		CastLength = FMath::Max(0.0f, CastLength + AddMod) * MultMod;
	}
	CastLength = FMath::Max(MinimumCastLength, CastLength);
	return CastLength;
}

void UAbilityHandler::GenerateNewCastID(FCastEvent& CastEvent)
{
	CastEvent.CastID = 0;
}

void UAbilityHandler::BroadcastAbilityInterrupt_Implementation(FInterruptEvent const& InterruptEvent)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(InterruptEvent.InterruptedAbility))
		{
			InterruptEvent.InterruptedAbility->InterruptCast();
		}
	}
	OnAbilityInterrupted.Broadcast(InterruptEvent);
}

void UAbilityHandler::BroadcastAbilityCancel_Implementation(FCancelEvent const& CancelEvent)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(CancelEvent.CancelledAbility))
		{
			CancelEvent.CancelledAbility->CancelCast();
		}
	}
	OnAbilityCancelled.Broadcast(CancelEvent);
}

void UAbilityHandler::BroadcastAbilityComplete_Implementation(FCastEvent const& CastEvent)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(CastEvent.Ability))
		{
			CastEvent.Ability->CompleteCast();
		}
	}
	OnAbilityComplete.Broadcast(CastEvent);
}

void UAbilityHandler::BroadcastAbilityTick_Implementation(FCastEvent const& CastEvent)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(CastEvent.Ability))
		{
			CastEvent.Ability->NonInitialTick(CastEvent.Tick);
		}
	}
	OnAbilityTick.Broadcast(CastEvent);
}

void UAbilityHandler::BroadcastAbilityStart_Implementation(FCastEvent const& CastEvent)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		if (IsValid(CastEvent.Ability))
		{
			if (CastEvent.Ability->GetCastType() == EAbilityCastType::Instant)
			{
				CastEvent.Ability->InitialTick();
			}
			else if (CastEvent.Ability->GetCastType() == EAbilityCastType::Channel && CastEvent.Ability->GetHasInitialTick())
			{
				CastEvent.Ability->InitialTick();
			}
		}
	}
	OnAbilityStart.Broadcast(CastEvent);
}

FCastEvent UAbilityHandler::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FCastEvent Result;
	if (GetOwnerRole() != ROLE_Authority)
	{
		Result.FailReason = FString(TEXT("Tried to use ability without authority."));
		return Result;
	}
	
	Result.Ability = FindActiveAbility(AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReason = FString(TEXT("Did not find valid active ability."));
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
	
	//TODO: CC and Lockout Checks.

	//This just sets the Cast ID to 0. Overriding later will generate an actual new cast ID for clients.
	GenerateNewCastID(Result);
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		StartGlobalCooldown(Result.Ability, Result.CastID);
	}
	
	Result.Ability->CommitCharges(Result.CastID);
	
	if (Costs.Num() > 0 && IsValid(ResourceHandler))
	{
		ResourceHandler->CommitAbilityCosts(Result.Ability, Result.CastID, Costs);
	}
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = FString(TEXT("No cast type was set."));
		return Result;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->InitialTick();
		BroadcastAbilityStart(Result);
		BroadcastAbilityComplete(Result);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->InitialTick();
		}
		StartCast(Result.Ability, Result.CastID);
		BroadcastAbilityStart(Result);
		break;
	default :
		Result.FailReason = FString(TEXT("Defaulted on cast type."));
		return Result;
	}
	
	return Result;
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
		Result.CancelledCastID = GetCastingState().CurrentCastID;
		Result.ElapsedTicks = GetCastingState().ElapsedTicks;
		EndCast();
		Result.CancelledAbility->CancelCast();
		BroadcastAbilityCancel(Result);
	}
	return Result;
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
	Result.CancelledCastID = GetCastingState().CurrentCastID;
	Result.InterruptTime = GetGameStateRef()->GetWorldTime();
	Result.bSuccess = true;
	
	Result.InterruptedAbility->InterruptCast();
	BroadcastAbilityInterrupt(Result);
	EndCast();
	
	return Result;
}
