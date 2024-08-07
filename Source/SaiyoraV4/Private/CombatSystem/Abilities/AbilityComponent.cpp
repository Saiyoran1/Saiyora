﻿#include "AbilityComponent.h"
#include "AbilityStructs.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatAbility.h"
#include "CombatDebugOptions.h"
#include "CombatStatusComponent.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraGameInstance.h"
#include "SaiyoraGameState.h"
#include "SaiyoraMovementComponent.h"
#include "UnrealNetwork.h"

#pragma region Setup

UAbilityComponent::UAbilityComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//Since this is now a GameplayTasksComponent, we do need to tick.
	//PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
	bReplicateUsingRegisteredSubObjectList = true;
}

bool UAbilityComponent::IsLocallyControlled() const
{
	const APawn* OwnerAsPawn = Cast<APawn>(GetOwner());
	return IsValid(OwnerAsPawn) && OwnerAsPawn->IsLocallyControlled();
}

void UAbilityComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Ability Handler."));
	ResourceHandlerRef = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwner());
	StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	CrowdControlHandlerRef = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwner());
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	MovementComponentRef = ISaiyoraCombatInterface::Execute_GetCustomMovementComponent(GetOwner());
	CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
	const USaiyoraGameInstance* GameInstance = Cast<USaiyoraGameInstance>(GetWorld()->GetGameInstance());
	if (IsValid(GameInstance))
	{
		CombatDebugOptions = GameInstance->CombatDebugOptions;
	}
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	GameStateRef = GetWorld()->GetGameState<ASaiyoraGameState>();
	checkf(IsValid(GameStateRef), TEXT("Got an invalid Game State Ref in Ability Handler."));
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(CrowdControlHandlerRef))
		{
			CrowdControlHandlerRef->OnCrowdControlChanged.AddDynamic(this, &UAbilityComponent::InterruptCastOnCrowdControl);
		}
		if (IsValid(DamageHandlerRef))
		{
			DamageHandlerRef->OnLifeStatusChanged.AddDynamic(this, &UAbilityComponent::InterruptCastOnDeath);
		}
		if (IsValid(MovementComponentRef))
		{
			MovementComponentRef->OnMovementChanged.AddDynamic(this, &UAbilityComponent::InterruptCastOnMovement);
		}
		if (IsValid(CombatStatusComponentRef))
		{
			CombatStatusComponentRef->OnPlaneSwapped.AddDynamic(this, &UAbilityComponent::InterruptCastOnPlaneSwap);
		}
		for (const TSubclassOf<UCombatAbility> AbilityClass : DefaultAbilities)
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

UCombatAbility* UAbilityComponent::AddNewAbility(const TSubclassOf<UCombatAbility> AbilityClass)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AbilityClass) || ActiveAbilities.Contains(AbilityClass))
	{
		return nullptr;
	}
	UCombatAbility* NewAbility = NewObject<UCombatAbility>(GetOwner(), AbilityClass);
	if (IsValid(NewAbility))
	{
		ActiveAbilities.Add(AbilityClass, NewAbility);
		AddReplicatedSubObject(NewAbility);
		NewAbility->InitializeAbility(this);
		if (AbilityUsageClassRestrictions.Contains(AbilityClass))
		{
			NewAbility->AddRestrictedTag(FSaiyoraCombatTags::Get().AbilityClassRestriction);
		}
		TArray<FGameplayTag> RestrictedTags;
		AbilityUsageTagRestrictions.GenerateKeyArray(RestrictedTags);
		for (const FGameplayTag Tag : RestrictedTags)
		{
			if (NewAbility->HasTag(Tag))
			{
				NewAbility->AddRestrictedTag(Tag);
			}
		}
		ApplyGenericCostModsToNewAbility(NewAbility);
		OnAbilityAdded.Broadcast(NewAbility);
	}
	return NewAbility;
}

void UAbilityComponent::NotifyOfReplicatedAbility(UCombatAbility* NewAbility)
{
	if (IsValid(NewAbility) && !ActiveAbilities.Contains(NewAbility->GetClass()))
	{
		ActiveAbilities.Add(NewAbility->GetClass(), NewAbility);
		OnAbilityAdded.Broadcast(NewAbility);
	}
}

bool UAbilityComponent::RemoveAbility(const TSubclassOf<UCombatAbility> AbilityClass)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(AbilityClass))
	{
		return false;
	}
	UCombatAbility* AbilityToRemove = ActiveAbilities.FindRef(AbilityClass);
	if (!IsValid(AbilityToRemove))
	{
		return false;
	}
	AbilityToRemove->DeactivateAbility();
	RecentlyRemovedAbilities.Add(AbilityToRemove);
	ActiveAbilities.Remove(AbilityClass);
	OnAbilityRemoved.Broadcast(AbilityToRemove);
	FTimerHandle RemovalHandle;
	const FTimerDelegate RemovalDelegate = FTimerDelegate::CreateUObject(this, &UAbilityComponent::CleanupRemovedAbility, AbilityToRemove);
	GetWorld()->GetTimerManager().SetTimer(RemovalHandle, RemovalDelegate, 1.0f, false);
	return true;
}

void UAbilityComponent::NotifyOfReplicatedAbilityRemoval(UCombatAbility* RemovedAbility)
{
	if (IsValid(RemovedAbility))
	{
		const UCombatAbility* ExistingAbility = ActiveAbilities.FindRef(RemovedAbility->GetClass());
		if (IsValid(ExistingAbility) && ExistingAbility == RemovedAbility)
		{
			RemovedAbility->DeactivateAbility();
			ActiveAbilities.Remove(RemovedAbility->GetClass());
			OnAbilityRemoved.Broadcast(RemovedAbility);
		}
	}
}

void UAbilityComponent::CleanupRemovedAbility(UCombatAbility* Ability)
{
	RemoveReplicatedSubObject(Ability);
	RecentlyRemovedAbilities.Remove(Ability);
}


#pragma endregion
#pragma region Ability Usage

FAbilityEvent UAbilityComponent::UseAbility(const TSubclassOf<UCombatAbility> AbilityClass)
{
	FAbilityEvent Result;
	const bool bLogAbilityEvent = IsValid(CombatDebugOptions) && CombatDebugOptions->bLogAbilityEvents;
	if (!IsLocallyControlled())
	{
		Result.FailReasons.AddUnique(ECastFailReason::NetRole);
		if (bLogAbilityEvent)
		{
			CombatDebugOptions->LogAbilityEvent(GetOwner(), Result);
		}
		return Result;
	}
	if (!IsValid(AbilityClass))
	{
		Result.FailReasons.AddUnique(ECastFailReason::InvalidAbility);
		if (bLogAbilityEvent)
		{
			CombatDebugOptions->LogAbilityEvent(GetOwner(), Result);
		}
		return Result;
	}
	Result.Ability = ActiveAbilities.FindRef(AbilityClass);
	if (!IsValid(Result.Ability))
	{
		Result.FailReasons.AddUnique(ECastFailReason::InvalidAbility);
		if (bLogAbilityEvent)
		{
			CombatDebugOptions->LogAbilityEvent(GetOwner(), Result);
		}
		return Result;
	}
	if (!Result.Ability->IsCastable(Result.FailReasons))
	{
		if (bLogAbilityEvent)
		{
			CombatDebugOptions->LogAbilityEvent(GetOwner(), Result);
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
				Result.Ability->PredictedTick(0, Result.Origin, Result.Targets);
				Result.Ability->ServerTick(0, Result.Origin, Result.Targets);
				MulticastAbilityTick(Result);
			}
			break;
		case EAbilityCastType::Channel :
			{
				Result.ActionTaken = ECastAction::Success;
				StartCast(Result.Ability);
				if (Result.Ability->HasInitialTick())
				{
					Result.Ability->PredictedTick(0, Result.Origin, Result.Targets);
				}
				else
				{
					Result.Ability->PredictedNoTickCastStart();
				}
				//StartCast will only call the PredictedCastStart event on the ability, so we need to call ServerCastStart here.
				//This is to avoid ordering issues where ServerCastStart happens before PredictedTick for listen servers.
				if (Result.Ability->HasInitialTick())
				{
					Result.Ability->ServerTick(0, Result.Origin, Result.Targets);
					MulticastAbilityTick(Result);
				}
				//If the ability is instant or has an initial tick, ServerCastStart and SimulatedCastStart will fire from those events.
				//If the ability is channeled and has no initial tick, we need to manually call ServerCastStart and RPC to trigger SimulatedCastStart.
				else
				{
					Result.Ability->ServerNoTickCastStart();
					MulticastNoTickCastStart(Result.Ability);
				}
			}
			break;
		default :
			if (bLogAbilityEvent)
			{
				CombatDebugOptions->LogAbilityEvent(GetOwner(), Result);
			}
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
					Result.Ability->PredictedTick(0, Result.Origin, Result.Targets, Result.PredictionID);
					Request.Targets = Result.Targets;
					Request.Origin = Result.Origin;
					OnAbilityTick.Broadcast(Result);
					break;
				}
			case EAbilityCastType::Channel :
				{
					Result.ActionTaken = ECastAction::Success;
					StartCast(Result.Ability, Result.PredictionID);
                    if (Result.Ability->HasInitialTick())
                    {
                    	Result.Ability->PredictedTick(0, Result.Origin, Result.Targets, Result.PredictionID);
                    	Request.Targets = Result.Targets;
                    	Request.Origin = Result.Origin;
                    	OnAbilityTick.Broadcast(Result);
                    }
					//If the ability is instant or has an initial tick, PredictedCastStart will fire from those events.
					//If the ability is channeled and has no initial tick, we need to manually call PredictedCastStart.
                    else
                    {
	                    Result.Ability->PredictedNoTickCastStart(Result.PredictionID);
                    }
                    break;
				}
			default :
				if (bLogAbilityEvent)
				{
					CombatDebugOptions->LogAbilityEvent(GetOwner(), Result);
				}
				return Result;
		}
		FClientAbilityPrediction Prediction;
		Prediction.Ability = Result.Ability;
		Prediction.bPredictedGCD = Result.Ability->HasGlobalCooldown();
		Prediction.GcdLength = CalculateGlobalCooldownLength(Result.Ability);
		Prediction.Time = GameStateRef->GetServerWorldTimeSeconds();
		UnackedAbilityPredictions.Add(Result.PredictionID, Prediction);
		ServerPredictAbility(Request);
	}
	if (bLogAbilityEvent)
	{
		CombatDebugOptions->LogAbilityEvent(GetOwner(), Result);
	}
	return Result;
}

void UAbilityComponent::ServerPredictAbility_Implementation(const FAbilityRequest& Request)
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
			ServerResult.FailReasons.AddUnique(ECastFailReason::NetRole);
			PredictedTickRecord.Add(FPredictedTick(Request.PredictionID, 0), false);
			ClientPredictionResult(ServerResult);
			return;
		}
		LastPredictionID = Request.PredictionID;
		if (!IsValid(Request.AbilityClass))
		{
			ServerResult.FailReasons.AddUnique(ECastFailReason::InvalidAbility);
			PredictedTickRecord.Add(FPredictedTick(Request.PredictionID, 0), false);
			ClientPredictionResult(ServerResult);
			return;
		}
		UCombatAbility* Ability = ActiveAbilities.FindRef(Request.AbilityClass);
		const bool bValidAbility = IsValid(Ability);
		if (!bValidAbility)
		{
			ServerResult.FailReasons.AddUnique(ECastFailReason::InvalidAbility);
		}
		if (!bValidAbility || !Ability->IsCastable(ServerResult.FailReasons))
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
		Result.Origin = Request.Origin;
        
        if (Ability->HasGlobalCooldown())
        {
        	ServerResult.GlobalLength = StartGlobalCooldown(Ability, Request.PredictionID);
        	ServerResult.bActivatedGlobal = true;
        }
        
        const int32 PreviousCharges = Ability->GetCurrentCharges();
        Ability->CommitCharges(Request.PredictionID);
        const int32 NewCharges = Ability->GetCurrentCharges();
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
        		Ability->ServerTick(0, Result.Origin, Result.Targets, Result.PredictionID);
        		MulticastAbilityTick(Result);
	        }
        	break;
        case EAbilityCastType::Channel :
	        {
        		Result.ActionTaken = ECastAction::Success;
        		ServerResult.CastLength = StartCast(Ability, Result.PredictionID);
        		ServerResult.bActivatedCastBar = true;
        		ServerResult.bInterruptible = CastingState.bInterruptible;
        		if (Ability->HasInitialTick())
        		{
        			Ability->ServerTick(0, Result.Origin, Result.Targets, Result.PredictionID);
        			MulticastAbilityTick(Result);
        		}
        		//If the ability is instant or has an initial tick, ServerCastStart and SimulatedCastStart will fire from those events.
        		//If the ability is channeled and has no initial tick, we need to manually call ServerCastStart and RPC to trigger SimulatedCastStart.
		        else
		        {
		        	Ability->ServerNoTickCastStart(Result.PredictionID);
			        MulticastNoTickCastStart(Result.Ability);
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
			ParamsAwaitingTicks.Add(FPredictedTick(Request.PredictionID, Request.Tick), FAbilityParams(Request.Origin, Request.Targets));
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
				TicksAwaitingParams[i].Ability->ServerTick(TicksAwaitingParams[i].Tick, TicksAwaitingParams[i].Origin, TicksAwaitingParams[i].Targets, TicksAwaitingParams[i].PredictionID);
				MulticastAbilityTick(TicksAwaitingParams[i]);
				OnAbilityTick.Broadcast(TicksAwaitingParams[i]);
				PredictedTickRecord.Add(FPredictedTick(TicksAwaitingParams[i].PredictionID, TicksAwaitingParams[i].Tick), true);
				TicksAwaitingParams.RemoveAt(i);
				return;
			}
		}
	}
}

void UAbilityComponent::ClientPredictionResult_Implementation(const FServerAbilityResult& Result)
{
	const FClientAbilityPrediction* OriginalPrediction = UnackedAbilityPredictions.Find(Result.PredictionID);
	if (OriginalPrediction == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Client did not have an ability prediction stored that matches server result with prediction ID %i."), Result.PredictionID);
		return;
	}
	ClearOldPredictions(Result.PredictionID);
	if (IsValid(ResourceHandlerRef))
	{
		ResourceHandlerRef->UpdatePredictedCostsFromServer(Result);
	}
	if (IsValid(OriginalPrediction->Ability))
	{
		OriginalPrediction->Ability->UpdatePredictionFromServer(Result);
	}
	UpdateCastFromServerResult(OriginalPrediction->Time, Result);
	UpdateGlobalCooldownFromServerResult(OriginalPrediction->Time, Result);
	if (!Result.bSuccess)
	{
		OnAbilityMispredicted.Broadcast(Result.PredictionID);
	}
}

void UAbilityComponent::ClearOldPredictions(const int32 AckedPredictionID)
{
	//TODO: Figure out how to either reverse iterate over a map or use an iterator to allow removal during iteration.
	TArray<int32> OldIDs;
	for (const TTuple<int32, FClientAbilityPrediction>& Prediction : UnackedAbilityPredictions)
	{
		if (Prediction.Key <= AckedPredictionID)
		{
			OldIDs.Add(Prediction.Key);
		}
	}
	for (const int32 ID : OldIDs)
	{
		UnackedAbilityPredictions.Remove(ID);
	}
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
	TickEvent.ActionTaken = CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNonInitialTicks() ? ECastAction::Complete : ECastAction::Tick;
	if (GetOwnerRole() == ROLE_Authority && IsLocallyControlled())
	{
		CastingState.CurrentCast->PredictedTick(CastingState.ElapsedTicks, TickEvent.Origin, TickEvent.Targets);
		CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, TickEvent.Origin, TickEvent.Targets);
		MulticastAbilityTick(TickEvent);
	}
	else if (GetOwnerRole() == ROLE_Authority)
	{
		RemoveExpiredTicks();
		const FPredictedTick CurrentTick = FPredictedTick(CastingState.PredictionID, CastingState.ElapsedTicks);
		if (ParamsAwaitingTicks.Contains(CurrentTick))
		{
			const FAbilityParams* Params = ParamsAwaitingTicks.Find(CurrentTick);
			TickEvent.Origin = Params->Origin;
			TickEvent.Targets = Params->Targets;
			CastingState.CurrentCast->ServerTick(CastingState.ElapsedTicks, TickEvent.Origin, TickEvent.Targets, CastingState.PredictionID);
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
		CastingState.CurrentCast->PredictedTick(CastingState.ElapsedTicks, TickEvent.Origin, TickEvent.Targets, CastingState.PredictionID);
		TickRequest.Targets = TickEvent.Targets;
		TickRequest.Origin = TickEvent.Origin;
		ServerPredictAbility(TickRequest);
		OnAbilityTick.Broadcast(TickEvent);
	}
	//Check if we're still casting here because effects of a cast's tick could have cancelled it.
	if (CastingState.bIsCasting && CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNonInitialTicks())
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
	for (const FPredictedTick& WaitingParam : WaitingParams)
	{
		if ((WaitingParam.PredictionID == CastingState.PredictionID && WaitingParam.TickNumber < CastingState.ElapsedTicks - 1) ||
			(WaitingParam.PredictionID < CastingState.PredictionID && CastingState.ElapsedTicks >= 1))
		{
			ExpiredTicks.Add(WaitingParam);
		}
	}
	for (const FPredictedTick& ExpiredTick : ExpiredTicks)
	{
		ParamsAwaitingTicks.Remove(ExpiredTick);
	}
}

void UAbilityComponent::MulticastAbilityTick_Implementation(const FAbilityEvent& Event)
{
	if (IsValid(Event.Ability))
	{
		Event.Ability->SimulatedTick(Event.Tick, Event.Origin, Event.Targets);
	}
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		//Auto proxy already fired this delegate for predicted tick.
		OnAbilityTick.Broadcast(Event);
	}
}

void UAbilityComponent::MulticastNoTickCastStart_Implementation(UCombatAbility* Ability)
{
	if (IsValid(Ability))
	{
		Ability->SimulatedNoTickCastStart();
	}
}

int32 UAbilityComponent::GenerateNewPredictionID()
{
	const int32 Previous = LastPredictionID;
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

bool UAbilityComponent::UseAbilityFromPredictedMovement(const FAbilityRequest& Request)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Request.AbilityClass) || Request.PredictionID == 0)
	{
		return false;
	}
	//Check to see if the ability handler has processed this cast already.
	const bool* PreviousResult = PredictedTickRecord.Find(FPredictedTick(Request.PredictionID, Request.Tick));
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
	Result.CancelledCastEnd = CastingState.CastStartTime + CastingState.CastLength;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.bSuccess = true;
	CastingState.PredictionID = GetOwnerRole() == ROLE_Authority ? CastingState.PredictionID : Result.PredictionID;
	CastingState.CurrentCast->PredictedCancel(Result.Origin, Result.Targets, Result.PredictionID);
	if (GetOwnerRole() == ROLE_Authority)
	{
		CastingState.CurrentCast->ServerCancel(Result.Origin, Result.Targets, Result.PredictionID);
		MulticastAbilityCancel(Result);
	}
	else
	{
		FCancelRequest Request;
		Request.CancelID = Result.PredictionID;
		Request.CancelledCastID = Result.CancelledCastID;
		Request.CancelTime = Result.CancelTime;
		Request.Targets = Result.Targets;
		Request.Origin = Result.Origin;
		ServerCancelAbility(Request);
		OnAbilityCancelled.Broadcast(Result);
	}
	EndCast();
	return Result;
}

void UAbilityComponent::ServerCancelAbility_Implementation(const FCancelRequest& Request)
{
	const int32 CastID = CastingState.PredictionID;
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
	Result.CancelledCastEnd = CastingState.CastStartTime + CastingState.CastLength;
	Result.CancelledCastID = CastID;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.Targets = Request.Targets;
	CastingState.CurrentCast->ServerCancel(Result.Origin, Result.Targets, Result.PredictionID);
	MulticastAbilityCancel(Result);
	EndCast();
}

void UAbilityComponent::MulticastAbilityCancel_Implementation(const FCancelEvent& Event)
{
	if (IsValid(Event.CancelledAbility))
	{
		Event.CancelledAbility->SimulatedCancel(Event.Origin, Event.Targets);
	}
	if (GetOwnerRole() != ROLE_AutonomousProxy)
	{
		//Auto proxy already fired this delegate for predicted cancel.
		OnAbilityCancelled.Broadcast(Event);
	}
}

#pragma endregion
#pragma region Interrupting

FInterruptEvent UAbilityComponent::InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, const bool bIgnoreRestrictions)
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
	Result.InterruptedCastEnd = CastingState.CastStartTime + CastingState.CastLength;
	Result.ElapsedTicks = CastingState.ElapsedTicks;
	Result.InterruptTime = GameStateRef->GetServerWorldTimeSeconds();
	Result.CancelledCastID = CastingState.PredictionID;
	if (!bIgnoreRestrictions)
	{
		if (!CastingState.bInterruptible || InterruptRestrictions.IsRestricted(Result))
		{
			Result.FailReason = EInterruptFailReason::Restricted;
			return Result;
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

void UAbilityComponent::ClientAbilityInterrupt_Implementation(const FInterruptEvent& InterruptEvent)
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

void UAbilityComponent::MulticastAbilityInterrupt_Implementation(const FInterruptEvent& InterruptEvent)
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

void UAbilityComponent::InterruptCastOnCrowdControl(const FCrowdControlStatus& PreviousStatus,
	const FCrowdControlStatus& NewStatus)
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

void UAbilityComponent::InterruptCastOnDeath(AActor* Actor, const ELifeStatus PreviousStatus,
	const ELifeStatus NewStatus)
{
	if (CastingState.bIsCasting && IsValid(CastingState.CurrentCast) && NewStatus != ELifeStatus::Alive)
	{
		if (!CastingState.CurrentCast->IsCastableWhileDead())
		{
			InterruptCurrentCast(GetOwner(), nullptr, true);
		}
	}
}

void UAbilityComponent::InterruptCastOnMovement(AActor* Actor, const bool bNewMovement)
{
	if (CastingState.bIsCasting && IsValid(CastingState.CurrentCast) && bNewMovement)
	{
		if (!CastingState.CurrentCast->IsCastableWhileMoving())
		{
			InterruptCurrentCast(GetOwner(), nullptr, true);
		}
	}
}

void UAbilityComponent::InterruptCastOnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane,
	UObject* SwapSource)
{
	if (CastingState.bIsCasting && IsValid(CastingState.CurrentCast) && NewPlane != PreviousPlane)
	{
		InterruptCurrentCast(GetOwner(), SwapSource, true);
	}
}


#pragma endregion 
#pragma region Casting

float UAbilityComponent::StartCast(UCombatAbility* Ability, const int32 PredictionID)
{
	const FCastingState PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.CurrentCast = Ability;
	if (PredictionID != 0)
	{
		CastingState.PredictionID = PredictionID;
	}
	CastingState.CastStartTime = GameStateRef->GetServerWorldTimeSeconds();
	CastingState.bInterruptible = Ability->IsInterruptible();
	CastingState.ElapsedTicks = 0;
	const float CastLength = CalculateCastLength(Ability);
	if (GetOwnerRole() == ROLE_Authority && !IsLocallyControlled())
	{
		//Reduce cast length by a fraction of actor's ping to prevent queued abilities from failing due to arriving too fast.
		CastingState.CastLength = CastLength - (LagCompensationRatio * FMath::Min(MaxLagCompensation, USaiyoraCombatLibrary::GetActorPing(GetOwner())));
	}
	else
	{
		CastingState.CastLength = CastLength;
	}
	GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityComponent::TickCurrentCast,
	(CastingState.CastLength / Ability->GetNonInitialTicks()), true);
    OnCastStateChanged.Broadcast(PreviousState, CastingState);
	return CastLength;
}

void UAbilityComponent::EndCast()
{
	const FCastingState PreviousState = CastingState;
    CastingState.bIsCasting = false;
    CastingState.CurrentCast = nullptr;
   	CastingState.bInterruptible = false;
    CastingState.ElapsedTicks = 0;
    CastingState.CastStartTime = 0.0f;
    CastingState.CastLength = 0.0f;
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
    OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UAbilityComponent::UpdateCastFromServerResult(const float PredictionTime, const FServerAbilityResult& Result)
{
	if (CastingState.PredictionID <= Result.PredictionID)
	{
		if (Result.AbilityClass->GetDefaultObject<UCombatAbility>()->CanCastWhileCasting())
		{
			return;
		}
		const FCastingState PreviousState = CastingState;
		if (Result.bSuccess && Result.bActivatedCastBar && PredictionTime + Result.CastLength > GameStateRef->GetServerWorldTimeSeconds())
		{
			CastingState.PredictionID = Result.PredictionID;
			CastingState.bIsCasting = true;
			CastingState.CastStartTime = PredictionTime;
			CastingState.CastLength = Result.CastLength;
			CastingState.CurrentCast = ActiveAbilities.FindRef(Result.AbilityClass);
			CastingState.bInterruptible = Result.bInterruptible;
			const bool bPredictedInitial = PreviousState.PredictionID == Result.PredictionID && PreviousState.bIsCasting;
			const int32 PredictedElapsed = PreviousState.PredictionID == Result.PredictionID && PreviousState.bIsCasting ? PreviousState.ElapsedTicks : 0;
			const float TickInterval = CastingState.CastLength / CastingState.CurrentCast->GetNonInitialTicks();
			const int32 ActualElapsed = FMath::FloorToInt((GameStateRef->GetServerWorldTimeSeconds() - CastingState.CastStartTime) / TickInterval);
			if (PredictedElapsed < ActualElapsed)
			{
				//Need to immediately predict the missed ticks, including an initial tick if we hadn't already.
				for (int32 i = PredictedElapsed; i < ActualElapsed; i++)
				{
					if (i == 0 && !bPredictedInitial && CastingState.CurrentCast->HasInitialTick())
					{
						FAbilityEvent TickEvent;
						TickEvent.Ability = CastingState.CurrentCast;
						TickEvent.PredictionID = CastingState.PredictionID;
						TickEvent.Tick = 0;
						TickEvent.ActionTaken = ECastAction::Success;
						FAbilityRequest TickRequest;
						TickRequest.PredictionID = CastingState.PredictionID;
						TickRequest.Tick = CastingState.ElapsedTicks;
						TickRequest.AbilityClass = CastingState.CurrentCast->GetClass();
						CastingState.CurrentCast->PredictedTick(CastingState.ElapsedTicks, TickEvent.Origin, TickEvent.Targets, CastingState.PredictionID);
						TickRequest.Targets = TickEvent.Targets;
						TickRequest.Origin = TickEvent.Origin;
						ServerPredictAbility(TickRequest);
						OnAbilityTick.Broadcast(TickEvent);
					}
					TickCurrentCast();
				}
			}
			GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityComponent::TickCurrentCast, TickInterval, true,
				(CastingState.CastStartTime + (TickInterval * (CastingState.ElapsedTicks + 1))) - GameStateRef->GetServerWorldTimeSeconds());
			OnCastStateChanged.Broadcast(PreviousState, CastingState);
		}
		else if (CastingState.bIsCasting)
		{
			//If we're ending a cast due to misprediction, we need to fire off the PredictedCastEnd event for the ability that is normally handled by cancelling, interrupting, or the final tick of the channel.
			if (IsValid(CastingState.CurrentCast))
			{
				CastingState.CurrentCast->MispredictedCastEnd(Result.PredictionID);
			}
			//TODO: Potential to miss ticks here if the cast should've already completed.
			EndCast();
		}
	}
}

#pragma endregion 
#pragma region Global Cooldown

float UAbilityComponent::StartGlobalCooldown(UCombatAbility* Ability, const int32 PredictionID)
{
	const FGlobalCooldown PreviousState = GlobalCooldownState;
	GlobalCooldownState.bActive = true;
	if (PredictionID != 0)
	{
		GlobalCooldownState.PredictionID = PredictionID;
	}
	GlobalCooldownState.StartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	const float GlobalCooldownLength = CalculateGlobalCooldownLength(Ability);
	if (GetOwnerRole() == ROLE_Authority && !IsLocallyControlled())
	{
		//Reduce GCD by a percentage of ping to prevent queued abilities from arriving too fast and failing to cast.
		//This only happens on the server for non-local players.
		GlobalCooldownState.Length = GlobalCooldownLength - (LagCompensationRatio * FMath::Min(MaxLagCompensation, USaiyoraCombatLibrary::GetActorPing(GetOwner())));
	}
	else
	{
		GlobalCooldownState.Length = GlobalCooldownLength;
	}
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UAbilityComponent::EndGlobalCooldown, GlobalCooldownState.Length, false);
	OnGlobalCooldownChanged.Broadcast(PreviousState, GlobalCooldownState);
	return GlobalCooldownLength;
}

void UAbilityComponent::EndGlobalCooldown()
{
	if (GlobalCooldownState.bActive)
	{
		const FGlobalCooldown PreviousGlobal;
		GlobalCooldownState.bActive = false;
		GlobalCooldownState.StartTime = 0.0f;
		GlobalCooldownState.Length = 0.0f;
		GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
		OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
	}
}

void UAbilityComponent::UpdateGlobalCooldownFromServerResult(const float PredictionTime, const FServerAbilityResult& Result)
{
	const FGlobalCooldown PreviousGlobal = GlobalCooldownState;
	//We will reset the GCD timer later if necessary, but clear it here anyway since it could be inaccurate.
	GetWorld()->GetTimerManager().ClearTimer(GlobalCooldownHandle);
	
	//Set the global state back to the server result, with the exception of Start Time, which we take from the client's saved prediction.
	GlobalCooldownState.bActive = Result.bActivatedGlobal;
	GlobalCooldownState.StartTime = PredictionTime;
	GlobalCooldownState.Length = Result.GlobalLength;
	GlobalCooldownState.PredictionID = Result.PredictionID;

	//Iterate over other predictions. Any prediction that is later than the result and could be relevant (happens AFTER the previous GCD has ended), is then applied.
	for (const TTuple<int32, FClientAbilityPrediction>& AbilityPrediction : UnackedAbilityPredictions)
	{
		if (AbilityPrediction.Key > GlobalCooldownState.PredictionID && AbilityPrediction.Value.Time > GlobalCooldownState.StartTime + GlobalCooldownState.Length)
		{
			GlobalCooldownState.bActive = AbilityPrediction.Value.bPredictedGCD;
			GlobalCooldownState.StartTime = AbilityPrediction.Value.Time;
			GlobalCooldownState.Length = AbilityPrediction.Value.GcdLength;
		}
	}

	if (GlobalCooldownState.bActive)
	{
		if (GlobalCooldownState.StartTime + GlobalCooldownState.Length <= GetGameStateRef()->GetServerWorldTimeSeconds())
		{
			//If, after updating from all predictions, we are left on a global, BUT the global should have already finished, then we end the global.
			GlobalCooldownState.bActive = false;
			GlobalCooldownState.Length = 0.0f;
		}
		else
		{
			//If instead, we are left on a global that is NOT yet finished, set a timer for global end to match the remaining time.
			const float RemainingTime = GlobalCooldownState.StartTime + GlobalCooldownState.Length - GetGameStateRef()->GetServerWorldTimeSeconds();
			GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UAbilityComponent::EndGlobalCooldown, RemainingTime, false);
		}
	}

	if (GlobalCooldownState != PreviousGlobal)
	{
		OnGlobalCooldownChanged.Broadcast(PreviousGlobal, GlobalCooldownState);
	}
}

#pragma endregion 
#pragma region Modifiers

float UAbilityComponent::CalculateGlobalCooldownLength(UCombatAbility* Ability) const
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
	GlobalCooldownMods.GetModifiers(Mods, Ability);
	if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_GlobalCooldownLength))
	{
		Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_GlobalCooldownLength), EModifierType::Multiplicative));
	}
	return FMath::Max(MinGcdLength, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultGlobalCooldownLength()));
}

float UAbilityComponent::CalculateCastLength(UCombatAbility* Ability) const
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
	CastLengthMods.GetModifiers(Mods, Ability);
	if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_CastLength))
	{
		Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_CastLength), EModifierType::Multiplicative));
	}
	return FMath::Max(MinCastLength, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultCastLength()));
}

float UAbilityComponent::CalculateCooldownLength(UCombatAbility* Ability, const bool bIgnoreGlobalMin) const
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
	CooldownMods.GetModifiers(Mods, Ability);
	if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_CooldownLength))
	{
		Mods.Add(FCombatModifier(StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_CooldownLength), EModifierType::Multiplicative));
	}
	return FMath::Max(bIgnoreGlobalMin ? 0.0f : MinCooldownLength, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultCooldownLength()));
}

FCombatModifierHandle UAbilityComponent::AddGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifier& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass))
	{
		return FCombatModifierHandle::Invalid;
	}
	FCombatModifierHandle Handle = FCombatModifierHandle::MakeHandle();
	FResourceCostModMap& ModMap = GenericResourceCostMods.Add(Handle, FResourceCostModMap(ResourceClass, Modifier));
	for (const TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*>& AbilityTuple : ActiveAbilities)
	{
		if (IsValid(AbilityTuple.Value))
		{
			TArray<FSimpleAbilityCost> Costs;
			AbilityTuple.Value->GetAbilityCosts(Costs);
			for (const FSimpleAbilityCost& Cost : Costs)
			{
				if (Cost.ResourceClass == ResourceClass)
				{
					ModMap.ModifierHandles.Add(AbilityTuple.Value, AbilityTuple.Value->AddResourceCostModifier(ResourceClass, Modifier));
					break;
				}
			}
		}
	}
	return Handle;
}

void UAbilityComponent::RemoveGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifierHandle& ModifierHandle)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !ModifierHandle.IsValid())
	{
		return;
	}
	if (FResourceCostModMap* ModMap = GenericResourceCostMods.Find(ModifierHandle))
	{
		for (const TTuple<UCombatAbility*, FCombatModifierHandle>& HandlePair : ModMap->ModifierHandles)
		{
			if (IsValid(HandlePair.Key))
			{
				HandlePair.Key->RemoveResourceCostModifier(ResourceClass, HandlePair.Value);
			}
		}
	}
	GenericResourceCostMods.Remove(ModifierHandle);
}

void UAbilityComponent::UpdateGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass,
	const FCombatModifierHandle& ModifierHandle, const FCombatModifier& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !ModifierHandle.IsValid())
	{
		return;
	}
	if (FResourceCostModMap* ModMap = GenericResourceCostMods.Find(ModifierHandle))
	{
		if (ResourceClass != ModMap->ResourceClass)
		{
			return;
		}
		ModMap->Modifier = Modifier;
		for (const TTuple<UCombatAbility*, FCombatModifierHandle>& HandlePair : ModMap->ModifierHandles)
		{
			if (IsValid(HandlePair.Key))
			{
				HandlePair.Key->UpdateResourceCostModifier(ResourceClass, HandlePair.Value, Modifier);
			}
		}
	}
}

void UAbilityComponent::ApplyGenericCostModsToNewAbility(UCombatAbility* NewAbility)
{
	TArray<FSimpleAbilityCost> AbilityCosts;
	NewAbility->GetAbilityCosts(AbilityCosts);
	for (TTuple<FCombatModifierHandle, FResourceCostModMap>& ModMapTuple : GenericResourceCostMods)
	{
		for (const FSimpleAbilityCost& Cost : AbilityCosts)
		{
			if (Cost.ResourceClass == ModMapTuple.Value.ResourceClass)
			{
				ModMapTuple.Value.ModifierHandles.Add(NewAbility, NewAbility->AddResourceCostModifier(Cost.ResourceClass, ModMapTuple.Value.Modifier));
				break;
			}
		}
	}
}

#pragma endregion 
#pragma region Restrictions

void UAbilityComponent::AddAbilityTagRestriction(UBuff* Source, const FGameplayTag Tag)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !Tag.IsValid() || Tag.MatchesTagExact(FSaiyoraCombatTags::Get().AbilityRestriction))
	{
		return;
	}
	const bool bAlreadyRestricted = AbilityUsageTagRestrictions.Num(Tag) > 0;
	AbilityUsageTagRestrictions.AddUnique(Tag, Source);
	if (!bAlreadyRestricted)
	{
		for (const TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*>& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Value) && AbilityPair.Value->HasTag(Tag))
			{
				AbilityPair.Value->AddRestrictedTag(Tag);
			}
		}
	}
}

void UAbilityComponent::RemoveAbilityTagRestriction(UBuff* Source, const FGameplayTag Tag)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !Tag.IsValid() || Tag.MatchesTagExact(FSaiyoraCombatTags::Get().AbilityRestriction))
	{
		return;
	}
	if (AbilityUsageTagRestrictions.Remove(Tag, Source) > 0 && AbilityUsageTagRestrictions.Num(Tag) == 0)
	{
		for (const TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*>& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Value) && AbilityPair.Value->HasTag(Tag))
			{
				AbilityPair.Value->RemoveRestrictedTag(Tag);
			}
		}
	}
}

void UAbilityComponent::AddAbilityClassRestriction(UBuff* Source, const TSubclassOf<UCombatAbility> Class)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Class))
	{
		return;
	}
	bool const bAlreadyRestricted = AbilityUsageClassRestrictions.Num(Class) > 0;
	AbilityUsageClassRestrictions.AddUnique(Class, Source);
	if (!bAlreadyRestricted)
	{
		for (const TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*>& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Key) && IsValid(AbilityPair.Value) && AbilityPair.Key == Class)
			{
				AbilityPair.Value->AddRestrictedTag(FSaiyoraCombatTags::Get().AbilityClassRestriction);
			}
		}
	}
}

void UAbilityComponent::RemoveAbilityClassRestriction(UBuff* Source, const TSubclassOf<UCombatAbility> Class)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(Class))
	{
		return;
	}
	if (AbilityUsageClassRestrictions.Remove(Class, Source) > 0 && AbilityUsageClassRestrictions.Num(Class) == 0)
	{
		for (const TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*>& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Key) && IsValid(AbilityPair.Value) && AbilityPair.Key == Class)
			{
				AbilityPair.Value->RemoveRestrictedTag(FSaiyoraCombatTags::Get().AbilityClassRestriction);
			}
		}
	}
}

#pragma endregion