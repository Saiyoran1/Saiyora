#include "AbilityComponent.h"
#include "AbilityStructs.h"
#include "Buff.h"
#include "CombatAbility.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"
#include "ResourceHandler.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/GameState.h"

#pragma region Setup

float const UAbilityComponent::MaxPingCompensation = 0.2f;
float const UAbilityComponent::MinimumCastLength = 0.5f;
float const UAbilityComponent::MinimumGlobalCooldownLength = 0.5f;
float const UAbilityComponent::MinimumCooldownLength = 0.5f;
float const UAbilityComponent::AbilityQueWindow = 0.2f;

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool UAbilityComponent::IsLocallyControlled() const
{
	APawn* OwnerAsPawn = Cast<APawn>(GetOwner());
	return IsValid(OwnerAsPawn) && OwnerAsPawn->IsLocallyControlled();
}


void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Ability Handler."));
	GameStateRef = GetWorld()->GetGameState<AGameState>();
	checkf(IsValid(GameStateRef), TEXT("Got an invalid Game State Ref in Ability Handler."));
	ResourceHandlerRef = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwner());
	StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	CrowdControlHandlerRef = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwner());
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
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

bool UAbilityComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	TArray<UCombatAbility*> RepAbilities;
	ActiveAbilities.GenerateValueArray(RepAbilities);
	RepAbilities.Append(RecentlyRemovedAbilities);
	bWroteSomething |= Channel->ReplicateSubobjectList(RepAbilities, *Bunch, *RepFlags);
	return bWroteSomething;
}

#pragma endregion 
#pragma region Ability Management

UCombatAbility* UAbilityComponent::AddNewAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AbilityClass) || ActiveAbilities.Contains(AbilityClass))
	{
		return nullptr;
	}
	UCombatAbility* NewAbility = NewObject<UCombatAbility>(GetOwner(), AbilityClass);
	if (IsValid(NewAbility))
	{
		ActiveAbilities.Add(AbilityClass, NewAbility);
		NewAbility->InitializeAbility(this);
		if (AbilityUsageClassRestrictions.Contains(AbilityClass))
		{
			NewAbility->AddRestrictedTag(AbilityClassRestrictionTag());
		}
		TArray<FGameplayTag> RestrictedTags;
		AbilityUsageTagRestrictions.GenerateKeyArray(RestrictedTags);
		for (FGameplayTag const Tag : RestrictedTags)
		{
			if (NewAbility->HasTag(Tag))
			{
				NewAbility->AddRestrictedTag(Tag);
			}
		}
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
	bool const bFromQueue = bUsingAbilityFromQueue;
	bUsingAbilityFromQueue = false;
	ExpireQueue();
	FAbilityEvent Result;
	if (!IsLocallyControlled() || !IsValid(AbilityClass))
	{
		Result.FailReason = ECastFailReason::NetRole;
		return Result;
	}
	Result.Ability = ActiveAbilities.FindRef(AbilityClass);
	if (!CanUseAbility(Result.Ability, Result.FailReason))
	{
		if (!bFromQueue)
		{
			if (Result.FailReason == ECastFailReason::AlreadyCasting)
			{
				if (TryQueueAbility(AbilityClass))
				{
					Result.FailReason = ECastFailReason::Queued;
				}
				else
				{
					CancelCurrentCast();
					bUsingAbilityFromQueue = true;
					return UseAbility(AbilityClass);
				}
			}
			else if (Result.FailReason == ECastFailReason::OnGlobalCooldown)
			{
				if (TryQueueAbility(AbilityClass))
				{
					Result.FailReason = ECastFailReason::Queued;
				}
			}
		}
		return Result;
	}
	Result.PredictionID = GetOwnerRole() == ROLE_Authority ? 0 : GenerateNewPredictionID();
	if (Result.Ability->HasGlobalCooldown())
	{
		StartGlobalCooldown(Result.Ability, Result.PredictionID);
	}
	Result.Ability->CommitCharges(Result.PredictionID);
	if (IsValid(ResourceHandlerRef))
	{
		ResourceHandlerRef->CommitAbilityCosts(Result.Ability, Result.PredictionID);
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		switch (Result.Ability->GetCastType())
		{
		case EAbilityCastType::Instant :
			{
				Result.ActionTaken = ECastAction::Success;
				Result.Ability->PredictedTick(0, Result.Targets);
				Result.Ability->ServerTick(0, Result.Targets);
				MulticastAbilityTick(Result);
			}
			break;
		case EAbilityCastType::Channel :
			{
				Result.ActionTaken = ECastAction::Success;
				StartCast(Result.Ability);
				if (Result.Ability->HasInitialTick())
				{
					Result.Ability->PredictedTick(0, Result.Targets);
					Result.Ability->ServerTick(0, Result.Targets);
					MulticastAbilityTick(Result);
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
		Request.PredictionID = Result.PredictionID;
		Request.ClientStartTime = GameStateRef->GetServerWorldTimeSeconds();
		switch (Result.Ability->GetCastType())
		{
			case EAbilityCastType::Instant :
				{
					Result.ActionTaken = ECastAction::Success;
					Result.Ability->PredictedTick(0, Result.Targets);
					Request.Targets = Result.Targets;
					OnAbilityTick.Broadcast(Result);
					break;
				}
			case EAbilityCastType::Channel :
				{
					Result.ActionTaken = ECastAction::Success;
					StartCast(Result.Ability, Result.PredictionID);
                    if (Result.Ability->HasInitialTick())
                    {
                    	Result.Ability->PredictedTick(0, Result.Targets);
                    	Request.Targets = Result.Targets;
                    	OnAbilityTick.Broadcast(Result);
                    }
                    break;
				}
			default :
				UE_LOG(LogTemp, Warning, TEXT("Defaulted on cast type for predicted ability."));
				return Result;
		}
		FClientAbilityPrediction Prediction;
		Prediction.Ability = Result.Ability;
		Prediction.bPredictedGCD = Result.Ability->HasGlobalCooldown();
		Prediction.bPredictedCastBar = Result.Ability->GetCastType() == EAbilityCastType::Channel;
		UnackedAbilityPredictions.Add(Result.PredictionID, Prediction);
		ServerPredictAbility(Request);
	}
	return Result;
}

void UAbilityComponent::ServerPredictAbility_Implementation(FAbilityRequest const& Request)
{
	if (PredictedTickRecord.Contains(FPredictedTick(Request.PredictionID, Request.Tick)))
	{
		return;
	}
	if (Request.Tick == 0)
	{
		FServerAbilityResult ServerResult;
		ServerResult.PredictionID = Request.PredictionID;
		ServerResult.AbilityClass = Request.AbilityClass;
		if (Request.PredictionID <= LastPredictionID)
		{
			ServerResult.FailReason = ECastFailReason::NetRole;
			PredictedTickRecord.Add(FPredictedTick(Request.PredictionID, 0), false);
			ClientPredictionResult(ServerResult);
			return;
		}
		LastPredictionID = Request.PredictionID;
		if (!IsValid(Request.AbilityClass))
		{
			ServerResult.FailReason = ECastFailReason::InvalidAbility;
			PredictedTickRecord.Add(FPredictedTick(Request.PredictionID, 0), false);
			ClientPredictionResult(ServerResult);
			return;
		}
		UCombatAbility* Ability = ActiveAbilities.FindRef(Request.AbilityClass);
		if (!CanUseAbility(Ability, ServerResult.FailReason))
		{
			PredictedTickRecord.Add(FPredictedTick(Request.PredictionID, 0), false);
			ClientPredictionResult(ServerResult);
			return;
		}
        //This should probably be manually calculated using ping value and server time, instead of trusting the client.
        //TODO: Update this once I've got Ping worked out and am confident in it being accurate.
        ServerResult.ClientStartTime = Request.ClientStartTime;
		ServerResult.bSuccess = true;
		
		FAbilityEvent Result;
		Result.Ability = Ability;
		Result.PredictionID = Request.PredictionID;
		Result.Targets = Request.Targets;
        
        if (Ability->HasGlobalCooldown())
        {
        	StartGlobalCooldown(Ability, Request.PredictionID);
        	ServerResult.bActivatedGlobal = true;
        	ServerResult.GlobalLength = CalculateGlobalCooldownLength(Ability, false);
        }
        
        int32 const PreviousCharges = Ability->GetCurrentCharges();
        Ability->CommitCharges(Request.PredictionID);
        int32 const NewCharges = Ability->GetCurrentCharges();
        ServerResult.ChargesSpent = PreviousCharges - NewCharges;
        
        if (IsValid(ResourceHandlerRef))
        {
        	ResourceHandlerRef->CommitAbilityCosts(Ability, Request.PredictionID);
        	Ability->GetAbilityCosts(ServerResult.AbilityCosts);
        }
        	
        switch (Ability->GetCastType())
        {
        case EAbilityCastType::Instant :
	        {
        		Result.ActionTaken = ECastAction::Success;
        		Ability->ServerTick(0, Result.Targets, Result.PredictionID);
        		MulticastAbilityTick(Result);
	        }
        	break;
        case EAbilityCastType::Channel :
	        {
        		Result.ActionTaken = ECastAction::Success;
        		StartCast(Ability, Result.PredictionID);
        		ServerResult.bActivatedCastBar = true;
        		ServerResult.CastLength = CalculateCastLength(Ability, false);
        		ServerResult.bInterruptible = CastingState.bInterruptible;
        		if (Ability->HasInitialTick())
        		{
        			Ability->ServerTick(0, Result.Targets, Result.PredictionID);
        			MulticastAbilityTick(Result);
        		}
	        }
        	break;
        default :
        	UE_LOG(LogTemp, Warning, TEXT("Defaulted on cast type for predicted ability."));
        	return;
        }

		PredictedTickRecord.Add(FPredictedTick(Request.PredictionID, 0), true);
        ClientPredictionResult(ServerResult);
	}
	else
	{
		if (CastingState.bIsCasting && IsValid(CastingState.CurrentCast) && CastingState.CurrentCast->GetClass() == Request.AbilityClass &&
			CastingState.PredictionID == Request.PredictionID && CastingState.ElapsedTicks < Request.Tick)
		{
			for (FAbilityTarget const& Target : Request.Targets)
			{
				ParamsAwaitingTicks.Add(FPredictedTick(Request.PredictionID, Request.Tick), Target);
			}
			//TODO: If within a certain tolerance, maybe just perform the ability and increase next tick timer by the remaining time?
			//This would help with race conditions between PlayerAbilityHandler and CMC.
			return;
		}
		for (int i = 0; i < TicksAwaitingParams.Num(); i++)
		{
			if (TicksAwaitingParams[i].PredictionID == Request.PredictionID && TicksAwaitingParams[i].Tick == Request.Tick &&
				IsValid(TicksAwaitingParams[i].Ability) && TicksAwaitingParams[i].Ability->GetClass() == Request.AbilityClass)
			{
				TicksAwaitingParams[i].Targets = Request.Targets;
				TicksAwaitingParams[i].Ability->ServerTick(TicksAwaitingParams[i].Tick, TicksAwaitingParams[i].Targets, TicksAwaitingParams[i].PredictionID);
				MulticastAbilityTick(TicksAwaitingParams[i]);
				OnAbilityTick.Broadcast(TicksAwaitingParams[i]);
				PredictedTickRecord.Add(FPredictedTick(TicksAwaitingParams[i].PredictionID, TicksAwaitingParams[i].Tick), true);
				TicksAwaitingParams.RemoveAt(i);
				return;
			}
		}
	}
}

void UAbilityComponent::ClientPredictionResult_Implementation(FServerAbilityResult const& Result)
{
	FClientAbilityPrediction* OriginalPrediction = UnackedAbilityPredictions.Find(Result.PredictionID);
	if (OriginalPrediction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Client did not have an ability prediction stored that matches server result with prediction ID %i."), Result.PredictionID);
		return;
	}
	if (IsValid(ResourceHandlerRef))
	{
		ResourceHandlerRef->UpdatePredictedCostsFromServer(Result);
	}
	if (IsValid(OriginalPrediction->Ability))
	{
		OriginalPrediction->Ability->UpdatePredictionFromServer(Result);
	}
	if (CastingState.PredictionID <= Result.PredictionID)
	{
		CastingState.PredictionID = Result.PredictionID;
		if (Result.bSuccess && Result.bActivatedCastBar && Result.ClientStartTime + Result.CastLength > GameStateRef->GetServerWorldTimeSeconds())
		{
			FCastingState const PreviousState = CastingState;
			CastingState.bIsCasting = true;
			CastingState.CastStartTime = Result.ClientStartTime;
			CastingState.CastEndTime = CastingState.CastStartTime + Result.CastLength;
			CastingState.CurrentCast = IsValid(OriginalPrediction->Ability) ? OriginalPrediction->Ability : ActiveAbilities.FindRef(Result.AbilityClass);
			CastingState.bInterruptible = Result.bInterruptible;
			//Check for any missed ticks. We will update the last missed tick (if any, this should be rare) after setting everything else.
			float const TickInterval = Result.CastLength / CastingState.CurrentCast->GetNumberOfTicks();
			int32 const ElapsedTicks = FMath::FloorToInt((GameStateRef->GetServerWorldTimeSeconds() - CastingState.CastStartTime) / TickInterval);
			//First iteration of the tick timer will get time remaining until the next tick (to account for travel time). Subsequent ticks use regular interval.
			GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityComponent::TickCurrentCast, TickInterval, true,
				(CastingState.CastStartTime + (TickInterval * (ElapsedTicks + 1))) - GameStateRef->GetServerWorldTimeSeconds());
			OnCastStateChanged.Broadcast(PreviousState, CastingState);
			//Immediately perform the last missed tick if one happened during the wait time between ability prediction and confirmation.
			if (ElapsedTicks > 0)
			{
				//Reduce tick by one since ticking the cast will increment it back up.
				CastingState.ElapsedTicks = ElapsedTicks - 1;
				TickCurrentCast();
			}
			else
			{
				CastingState.ElapsedTicks = ElapsedTicks;
			}
		}
		else if (CastingState.bIsCasting)
		{
			EndCast();
		}
	}
	if (GlobalCooldownState.PredictionID <= Result.PredictionID)
	{
		GlobalCooldownState.PredictionID = Result.PredictionID;
		if (Result.bSuccess && Result.bActivatedGlobal && Result.ClientStartTime + Result.GlobalLength <  GameStateRef->GetServerWorldTimeSeconds())
		{
			FGlobalCooldown const PreviousState = GlobalCooldownState;
			GlobalCooldownState.bGlobalCooldownActive = true;
			GlobalCooldownState.StartTime = Result.ClientStartTime;
			GlobalCooldownState.EndTime = GlobalCooldownState.StartTime + Result.GlobalLength;
			GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
			GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UAbilityComponent::EndGlobalCooldown, GlobalCooldownState.EndTime - GameStateRef->GetServerWorldTimeSeconds(), false);
			OnGlobalCooldownChanged.Broadcast(PreviousState, GlobalCooldownState);
		}
		else if (GlobalCooldownState.bGlobalCooldownActive)
		{
			EndGlobalCooldown();
		}
	}
	if (!Result.bSuccess)
	{
		OnAbilityMispredicted.Broadcast(Result.PredictionID);
	}
	UnackedAbilityPredictions.Remove(Result.PredictionID);
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
	if (GetOwnerRole() == ROLE_Authority && IsLocallyControlled())
	{
		CastingState.CurrentCast->PredictedTick(CastingState.ElapsedTicks, TickEvent.Targets);
		CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, TickEvent.Targets);
		MulticastAbilityTick(TickEvent);
	}
	else if (GetOwnerRole() == ROLE_Authority)
	{
		RemoveExpiredTicks();
		FPredictedTick const CurrentTick = FPredictedTick(CastingState.PredictionID, CastingState.ElapsedTicks);
		if (ParamsAwaitingTicks.Contains(CurrentTick))
		{
			ParamsAwaitingTicks.MultiFind(CurrentTick, TickEvent.Targets);
			CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, TickEvent.Targets, CastingState.PredictionID);
			PredictedTickRecord.Add(CurrentTick, true);
			ParamsAwaitingTicks.Remove(CurrentTick);
			MulticastAbilityTick(TickEvent);
		}
		else
		{
			TicksAwaitingParams.Add(TickEvent);
		}
	}
	else if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FAbilityRequest TickRequest;
		TickRequest.PredictionID = CastingState.PredictionID;
		TickRequest.Tick = CastingState.ElapsedTicks;
		TickRequest.AbilityClass = CastingState.CurrentCast->GetClass();
		CastingState.CurrentCast->PredictedTick(CastingState.ElapsedTicks, TickEvent.Targets, CastingState.PredictionID);
		TickRequest.Targets = TickEvent.Targets;
		ServerPredictAbility(TickRequest);
		OnAbilityTick.Broadcast(TickEvent);
	}
	if (CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNumberOfTicks())
	{
		EndCast();
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
	TArray<FPredictedTick> WaitingParams;
	ParamsAwaitingTicks.GetKeys(WaitingParams);
	for (FPredictedTick const& WaitingParam : WaitingParams)
	{
		if ((WaitingParam.PredictionID == CastingState.PredictionID && WaitingParam.TickNumber < CastingState.ElapsedTicks - 1) ||
			(WaitingParam.PredictionID < CastingState.PredictionID && CastingState.ElapsedTicks >= 1))
		{
			ExpiredTicks.Add(WaitingParam);
		}
	}
	for (FPredictedTick const& ExpiredTick : ExpiredTicks)
	{
		ParamsAwaitingTicks.Remove(ExpiredTick);
	}
}

void UAbilityComponent::MulticastAbilityTick_Implementation(FAbilityEvent const& Event)
{
	if (IsValid(Event.Ability))
	{
		Event.Ability->SimulatedTick(Event.Tick, Event.Targets);
	}
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		//Auto proxy already fired this delegate for predicted tick.
		OnAbilityTick.Broadcast(Event);
	}
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

bool UAbilityComponent::TryQueueAbility(TSubclassOf<UCombatAbility> const AbilityClass)
{
	if (!IsLocallyControlled() || !IsValid(AbilityClass) || (!GlobalCooldownState.bGlobalCooldownActive && !CastingState.bIsCasting))
	{
		return false;
	}
	if (CastingState.bIsCasting && CastingState.CastEndTime == 0.0f)
	{
		return false;
	}
	if (AbilityClass->GetDefaultObject<UCombatAbility>()->HasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive && GlobalCooldownState.EndTime == 0.0f)
	{
		return false;
	}
	float const GlobalTimeRemaining = AbilityClass->GetDefaultObject<UCombatAbility>()->HasGlobalCooldown() && GlobalCooldownState.bGlobalCooldownActive ?
		GlobalCooldownState.EndTime - GameStateRef->GetServerWorldTimeSeconds() : 0.0f;
	float const CastTimeRemaining = CastingState.bIsCasting ? CastingState.CastEndTime - GameStateRef->GetServerWorldTimeSeconds() : 0.0f;
	if (GlobalTimeRemaining > AbilityQueWindow || CastTimeRemaining > AbilityQueWindow)
	{
		return false;
	}
	if (GlobalTimeRemaining == CastTimeRemaining)
	{
		QueueStatus = EQueueStatus::WaitForBoth;
	}
	else if (GlobalTimeRemaining > CastTimeRemaining)
	{
		QueueStatus = EQueueStatus::WaitForGlobal;
	}
	else
	{
		QueueStatus = EQueueStatus::WaitForCast;
	}
	QueuedAbility = AbilityClass;
	GetWorld()->GetTimerManager().SetTimer(QueueExpirationHandle, this, &UAbilityComponent::ExpireQueue, AbilityQueWindow);
	return true;
}

void UAbilityComponent::ExpireQueue()
{
	QueueStatus = EQueueStatus::Empty;
	QueuedAbility = nullptr;
	GetWorld()->GetTimerManager().ClearTimer(QueueExpirationHandle);
}

bool UAbilityComponent::UseAbilityFromPredictedMovement(FAbilityRequest const& Request)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Request.AbilityClass) || Request.PredictionID == 0)
	{
		return false;
	}
	//Check to see if the ability handler has processed this cast already.
	bool* PreviousResult = PredictedTickRecord.Find(FPredictedTick(Request.PredictionID, Request.Tick));
	if (PreviousResult)
	{
		return *PreviousResult;
	}
	//Process the cast as normal.
	ServerPredictAbility_Implementation(Request);
	//Recheck for the newly created cast record.
	PreviousResult = PredictedTickRecord.Find(FPredictedTick(Request.PredictionID, Request.Tick));
	if (!PreviousResult)
	{
		//Tick still didn't happen, server tick timer hasn't passed yet.
		return false;
	}
	return *PreviousResult;
}

#pragma endregion
#pragma region Cancelling

FCancelEvent UAbilityComponent::CancelCurrentCast()
{
	FCancelEvent Result;
	if (!IsLocallyControlled())
	{
		Result.FailReason = ECancelFailReason::NetRole;
		return Result;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		Result.FailReason = ECancelFailReason::NotCasting;
		return Result;
	}
	Result.PredictionID = GetOwnerRole() == ROLE_Authority ? 0 : GenerateNewPredictionID();
	Result.CancelledCastID = CastingState.PredictionID;
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GameStateRef->GetServerWorldTimeSeconds();
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.bSuccess = true;
	CastingState.PredictionID = GetOwnerRole() == ROLE_Authority ? CastingState.PredictionID : Result.PredictionID;
	CastingState.CurrentCast->PredictedCancel(Result.Targets, Result.PredictionID);
	if (GetOwnerRole() == ROLE_Authority)
	{
		CastingState.CurrentCast->ServerCancel(Result.Targets, Result.PredictionID);
		MulticastAbilityCancel(Result);
	}
	else
	{
		FCancelRequest Request;
		Request.CancelID = Result.PredictionID;
		Request.CancelledCastID = Result.CancelledCastID;
		Request.CancelTime = Result.CancelTime;
		Request.Targets = Result.Targets;
		ServerCancelAbility(Request);
		OnAbilityCancelled.Broadcast(Result);
	}
	EndCast();
	return Result;
}

void UAbilityComponent::ServerCancelAbility_Implementation(FCancelRequest const& Request)
{
	int32 const CastID = CastingState.PredictionID;
	if (Request.CancelID > CastingState.PredictionID)
	{
		CastingState.PredictionID = Request.CancelID;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast) || CastID != Request.CancelledCastID)
	{
		return;
	}
	FCancelEvent Result;
	Result.CancelledAbility = CastingState.CurrentCast;
	Result.CancelTime = GameStateRef->GetServerWorldTimeSeconds();
	Result.PredictionID = Request.CancelID;
	Result.CancelledCastStart = CastingState.CastStartTime;
	Result.CancelledCastEnd = CastingState.CastEndTime;
	Result.CancelledCastID = CastID;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.Targets = Request.Targets;
	CastingState.CurrentCast->ServerCancel(Result.Targets, Result.PredictionID);
	MulticastAbilityCancel(Result);
	EndCast();
}

void UAbilityComponent::MulticastAbilityCancel_Implementation(FCancelEvent const& Event)
{
	if (IsValid(Event.CancelledAbility))
	{
		Event.CancelledAbility->SimulatedCancel(Event.Targets);
	}
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		//Auto proxy already fired this delegate for predicted cancel.
		OnAbilityCancelled.Broadcast(Event);
	}
}

#pragma endregion
#pragma region Interrupting

FInterruptEvent UAbilityComponent::InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions)
{
	FInterruptEvent Result;
	if (GetOwnerRole() != ROLE_Authority)
	{
		Result.FailReason = EInterruptFailReason::NetRole;
		return Result;
	}
	if (!CastingState.bIsCasting || !IsValid(CastingState.CurrentCast))
	{
		Result.FailReason = EInterruptFailReason::NotCasting;
		return Result;
	}
	Result.InterruptAppliedBy = AppliedBy;
	Result.InterruptAppliedTo = GetOwner();
	Result.InterruptSource = InterruptSource;
	Result.InterruptedAbility = CastingState.CurrentCast;
	Result.InterruptedCastStart = CastingState.CastStartTime;
	Result.InterruptedCastEnd = CastingState.CastEndTime;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.InterruptTime = GameStateRef->GetServerWorldTimeSeconds();
	Result.CancelledCastID = CastingState.PredictionID;
	if (!bIgnoreRestrictions)
	{
		if (!CastingState.bInterruptible)
		{
			Result.FailReason = EInterruptFailReason::Restricted;
			return Result;
		}
		for (TTuple<UBuff*, FInterruptRestriction> const& Restriction : InterruptRestrictions)
		{
			if (Restriction.Value.IsBound() && Restriction.Value.Execute(Result))
			{
				Result.FailReason = EInterruptFailReason::Restricted;
				return Result;
			}
		}
	}
	Result.bSuccess = true;
	Result.InterruptedAbility->ServerInterrupt(Result);
	MulticastAbilityInterrupt(Result);
	if (!IsLocallyControlled())
	{
		ClientAbilityInterrupt(Result);
	}
	EndCast();
	return Result;
}

void UAbilityComponent::ClientAbilityInterrupt_Implementation(FInterruptEvent const& InterruptEvent)
{
	if (!CastingState.bIsCasting || CastingState.PredictionID > InterruptEvent.CancelledCastID || !IsValid(CastingState.CurrentCast) ||
		!IsValid(InterruptEvent.InterruptedAbility) || CastingState.CurrentCast != InterruptEvent.InterruptedAbility)
    {
    	//We have already moved on.
    	return;
    }
    InterruptEvent.InterruptedAbility->SimulatedInterrupt(InterruptEvent);
    OnAbilityInterrupted.Broadcast(InterruptEvent);
    EndCast();
}

void UAbilityComponent::MulticastAbilityInterrupt_Implementation(FInterruptEvent const& InterruptEvent)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy && IsValid(InterruptEvent.InterruptedAbility))
	{
		InterruptEvent.InterruptedAbility->SimulatedInterrupt(InterruptEvent);
	}
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		OnAbilityInterrupted.Broadcast(InterruptEvent);
	}
}

void UAbilityComponent::InterruptCastOnCrowdControl(FCrowdControlStatus const& PreviousStatus,
	FCrowdControlStatus const& NewStatus)
{
	if (CastingState.bIsCasting && IsValid(CastingState.CurrentCast) && NewStatus.bActive == true)
	{
		FGameplayTagContainer RestrictedCcs;
		CastingState.CurrentCast->GetRestrictedCrowdControls(RestrictedCcs);
		if (RestrictedCcs.HasTagExact(NewStatus.CrowdControlType))
		{
			InterruptCurrentCast(NewStatus.DominantBuffInstance->GetHandler()->GetOwner(), NewStatus.DominantBuffInstance, true);
		}
	}
}

void UAbilityComponent::InterruptCastOnDeath(AActor* Actor, ELifeStatus const PreviousStatus,
	ELifeStatus const NewStatus)
{
	if (CastingState.bIsCasting && IsValid(CastingState.CurrentCast) && NewStatus != ELifeStatus::Alive)
	{
		if (!CastingState.CurrentCast->IsCastableWhileDead())
		{
			InterruptCurrentCast(GetOwner(), nullptr, true);
		}
	}
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
	CastingState.bInterruptible = Ability->IsInterruptible();
	CastingState.ElapsedTicks = 0;
	if (GetOwnerRole() == ROLE_Authority)
	{
		float const CastLength = CalculateCastLength(Ability, true);
		CastingState.CastEndTime = CastingState.CastStartTime + CastLength;
		GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityComponent::TickCurrentCast,
			(CastLength / Ability->GetNumberOfTicks()), true);
	}
	else
	{
		CastingState.CastEndTime = 0.0f;
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
	if (IsLocallyControlled())
	{
		if (QueueStatus == EQueueStatus::WaitForBoth)
		{
			QueueStatus = EQueueStatus::WaitForGlobal;
		}
		else if (QueueStatus == EQueueStatus::WaitForCast)
		{
			bUsingAbilityFromQueue = true;
			UseAbility(QueuedAbility);
		}
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
		float const GlobalLength = CalculateGlobalCooldownLength(Ability, true);
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
		if (IsLocallyControlled())
		{
			if (QueueStatus == EQueueStatus::WaitForBoth)
			{
				QueueStatus = EQueueStatus::WaitForCast;
			}
			else if (QueueStatus == EQueueStatus::WaitForGlobal)
			{
				bUsingAbilityFromQueue = true;
				UseAbility(QueuedAbility);
			}
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

void UAbilityComponent::SubscribeToAbilityTicked(FAbilityCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityTick.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromAbilityTicked(FAbilityCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityTick.Remove(Callback);
	}
}

void UAbilityComponent::SubscribeToAbilityCancelled(FAbilityCancelCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityCancelled.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromAbilityCancelled(FAbilityCancelCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityCancelled.Remove(Callback);
	}
}

void UAbilityComponent::SubscribeToAbilityInterrupted(FInterruptCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityInterrupted.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromAbilityInterrupted(FInterruptCallback const& Callback)
{
	if (Callback.IsBound())
	{
		OnAbilityInterrupted.Remove(Callback);
	}
}

void UAbilityComponent::SubscribeToAbilityMispredicted(FAbilityMispredictionCallback const& Callback)
{
	if (GetOwnerRole() == ROLE_AutonomousProxy && Callback.IsBound())
	{
		OnAbilityMispredicted.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromAbilityMispredicted(FAbilityMispredictionCallback const& Callback)
{
	if (GetOwnerRole() == ROLE_AutonomousProxy && Callback.IsBound())
	{
		OnAbilityMispredicted.Remove(Callback);
	}
}

void UAbilityComponent::SubscribeToGlobalCooldownChanged(FGlobalCooldownCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_SimulatedProxy && Callback.IsBound())
	{
		OnGlobalCooldownChanged.AddUnique(Callback);
	}
}

void UAbilityComponent::UnsubscribeFromGlobalCooldownChanged(FGlobalCooldownCallback const& Callback)
{
	if (GetOwnerRole() != ROLE_SimulatedProxy && Callback.IsBound())
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

void UAbilityComponent::AddGlobalCooldownModifier(UBuff* Source, FAbilityModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		GlobalCooldownMods.Add(Source, Modifier);
	}
}

void UAbilityComponent::RemoveGlobalCooldownModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		GlobalCooldownMods.Remove(Source);
	}
}

float UAbilityComponent::CalculateGlobalCooldownLength(UCombatAbility* Ability, bool const bWithPingComp) const
{
	if (!IsValid(Ability) || !Ability->HasGlobalCooldown())
	{
		return -1.0f;
	}
	if (Ability->HasStaticGlobalCooldownLength())
	{
		return Ability->GetDefaultGlobalCooldownLength();
	}
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FAbilityModCondition> const& Mod : GlobalCooldownMods)
	{
		if (Mod.Value.IsBound())
		{
			Mods.Add(Mod.Value.Execute(Ability));
		}
	}
	if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(GlobalCooldownLengthStatTag()))
	{
		Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(GlobalCooldownLengthStatTag()), EModifierType::Multiplicative));
	}
	float const PingCompensation = bWithPingComp ? FMath::Min(MaxPingCompensation, USaiyoraCombatLibrary::GetActorPing(GetOwner())) : 0.0f;
	return FMath::Max(MinimumGlobalCooldownLength, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultGlobalCooldownLength()) - PingCompensation);
}

void UAbilityComponent::AddCastLengthModifier(UBuff* Source, FAbilityModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		CastLengthMods.Add(Source, Modifier);
	}
}

void UAbilityComponent::RemoveCastLengthModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		CastLengthMods.Remove(Source);
	}
}

float UAbilityComponent::CalculateCastLength(UCombatAbility* Ability, bool const bWithPingComp) const
{
	if (!IsValid(Ability) || Ability->GetCastType() != EAbilityCastType::Channel)
	{
		return -1.0f;
	}
	if (Ability->HasStaticCastLength())
	{
		return Ability->GetDefaultCastLength();
	}
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FAbilityModCondition> const& Mod : CastLengthMods)
	{
		if (Mod.Value.IsBound())
		{
			Mods.Add(Mod.Value.Execute(Ability));
		}
	}
	if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(CastLengthStatTag()))
	{
		Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(CastLengthStatTag()), EModifierType::Multiplicative));
	}
	float const PingCompensation = bWithPingComp ? FMath::Min(MaxPingCompensation, USaiyoraCombatLibrary::GetActorPing(GetOwner())) : 0.0f;
	return FMath::Max(MinimumCastLength, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultCastLength()) - PingCompensation);
}

void UAbilityComponent::AddCooldownModifier(UBuff* Source, FAbilityModCondition const& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Modifier.IsBound())
	{
		CooldownMods.Add(Source, Modifier);
	}
}

void UAbilityComponent::RemoveCooldownModifier(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		CooldownMods.Remove(Source);
	}
}

float UAbilityComponent::CalculateCooldownLength(UCombatAbility* Ability, bool const bWithPingComp) const
{
	if (!IsValid(Ability))
	{
		return -1.0f;
	}
	if (Ability->HasStaticCooldownLength())
	{
		return Ability->GetDefaultCooldownLength();
	}
	TArray<FCombatModifier> Mods;
	for (TTuple<UBuff*, FAbilityModCondition> const& Mod : CooldownMods)
	{
		if (Mod.Value.IsBound())
		{
			Mods.Add(Mod.Value.Execute(Ability));
		}
	}
	if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(CooldownLengthStatTag()))
	{
		Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(CooldownLengthStatTag()), EModifierType::Multiplicative));
	}
	float const PingCompensation = bWithPingComp ? FMath::Min(MaxPingCompensation, USaiyoraCombatLibrary::GetActorPing(GetOwner())) : 0.0f;
	return FMath::Max(MinimumCooldownLength, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultCooldownLength()) - PingCompensation);
}

void UAbilityComponent::AddGenericResourceCostModifier(TSubclassOf<UResource> const ResourceClass, FCombatModifier const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !IsValid(Modifier.Source))
	{
		return;
	}
	for (TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*> const& AbilityTuple : ActiveAbilities)
	{
		if (IsValid(AbilityTuple.Value))
		{
			TArray<FDefaultAbilityCost> Costs;
			AbilityTuple.Value->GetDefaultAbilityCosts(Costs);
			for (FDefaultAbilityCost const& Cost : Costs)
			{
				if (Cost.ResourceClass == ResourceClass)
				{
					if (!Cost.bStaticCost)
					{
						AbilityTuple.Value->AddResourceCostModifier(ResourceClass, Modifier);
					}
					break;
				}
			}
		}
	}
}

void UAbilityComponent::RemoveGenericResourceCostModifier(TSubclassOf<UResource> const ResourceClass, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !IsValid(Source))
	{
		return;
	}
	for (TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*> const& AbilityTuple : ActiveAbilities)
	{
		if (IsValid(AbilityTuple.Value))
		{
			TArray<FDefaultAbilityCost> Costs;
			AbilityTuple.Value->GetDefaultAbilityCosts(Costs);
			for (FDefaultAbilityCost const& Cost : Costs)
			{
				if (Cost.ResourceClass == ResourceClass)
				{
					if (!Cost.bStaticCost)
					{
						AbilityTuple.Value->RemoveResourceCostModifier(ResourceClass, Source);
					}
					break;
				}
			}
		}
	}
}

#pragma endregion 
#pragma region Restrictions

void UAbilityComponent::AddAbilityTagRestriction(UBuff* Source, FGameplayTag const Tag)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !Tag.IsValid() || Tag.MatchesTagExact(AbilityClassRestrictionTag()))
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
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !Tag.IsValid() || Tag.MatchesTagExact(AbilityClassRestrictionTag()))
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
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Class))
	{
		return;
	}
	bool const bAlreadyRestricted = AbilityUsageClassRestrictions.Num(Class) > 0;
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
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Class))
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
	if (Ability->GetCastType() == EAbilityCastType::None)
	{
		OutFailReason = ECastFailReason::InvalidCastType;
		return false;
	}
	if (!Ability->IsCastableWhileDead() && IsValid(DamageHandlerRef) && DamageHandlerRef->GetLifeStatus() != ELifeStatus::Alive)
	{
		OutFailReason = ECastFailReason::Dead;
		return false;
	}
	OutFailReason = Ability->IsCastable();
	if (OutFailReason != ECastFailReason::None)
	{
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
	return true;
}

void UAbilityComponent::AddInterruptRestriction(UBuff* Source, FInterruptRestriction const& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		InterruptRestrictions.Add(Source, Restriction);
	}
}

void UAbilityComponent::RemoveInterruptRestriction(UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		InterruptRestrictions.Remove(Source);
	}
}

#pragma endregion