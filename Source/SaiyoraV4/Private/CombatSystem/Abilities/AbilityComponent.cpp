// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityComponent.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ResourceHandler.h"

const float UAbilityComponent::MinimumGlobalCooldownLength = 0.5f;
const float UAbilityComponent::MinimumCastLength = 0.5f;

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAbilityComponent::BeginPlay()
{
	GameStateRef = GetWorld()->GetGameState<ASaiyoraGameState>();
	if (!GameStateRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Game State Ref in Ability Component."));
	}
	ResourceHandler = GetOwner()->FindComponentByClass<UResourceHandler>();
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (TSubclassOf<UCombatAbility> const AbilityClass : DefaultAbilities)
		{
			AddNewAbility(AbilityClass);
		}
	}
}

void UAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityComponent, CastingState);
	DOREPLIFETIME_CONDITION(UAbilityComponent, GlobalCooldownState, COND_OwnerOnly);
}

bool UAbilityComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	
	bWroteSomething |= Channel->ReplicateSubobjectList(ActiveAbilities, *Bunch, *RepFlags);
	return bWroteSomething;
}

UCombatAbility* UAbilityComponent::AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass)
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

void UAbilityComponent::NotifyOfReplicatedAddedAbility(UCombatAbility* NewAbility)
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

UCombatAbility* UAbilityComponent::FindActiveAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsValid(AbilityClass))
	{
		return nullptr;
	}
	for (UCombatAbility* Ability : ActiveAbilities)
	{
		if (IsValid(Ability) && Ability->IsA(AbilityClass) && Ability->GetInitialized())
		{
			return Ability;
		}
	}
	return nullptr;
}

void UAbilityComponent::OnRep_GlobalCooldownState(FGlobalCooldown const& PreviousGlobal)
{
	OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
}

void UAbilityComponent::StartGlobalCooldown(UCombatAbility* Ability)
{
	FGlobalCooldown const PreviousGlobal = GetGlobalCooldownState();
	GlobalCooldownState.bGlobalCooldownActive = true;
	GlobalCooldownState.StartTime = GameStateRef->GetWorldTime();
	//TODO: GlobalCooldownLength stat factoring.
	GlobalCooldownState.EndTime = GetGlobalCooldownState().StartTime + FMath::Max(Ability->GetGlobalCooldownLength(), MinimumGlobalCooldownLength);
	GlobalCooldownState.CastID = 0;
	FTimerDelegate GCDDelegate;
	GCDDelegate.BindUFunction(this, FName(TEXT("EndGlobalCooldown")));
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, GCDDelegate, GetGlobalCooldownState().EndTime - GameStateRef->GetWorldTime(), false);
	OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GetGlobalCooldownState());
}

void UAbilityComponent::EndGlobalCooldown()
{
	FGlobalCooldown const PreviousGlobal;
	GlobalCooldownState.bGlobalCooldownActive = false;
	GlobalCooldownState.StartTime = 0.0f;
	GlobalCooldownState.EndTime = 0.0f;
	GlobalCooldownState.CastID = 0;
	if (GetWorld()->GetTimerManager().IsTimerActive(GlobalCooldownHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
	}
	OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
}

void UAbilityComponent::OnRep_CastingState(FCastingState const& PreviousState)
{
	OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UAbilityComponent::StartCast(UCombatAbility* Ability)
{
	FCastingState const PreviousState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	CastingState.CurrentCastID = 0;
	CastingState.CastStartTime = GameStateRef->GetWorldTime();
	CastingState.CastEndTime = GetCastingState().CastStartTime + FMath::Max(Ability->GetCastTime(), MinimumCastLength);
	CastingState.bInterruptible = Ability->GetInterruptible();
	FTimerDelegate CastDelegate;
	CastDelegate.BindUFunction(this, FName(TEXT("CompleteCast")));
	GetWorld()->GetTimerManager().SetTimer(CastHandle, CastDelegate, GetCastingState().CastEndTime - GameStateRef->GetWorldTime(), false);
	FTimerDelegate TickDelegate;
	TickDelegate.BindUFunction(this, FName(TEXT("TickCurrentCast")));
	GetWorld()->GetTimerManager().SetTimer(TickHandle, TickDelegate, (GetCastingState().CastEndTime - GetCastingState().CastStartTime) / Ability->GetNumberOfTicks(), true);
	OnCastStateChanged.Broadcast(PreviousState, GetCastingState());
}

void UAbilityComponent::CompleteCast()
{
	FCastEvent CompletionEvent;
	CompletionEvent.Ability = GetCastingState().CurrentCast;
	CompletionEvent.CastID = GetCastingState().CurrentCastID;
	CompletionEvent.ActionTaken = ECastAction::Complete;
	CompletionEvent.Tick = GetCastingState().ElapsedTicks;
	OnAbilityComplete.Broadcast(CompletionEvent);
	GetWorld()->GetTimerManager().ClearTimer(CastHandle);
	if (!GetWorld()->GetTimerManager().IsTimerActive(TickHandle))
	{
		EndCast();
	}
}

void UAbilityComponent::TickCurrentCast()
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
	OnAbilityTick.Broadcast(TickEvent);
	if (GetCastingState().ElapsedTicks >= GetCastingState().CurrentCast->GetNumberOfTicks())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		if (!GetWorld()->GetTimerManager().IsTimerActive(CastHandle))
		{
			EndCast();
		}
	}
}

void UAbilityComponent::EndCast()
{
	FCastingState const PreviousState = GetCastingState();
	CastingState.bIsCasting = false;
	CastingState.CurrentCast = nullptr;
	CastingState.bInterruptible = false;
	CastingState.ElapsedTicks = 0;
	CastingState.CastStartTime = 0.0f;
	CastingState.CastEndTime = 0.0f;
	OnCastStateChanged.Broadcast(PreviousState, GetCastingState());
}

FCastEvent UAbilityComponent::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FCastEvent Result;
	if (GetOwnerRole() != ROLE_Authority)
	{
		return Result;
	}
	
	Result.Ability = FindActiveAbility(AbilityClass);
	if (!IsValid(Result.Ability))
	{
		return Result;
	}
	
	if (Result.Ability->GetHasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive)
	{
		return Result;
	}
	
	if (GetCastingState().bIsCasting)
	{
		return Result;
	}
	
	TArray<FAbilityCost> Costs;
	Result.Ability->GetAbilityCosts(Costs);
	if (Costs.Num() > 0)
	{
		if (!IsValid(ResourceHandler))
		{
			return Result;
		}
		if (!ResourceHandler->CheckAbilityCostsMet(Result.Ability, Costs))
		{
			return Result;
		}
	}

	if (!Result.Ability->CheckChargesMet())
	{
		return Result;
	}

	if (!Result.Ability->CheckCustomCastConditionsMet())
	{
		return Result;
	}
	
	//TODO: CC and Lockout Checks.
	
	if (Result.Ability->GetHasGlobalCooldown())
	{
		StartGlobalCooldown(Result.Ability);
	}
	
	Result.Ability->CommitCharges();
	
	if (Costs.Num() > 0 && IsValid(ResourceHandler))
	{
		ResourceHandler->CommitAbilityCosts(Result.Ability, 0, Costs);
	}
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		return Result;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->InitialTick();
		OnAbilityStart.Broadcast(Result);
		OnAbilityComplete.Broadcast(Result);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->InitialTick();
		}
		StartCast(Result.Ability);
		OnAbilityStart.Broadcast(Result);
		break;
	default :
		return Result;
	}
	
	return Result;
}

FCancelEvent UAbilityComponent::CancelCurrentCast()
{
	FCancelEvent Result;
	UCombatAbility* Ability = GetCastingState().CurrentCast;
	if (GetCastingState().bIsCasting && IsValid(Ability))
	{
		Result.CancelledAbility = Ability;
		Result.CancelTime = GameStateRef->GetWorldTime();
		Result.CancelID = 0;
		Result.CancelledCastStart = GetCastingState().CastStartTime;
		Result.CancelledCastEnd = GetCastingState().CastEndTime;
		Result.CancelledCastID = GetCastingState().CurrentCastID;
		Result.ElapsedTicks = GetCastingState().ElapsedTicks;
		GetWorld()->GetTimerManager().ClearTimer(CastHandle);
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);
		EndCast();
		Ability->CancelCast();
		OnAbilityCancelled.Broadcast(Result);
	}
	return Result;
}