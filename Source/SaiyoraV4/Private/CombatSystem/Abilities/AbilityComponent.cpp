#include "AbilityComponent.h"
#include "AbilityStructs.h"
#include "Buff.h"
#include "CombatAbility.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"
#include "UnrealNetwork.h"
#include "GameFramework/GameState.h"

#pragma region Setup

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Ability Handler."), *GetOwner()->GetActorLabel());
	GameStateRef = GetWorld()->GetGameState<AGameState>();
	checkf(IsValid(GameStateRef), TEXT("%s got an invalid Game State Ref in Ability Handler."), *GetOwner()->GetActorLabel());
	APawn* OwnerAsPawn = Cast<APawn>(GetOwner());
	bLocallyControlled = IsValid(OwnerAsPawn) && OwnerAsPawn->IsLocallyControlled();
	ResourceHandlerRef = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwner());
	StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	CrowdControlHandlerRef = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwner());
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	if (IsValid(StatHandlerRef))
	{
		StatCooldownMod.BindDynamic(this, &UAbilityComponent::ModifyCooldownFromStat);
		AddCooldownModifier(StatCooldownMod);
		StatGlobalCooldownMod.BindDynamic(this, &UAbilityComponent::ModifyGlobalCooldownFromStat);
		AddGlobalCooldownModifier(StatGlobalCooldownMod);
		StatCastLengthMod.BindDynamic(this, &UAbilityComponent::ModifyCastLengthFromStat);
		AddCastLengthModifier(StatCastLengthMod);
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(CrowdControlHandlerRef))
		{
			CcCallback.BindDynamic(this, &UAbilityComponent::InterruptCastOnCrowdControl);
			CrowdControlHandlerRef->SubscribeToCrowdControlChanged(CcCallback);
		}
		if (IsValid(DamageHandlerRef))
		{
			DeathCallback.BindDynamic(this, &UAbilityComponent::InterruptCastOnDeath);
			DamageHandlerRef->SubscribeToLifeStatusChanged(DeathCallback);
		}
		for (TSubclassOf<UCombatAbility> const AbilityClass : DefaultAbilities)
		{
			AddNewAbility(AbilityClass);
		}
	}	
}

void UAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UAbilityComponent, CastingState, COND_SkipOwner);
}

#pragma endregion 
#pragma region Ability Management

UCombatAbility* UAbilityComponent::AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass,
	bool const bIgnoreRestrictions)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AbilityClass) || ActiveAbilities.Contains(AbilityClass))
	{
		return nullptr;
	}
	if (!bIgnoreRestrictions)
	{
		for (FAbilityClassRestriction const& Restriction : AbilityAcquisitionRestrictions)
		{
			if (Restriction.IsBound() && Restriction.Execute(AbilityClass))
			{
				return nullptr;
			}
		}
	}
	UCombatAbility* NewAbility = NewObject<UCombatAbility>(GetOwner(), AbilityClass);
	if (IsValid(NewAbility))
	{
		ActiveAbilities.Add(AbilityClass, NewAbility);
		NewAbility->InitializeAbility(this);
		OnAbilityAdded.Broadcast(NewAbility);
	}
	return NewAbility;
}

void UAbilityComponent::NotifyOfReplicatedAbility(UCombatAbility* NewAbility)
{
	if (IsValid(NewAbility) && !ActiveAbilities.Contains(NewAbility->GetClass()))
	{
		ActiveAbilities.Add(NewAbility->GetClass(), NewAbility);
		NewAbility->InitializeAbility(this);
		OnAbilityAdded.Broadcast(NewAbility);
	}
}

void UAbilityComponent::RemoveAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AbilityClass))
	{
		return;
	}
	UCombatAbility* AbilityToRemove = ActiveAbilities.FindRef(AbilityClass);
	if (!IsValid(AbilityToRemove))
	{
		return;
	}
	AbilityToRemove->DeactivateAbility();
	RecentlyRemovedAbilities.Add(AbilityToRemove);
	ActiveAbilities.Remove(AbilityClass);
	OnAbilityRemoved.Broadcast(AbilityToRemove);
	FTimerHandle RemovalHandle;
	FTimerDelegate const RemovalDelegate = FTimerDelegate::CreateUObject(this, &UAbilityComponent::CleanupRemovedAbility, AbilityToRemove);
	GetWorld()->GetTimerManager().SetTimer(RemovalHandle, RemovalDelegate, 1.0f, false);
}

void UAbilityComponent::NotifyOfReplicatedAbilityRemoval(UCombatAbility* RemovedAbility)
{
	if (IsValid(RemovedAbility))
	{
		UCombatAbility* ExistingAbility = ActiveAbilities.FindRef(RemovedAbility->GetClass());
		if (IsValid(ExistingAbility) && ExistingAbility == RemovedAbility)
		{
			RemovedAbility->DeactivateAbility();
			ActiveAbilities.Remove(RemovedAbility->GetClass());
			OnAbilityRemoved.Broadcast(RemovedAbility);
		}
	}
}

#pragma endregion
#pragma region Ability Usage

FAbilityEvent UAbilityComponent::UseAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	FAbilityEvent Result;
	//TODO: Clear queue.
	if (!bLocallyControlled || !IsValid(AbilityClass))
	{
		Result.FailReason = ECastFailReason::NetRole;
		return Result;
	}
	Result.Ability = ActiveAbilities.FindRef(AbilityClass);
	if (!CanUseAbility(Result.Ability, Result.FailReason))
	{
		return Result;
	}
	int32 const PredictionID = GetOwnerRole() == ROLE_Authority ? 0 : GenerateNewPredictionID();
	if (Result.Ability->HasGlobalCooldown())
	{
		StartGlobalCooldown(Result.Ability, PredictionID);
	}
	Result.Ability->CommitCharges(PredictionID);
	if (IsValid(ResourceHandlerRef))
	{
		//TODO: For prediction, resource handler should keep a record of predicted costs, then ability handler can send the updated list of costs from server RPC.
		ResourceHandlerRef->CommitAbilityCosts(Result.Ability, PredictionID);
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		switch (Result.Ability->GetCastType())
		{
		case EAbilityCastType::Instant :
			{
				Result.ActionTaken = ECastAction::Success;
				Result.Ability->PredictedTick(0, Result.PredictionParams);
				Result.Ability->ServerTick(0, Result.PredictionParams, Result.BroadcastParams);
				MulticastAbilityTick(Result);
				OnAbilityTick.Broadcast(Result);
			}
			break;
		case EAbilityCastType::Channel :
			{
				Result.ActionTaken = ECastAction::Success;
				StartCast(Result.Ability);
				if (Result.Ability->HasInitialTick())
				{
					Result.Ability->PredictedTick(0, Result.PredictionParams);
					Result.Ability->ServerTick(0, Result.PredictionParams, Result.BroadcastParams);
					MulticastAbilityTick(Result);
					OnAbilityTick.Broadcast(Result);
				}
			}
			break;
		default :
			Result.FailReason = ECastFailReason::InvalidCastType;
			return Result;
		}
	}
	else if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FAbilityRequest Request;
		Request.AbilityClass = Result.Ability->GetClass();
		Request.PredictionID = PredictionID;
		Request.ClientStartTime = GameStateRef->GetServerWorldTimeSeconds();
		switch (Result.Ability->GetCastType())
		{
			case EAbilityCastType::Instant :
				{
					Result.ActionTaken = ECastAction::Success;
					Result.Ability->PredictedTick(0, Result.PredictionParams, PredictionID);
					Request.PredictionParams = Result.PredictionParams;
					OnAbilityTick.Broadcast(Result);
					break;
				}
			case EAbilityCastType::Channel :
				{
					Result.ActionTaken = ECastAction::Success;
					StartCast(Result.Ability, Result.PredictionID);
                    if (Result.Ability->HasInitialTick())
                    {
                    	Result.Ability->PredictedTick(0, Result.PredictionParams, Result.PredictionID);
                    	Request.PredictionParams = Result.PredictionParams;
                    	OnAbilityTick.Broadcast(Result);
                    }
                    break;
				}
			default :
				Result.FailReason = ECastFailReason::InvalidCastType;
				return Result;
		}
		FClientAbilityPrediction Prediction;
		Prediction.Ability = Result.Ability;
		Prediction.ClientTime = GameStateRef->GetServerWorldTimeSeconds();
		Prediction.bPredictedGCD = Result.Ability->HasGlobalCooldown();
		Prediction.bPredictedCastBar = Result.Ability->GetCastType() == EAbilityCastType::Channel;
		UnackedAbilityPredictions.Add(PredictionID, Prediction);
		ServerPredictAbility(Request);
	}
	return Result;
}

void UAbilityComponent::ServerPredictAbility_Implementation(FAbilityRequest const& Request)
{
	//TODO: Merge this function with the predicted tick function? One centralized place to predict will let me do predicted movement on ticks possibly.s
}

int32 UAbilityComponent::GenerateNewPredictionID()
{
	int32 const Previous = LastPredictionID;
	LastPredictionID = LastPredictionID + 1;
	if (LastPredictionID == 0)
	{
		LastPredictionID++;
	}
	if (LastPredictionID < Previous)
	{
		UE_LOG(LogTemp, Warning, TEXT("PredictionID overflowed into the negatives."));
	}
	return LastPredictionID;
}

#pragma endregion
#pragma region Casting

void UAbilityComponent::StartCast(UCombatAbility* Ability, int32 const PredictionID)
{
	FCastingState const PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	if (PredictionID != 0)
	{
		CastingState.PredictionID = PredictionID;
	}
	CastingState.CastStartTime = GameStateRef->GetServerWorldTimeSeconds();
	float const CastLength = GetOwnerRole() == ROLE_Authority ? CalculateCastLength(Ability) : 0.0f;
	CastingState.CastEndTime = GetOwnerRole() == ROLE_Authority ? CastingState.CastStartTime + CastLength : 0.0f;
	CastingState.bInterruptible = Ability->IsInterruptible();
	CastingState.ElapsedTicks = 0;
	if (GetOwnerRole() == ROLE_Authority)
	{
		GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityComponent::TickCurrentCast,
			(CastLength / Ability->GetNumberOfTicks()), true);
	}
    OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UAbilityComponent::EndCast()
{
	FCastingState const PreviousState = CastingState;
    CastingState.bIsCasting = false;
    CastingState.CurrentCast = nullptr;
   	CastingState.bInterruptible = false;
    CastingState.ElapsedTicks = 0;
    CastingState.CastStartTime = 0.0f;
    CastingState.CastEndTime = 0.0f;
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
    OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UAbilityComponent::TickCurrentCast()
{
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		return;
	}
	CastingState.ElapsedTicks++;
	FAbilityEvent TickEvent;
	TickEvent.Ability = CastingState.CurrentCast;
	TickEvent.PredictionID = CastingState.PredictionID;
	TickEvent.Tick = CastingState.ElapsedTicks;
	TickEvent.ActionTaken = CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNumberOfTicks() ? ECastAction::Complete : ECastAction::Tick;
	bool bTickFired = false;
	if (GetOwnerRole() == ROLE_Authority && bLocallyControlled)
	{
		CastingState.CurrentCast->PredictedTick(CastingState.ElapsedTicks, TickEvent.PredictionParams);
		CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, TickEvent.PredictionParams, TickEvent.BroadcastParams);
		bTickFired = true;
	}
	else if (GetOwnerRole() == ROLE_Authority)
	{
		RemoveExpiredTicks();
		FPredictedTick const CurrentTick = FPredictedTick(CastingState.PredictionID, CastingState.ElapsedTicks);
		if (ParamsAwaitingTicks.Contains(CurrentTick))
		{
			TickEvent.PredictionParams = ParamsAwaitingTicks.FindRef(CurrentTick);
			CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, TickEvent.PredictionParams, TickEvent.BroadcastParams, CastingState.PredictionID);
			ParamsAwaitingTicks.Remove(CurrentTick);
			bTickFired = true;
		}
		else
		{
			TicksAwaitingParams.Add(TickEvent);
			bTickFired = false;
		}
	}
	else if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FAbilityRequest TickRequest;
		TickRequest.PredictionID = CastingState.PredictionID;
		TickRequest.Tick = CastingState.ElapsedTicks;
		TickRequest.AbilityClass = CastingState.CurrentCast->GetClass();
		CastingState.CurrentCast->PredictedTick(CastingState.ElapsedTicks, TickEvent.PredictionParams, CastingState.PredictionID);
		TickRequest.PredictionParams = TickEvent.PredictionParams;
		ServerHandlePredictedTick(TickRequest);
		bTickFired = true;
	}
	if (bTickFired)
	{
		OnAbilityTick.Broadcast(TickEvent);
		if (GetOwnerRole() == ROLE_Authority)
		{
			MulticastAbilityTick(TickEvent);
		}
	}
	if (CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNumberOfTicks())
	{
		EndCast();
	}
}

void UAbilityComponent::ServerHandlePredictedTick_Implementation(FAbilityRequest const& TickRequest)
{
	if (CastingState.bIsCasting && IsValid(CastingState.CurrentCast) && CastingState.CurrentCast->GetClass() == TickRequest.AbilityClass &&
		CastingState.PredictionID == TickRequest.PredictionID && CastingState.ElapsedTicks < TickRequest.Tick)
	{
		FPredictedTick const Tick = FPredictedTick(TickRequest.PredictionID, TickRequest.Tick);
		ParamsAwaitingTicks.Add(Tick, TickRequest.PredictionParams);
		//TODO: If within a certain tolerance, maybe just perform the ability and increase next tick timer by the remaining time?
		//This would help with race conditions between PlayerAbilityHandler and CMC.
		return;
	}
	for (int i = 0; i < TicksAwaitingParams.Num(); i++)
	{
		if (TicksAwaitingParams[i].PredictionID == TickRequest.PredictionID && TicksAwaitingParams[i].Tick == TickRequest.Tick &&
			IsValid(TicksAwaitingParams[i].Ability) && TicksAwaitingParams[i].Ability->GetClass() == TickRequest.AbilityClass)
		{
			TicksAwaitingParams[i].PredictionParams = TickRequest.PredictionParams;
			TicksAwaitingParams[i].Ability->ServerTick(TicksAwaitingParams[i].Tick, TicksAwaitingParams[i].PredictionParams, TicksAwaitingParams[i].BroadcastParams, TicksAwaitingParams[i].PredictionID);
			OnAbilityTick.Broadcast(TicksAwaitingParams[i]);
			MulticastAbilityTick(TicksAwaitingParams[i]);
			TicksAwaitingParams.RemoveAt(i);
			return;
		}
	}
}

void UAbilityComponent::RemoveExpiredTicks()
{
	for (int i = TicksAwaitingParams.Num() - 1; i >= 0; i--)
	{
		if ((TicksAwaitingParams[i].PredictionID == CastingState.PredictionID && TicksAwaitingParams[i].Tick < CastingState.ElapsedTicks - 1) ||
			(TicksAwaitingParams[i].PredictionID < CastingState.PredictionID && CastingState.ElapsedTicks >= 1))
		{
			TicksAwaitingParams.RemoveAt(i);
		}
	}
	TArray<FPredictedTick> ExpiredTicks;
	for (TTuple<FPredictedTick, FCombatParameters> const& WaitingParams : ParamsAwaitingTicks)
	{
		if ((WaitingParams.Key.PredictionID == CastingState.PredictionID && WaitingParams.Key.TickNumber < CastingState.ElapsedTicks - 1) ||
			(WaitingParams.Key.PredictionID < CastingState.PredictionID && CastingState.ElapsedTicks >= 1))
		{
			ExpiredTicks.Add(WaitingParams.Key);
		}
	}
	for (FPredictedTick const& ExpiredTick : ExpiredTicks)
	{
		ParamsAwaitingTicks.Remove(ExpiredTick);
	}
}

#pragma endregion 
#pragma region Global Cooldown

void UAbilityComponent::StartGlobalCooldown(UCombatAbility* Ability, int32 const PredictionID)
{
	FGlobalCooldown const PreviousState = GlobalCooldownState;
	GlobalCooldownState.bGlobalCooldownActive = true;
	if (PredictionID != 0)
	{
		GlobalCooldownState.PredictionID = PredictionID;
	}
	GlobalCooldownState.StartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	if (GetOwnerRole() == ROLE_Authority)
	{
		float const GlobalLength = GlobalCooldownState.StartTime + CalculateGlobalCooldownLength(Ability);
		GlobalCooldownState.EndTime = GlobalCooldownState.StartTime + GlobalLength;
		GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UAbilityComponent::EndGlobalCooldown, GlobalLength, false);
	}
	else
	{
		GlobalCooldownState.EndTime = 0.0f;
	}
	OnGlobalCooldownChanged.Broadcast(PreviousState, GlobalCooldownState);
}

void UAbilityComponent::EndGlobalCooldown()
{
	if (GlobalCooldownState.bGlobalCooldownActive)
	{
		FGlobalCooldown const PreviousGlobal;
		GlobalCooldownState.bGlobalCooldownActive = false;
		GlobalCooldownState.StartTime = 0.0f;
		GlobalCooldownState.EndTime = 0.0f;
		GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
		OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
		if (bLocallyControlled)
		{
			//TODO: Check for queued ability.
		}
	}
}

#pragma endregion 
#pragma region Subscriptions

void UAbilityComponent::SubscribeToAbilityAdded(FAbilityInstanceCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityAdded.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromAbilityAdded(FAbilityInstanceCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityAdded.Remove(Callback);
	}
}

void UAbilityComponent::SubscribeToAbilityRemoved(FAbilityInstanceCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityRemoved.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromAbilityRemoved(FAbilityInstanceCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityRemoved.Remove(Callback);
	}
}

void UAbilityComponent::SubscribeToGlobalCooldownChanged(FGlobalCooldownCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnGlobalCooldownChanged.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromGlobalCooldownChanged(FGlobalCooldownCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnGlobalCooldownChanged.Remove(Callback);
	}
}

void UAbilityComponent::SubscribeToCastStateChanged(FCastingStateCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnCastStateChanged.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromCastStateChanged(FCastingStateCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnCastStateChanged.Remove(Callback);
	}
}

#pragma endregion
#pragma region Modifiers

void UAbilityComponent::AddGlobalCooldownModifier(FAbilityModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		GlobalCooldownMods.AddUnique(Modifier);
	}
}

void UAbilityComponent::RemoveGlobalCooldownModifier(FAbilityModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		GlobalCooldownMods.Remove(Modifier);
	}
}

FCombatModifier UAbilityComponent::ModifyGlobalCooldownFromStat(TSubclassOf<UCombatAbility> AbilityClass)
{
	if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(GlobalCooldownLengthStatTag()))
	{
		return FCombatModifier(StatHandlerRef->GetStatValue(GlobalCooldownLengthStatTag()), EModifierType::Multiplicative);
	}
	return FCombatModifier::InvalidMod;
}

float UAbilityComponent::CalculateGlobalCooldownLength(UCombatAbility* Ability)
{
	if (!IsValid(Ability) || !Ability->HasGlobalCooldown())
	{
		return -1.0f;
	}
	if (Ability->HasStaticGlobalCooldown())
	{
		return Ability->GetGlobalCooldownLength();
	}
	TArray<FCombatModifier> Mods;
	for (FAbilityModCondition const& Mod : GlobalCooldownMods)
	{
		if (Mod.IsBound())
		{
			Mods.Add(Mod.Execute(Ability->GetClass()));
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, Ability->GetGlobalCooldownLength());
}

void UAbilityComponent::AddCastLengthModifier(FAbilityModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		CastLengthMods.AddUnique(Modifier);
	}
}

void UAbilityComponent::RemoveCastLengthModifier(FAbilityModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.IsBound())
	{
		CastLengthMods.Remove(Modifier);
	}
}

FCombatModifier UAbilityComponent::ModifyCastLengthFromStat(TSubclassOf<UCombatAbility> AbilityClass)
{
	if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(CastLengthStatTag()))
	{
		return FCombatModifier(StatHandlerRef->GetStatValue(CastLengthStatTag()), EModifierType::Multiplicative);
	}
	return FCombatModifier::InvalidMod;
}

float UAbilityComponent::CalculateCastLength(UCombatAbility* Ability)
{
	if (!IsValid(Ability) || Ability->GetCastType() != EAbilityCastType::Channel)
	{
		return -1.0f;
	}
	if (Ability->HasStaticCastLength())
	{
		return Ability->GetCastLength();
	}
	TArray<FCombatModifier> Mods;
	for (FAbilityModCondition const& Mod : CastLengthMods)
	{
		if (Mod.IsBound())
		{
			Mods.Add(Mod.Execute(Ability->GetClass()));
		}
	}
	return FCombatModifier::ApplyModifiers(Mods, Ability->GetCastLength());
}

#pragma endregion 
#pragma region Restrictions

void UAbilityComponent::AddAbilityAcquisitionRestriction(FAbilityClassRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		AbilityAcquisitionRestrictions.AddUnique(Restriction);
		TArray<TSubclassOf<UCombatAbility>> ActiveAbilityClasses;
		ActiveAbilities.GenerateKeyArray(ActiveAbilityClasses);
		for (TSubclassOf<UCombatAbility> const AbilityClass : ActiveAbilityClasses)
		{
			if (Restriction.Execute(AbilityClass))
			{
				RemoveAbility(AbilityClass);
			}
		}
	}
}

void UAbilityComponent::RemoveAbilityAcquisitionRestriction(FAbilityClassRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && Restriction.IsBound())
	{
		AbilityAcquisitionRestrictions.Remove(Restriction);
	}
}

void UAbilityComponent::AddAbilityTagRestriction(UBuff* Source, FGameplayTag const Tag)
{
	if (!IsValid(Source) || !Tag.IsValid() || !Tag.MatchesTag(AbilityRestrictionTag()) || Tag.MatchesTagExact(AbilityRestrictionTag()))
	{
		return;
	}
	bool const bAlreadyRestricted = AbilityUsageTagRestrictions.Num(Tag) > 0;
	AbilityUsageTagRestrictions.AddUnique(Tag, Source);
	if (!bAlreadyRestricted)
	{
		for (TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*> const& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Value) && AbilityPair.Value->HasTag(Tag))
			{
				AbilityPair.Value->AddRestrictedTag(Tag);
			}
		}
	}
}

void UAbilityComponent::RemoveAbilityTagRestriction(UBuff* Source, FGameplayTag const Tag)
{
	if (!IsValid(Source) || !Tag.IsValid() || !Tag.MatchesTag(AbilityRestrictionTag()) || Tag.MatchesTagExact(AbilityRestrictionTag()))
	{
		return;
	}
	if (AbilityUsageTagRestrictions.Remove(Tag, Source) > 0 && AbilityUsageTagRestrictions.Num(Tag) == 0)
	{
		for (TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*> const& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Value) && AbilityPair.Value->HasTag(Tag))
			{
				AbilityPair.Value->RemoveRestrictedTag(Tag);
			}
		}
	}
}

void UAbilityComponent::AddAbilityClassRestriction(UBuff* Source, TSubclassOf<UCombatAbility> const Class)
{
	if (!IsValid(Source) || !IsValid(Class))
	{
		return;
	}
	bool const bAlreadyRestricted = AbilityUsageClassRestrictions.Num(Class);
	AbilityUsageClassRestrictions.AddUnique(Class, Source);
	if (!bAlreadyRestricted)
	{
		for (TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*> const& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Key) && IsValid(AbilityPair.Value) && AbilityPair.Key == Class)
			{
				AbilityPair.Value->AddRestrictedTag(AbilityClassRestrictionTag());
			}
		}
	}
}

void UAbilityComponent::RemoveAbilityClassRestriction(UBuff* Source, TSubclassOf<UCombatAbility> const Class)
{
	if (!IsValid(Source) || !IsValid(Class))
	{
		return;
	}
	if (AbilityUsageClassRestrictions.Remove(Class, Source) > 0 && AbilityUsageClassRestrictions.Num(Class) == 0)
	{
		for (TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*> const& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Key) && IsValid(AbilityPair.Value) && AbilityPair.Key == Class)
			{
				AbilityPair.Value->RemoveRestrictedTag(AbilityClassRestrictionTag());
			}
		}
	}
}

bool UAbilityComponent::CanUseAbility(UCombatAbility* Ability, ECastFailReason& OutFailReason) const
{
	if (!IsValid(Ability))
	{
		OutFailReason = ECastFailReason::InvalidAbility;
		return false;
	}
	if (!Ability->GetCastableWhileDead() && IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		OutFailReason = ECastFailReason::Dead;
		return false;
	}
	if (Ability->HasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive)
	{
		OutFailReason = ECastFailReason::OnGlobalCooldown;
		return false;
	}
	if (CastingState.bIsCasting)
	{
		OutFailReason = ECastFailReason::AlreadyCasting;
		return false;
	}
	OutFailReason = Ability->IsCastable();
	if (OutFailReason != ECastFailReason::None)
	{
		return false;
	}
	return true;
}

#pragma endregion
