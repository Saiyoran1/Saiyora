#include "AbilityComponent.h"
#include "AbilityStructs.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatAbility.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatLibrary.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#pragma region Setup

const float UAbilityComponent::PINGCOMPENSATIONRATIO = 0.2f;
const float UAbilityComponent::MINCASTLENGTH = 0.5f;
const float UAbilityComponent::MINGCDLENGTH = 0.05f;
const float UAbilityComponent::MINCDLENGTH = 0.05f;
const float UAbilityComponent::ABILITYQUEWINDOW = 0.2f;

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

bool UAbilityComponent::IsLocallyControlled() const
{
	const APawn* OwnerAsPawn = Cast<APawn>(GetOwner());
	return IsValid(OwnerAsPawn) && OwnerAsPawn->IsLocallyControlled();
}

void UAbilityComponent::InitializeComponent()
{
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Ability Handler."));
	ResourceHandlerRef = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwner());
	StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	CrowdControlHandlerRef = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwner());
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	
	GameStateRef = GetWorld()->GetGameState<AGameState>();
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

void UAbilityComponent::RemoveAbility(const TSubclassOf<UCombatAbility> AbilityClass)
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
	const FTimerDelegate RemovalDelegate = FTimerDelegate::CreateUObject(this, &UAbilityComponent::CleanupRemovedAbility, AbilityToRemove);
	GetWorld()->GetTimerManager().SetTimer(RemovalHandle, RemovalDelegate, 1.0f, false);
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

#pragma endregion
#pragma region Ability Usage

FAbilityEvent UAbilityComponent::UseAbility(const TSubclassOf<UCombatAbility> AbilityClass)
{
	FAbilityEvent Result;
	if (!IsLocallyControlled() || !IsValid(AbilityClass))
	{
		Result.FailReason = ECastFailReason::NetRole;
		return Result;
	}
	Result.Ability = ActiveAbilities.FindRef(AbilityClass);
	if (!CanUseAbility(Result.Ability, Result.FailReason))
	{
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
					Result.Ability->ServerTick(0, Result.Origin, Result.Targets);
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
					Result.Ability->PredictedTick(0, Result.Origin, Result.Targets);
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
                    	Result.Ability->PredictedTick(0, Result.Origin, Result.Targets);
                    	Request.Targets = Result.Targets;
                    	Request.Origin = Result.Origin;
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
		Prediction.GcdLength = CalculateGlobalCooldownLength(Result.Ability);
		Prediction.Time = GameStateRef->GetServerWorldTimeSeconds();
		UnackedAbilityPredictions.Add(Result.PredictionID, Prediction);
		ServerPredictAbility(Request);
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
		Result.Origin = Request.Origin;
        
        if (Ability->HasGlobalCooldown())
        {
        	StartGlobalCooldown(Ability, Request.PredictionID);
        	ServerResult.bActivatedGlobal = true;
        	ServerResult.GlobalLength = CalculateGlobalCooldownLength(Ability);
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
        		StartCast(Ability, Result.PredictionID);
        		ServerResult.bActivatedCastBar = true;
        		ServerResult.CastLength = CalculateCastLength(Ability);
        		ServerResult.bInterruptible = CastingState.bInterruptible;
        		if (Ability->HasInitialTick())
        		{
        			Ability->ServerTick(0, Result.Origin, Result.Targets, Result.PredictionID);
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
	TickEvent.ActionTaken = CastingState.ElapsedTicks >= CastingState.CurrentCast->GetNumberOfTicks() ? ECastAction::Complete : ECastAction::Tick;
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
		//TODO: I changed this to allow cancelling of unacked casts. If this breaks things, here is where to fix!
		//Note that we can not cancel a cast that is being predicted and has not had its cast time acked.
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
	Result.CancelledCastEnd = CastingState.CastEndTime;
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
		for (const TTuple<UBuff*, FInterruptRestriction>& Restriction : InterruptRestrictions)
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

#pragma endregion 
#pragma region Casting

void UAbilityComponent::StartCast(UCombatAbility* Ability, const int32 PredictionID)
{
	const FCastingState PreviousState = CastingState;
	CastingState.bIsCasting = true;
	CastingState.bAcked = GetOwnerRole() == ROLE_Authority;
	CastingState.CurrentCast = Ability;
	if (PredictionID != 0)
	{
		CastingState.PredictionID = PredictionID;
	}
	CastingState.CastStartTime = GameStateRef->GetServerWorldTimeSeconds();
	CastingState.bInterruptible = Ability->IsInterruptible();
	CastingState.ElapsedTicks = 0;
	float const CastLength = GetOwnerRole() == ROLE_Authority ? CalculateCastLength(Ability) : -1.0f;
	CastingState.CastEndTime = GetOwnerRole() == ROLE_Authority ? CastingState.CastStartTime + CastLength : -1.0f;
	if (GetOwnerRole() == ROLE_Authority)
	{
		GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UAbilityComponent::TickCurrentCast,
			(CastLength / Ability->GetNumberOfTicks()), true);
	}
    OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UAbilityComponent::EndCast()
{
	const FCastingState PreviousState = CastingState;
    CastingState.bIsCasting = false;
    CastingState.CurrentCast = nullptr;
   	CastingState.bInterruptible = false;
    CastingState.ElapsedTicks = 0;
    CastingState.CastStartTime = 0.0f;
    CastingState.CastEndTime = 0.0f;
    GetWorld()->GetTimerManager().ClearTimer(TickHandle);
    OnCastStateChanged.Broadcast(PreviousState, CastingState);
}

void UAbilityComponent::UpdateCastFromServerResult(const float PredictionTime, const FServerAbilityResult& Result)
{
	if (CastingState.PredictionID <= Result.PredictionID)
	{
		CastingState.PredictionID = Result.PredictionID;
		if (Result.bSuccess && Result.bActivatedCastBar && PredictionTime + Result.CastLength > GameStateRef->GetServerWorldTimeSeconds())
		{
			const FCastingState PreviousState = CastingState;
			CastingState.bIsCasting = true;
			CastingState.bAcked = true;
			CastingState.CastStartTime = PredictionTime;
			CastingState.CastEndTime = CastingState.CastStartTime + Result.CastLength;
			CastingState.CurrentCast = ActiveAbilities.FindRef(Result.AbilityClass);
			CastingState.bInterruptible = Result.bInterruptible;
			//Check for any missed ticks. We will update the last missed tick (if any, this should be rare) after setting everything else.
			const float TickInterval = Result.CastLength / CastingState.CurrentCast->GetNumberOfTicks();
			const int32 ElapsedTicks = FMath::FloorToInt((GameStateRef->GetServerWorldTimeSeconds() - CastingState.CastStartTime) / TickInterval);
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
			//TODO: Potential to miss ticks here if the cast should've already completed.
			EndCast();
		}
	}
}

#pragma endregion 
#pragma region Global Cooldown

void UAbilityComponent::StartGlobalCooldown(UCombatAbility* Ability, const int32 PredictionID)
{
	const FGlobalCooldown PreviousState = GlobalCooldownState;
	GlobalCooldownState.bActive = true;
	if (PredictionID != 0)
	{
		GlobalCooldownState.PredictionID = PredictionID;
	}
	GlobalCooldownState.StartTime = GetGameStateRef()->GetServerWorldTimeSeconds();
	GlobalCooldownState.Length = CalculateGlobalCooldownLength(Ability);
	if (GetOwnerRole() == ROLE_Authority && !IsLocallyControlled())
	{
		//Reduce GCD by a percentage of ping to prevent queued abilities from arriving too fast and failing to cast.
		//This only happens on the server for non-local players.
		GlobalCooldownState.Length = FMath::Max(0.0f, GlobalCooldownState.Length - (USaiyoraCombatLibrary::GetActorPing(GetOwner()) * PINGCOMPENSATIONRATIO));
	}
	GetWorld()->GetTimerManager().SetTimer(GlobalCooldownHandle, this, &UAbilityComponent::EndGlobalCooldown, GlobalCooldownState.Length, false);
	OnGlobalCooldownChanged.Broadcast(PreviousState, GlobalCooldownState);
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
	return FMath::Max(MINGCDLENGTH, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultGlobalCooldownLength()));
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
	return FMath::Max(MINCASTLENGTH, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultCastLength()));
}

float UAbilityComponent::CalculateCooldownLength(UCombatAbility* Ability) const
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
	return FMath::Max(MINCDLENGTH, FCombatModifier::ApplyModifiers(Mods, Ability->GetDefaultCooldownLength()));
}

void UAbilityComponent::AddGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, const FCombatModifier& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !IsValid(Modifier.Source))
	{
		return;
	}
	for (const TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*>& AbilityTuple : ActiveAbilities)
	{
		if (IsValid(AbilityTuple.Value))
		{
			TArray<FDefaultAbilityCost> Costs;
			AbilityTuple.Value->GetDefaultAbilityCosts(Costs);
			for (const FDefaultAbilityCost& Cost : Costs)
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

void UAbilityComponent::RemoveGenericResourceCostModifier(const TSubclassOf<UResource> ResourceClass, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(ResourceClass) || !IsValid(Source))
	{
		return;
	}
	for (const TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*>& AbilityTuple : ActiveAbilities)
	{
		if (IsValid(AbilityTuple.Value))
		{
			TArray<FDefaultAbilityCost> Costs;
			AbilityTuple.Value->GetDefaultAbilityCosts(Costs);
			for (const FDefaultAbilityCost& Cost : Costs)
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

bool UAbilityComponent::CanUseAbility(const UCombatAbility* Ability, ECastFailReason& OutFailReason) const
{
	OutFailReason = ECastFailReason::None;
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
	if (Ability->HasGlobalCooldown() && GlobalCooldownState.bActive)
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

void UAbilityComponent::AddInterruptRestriction(UBuff* Source, const FInterruptRestriction& Restriction)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source) && Restriction.IsBound())
	{
		InterruptRestrictions.Add(Source, Restriction);
	}
}

void UAbilityComponent::RemoveInterruptRestriction(const UBuff* Source)
{
	if (GetOwnerRole() == ROLE_Authority && IsValid(Source))
	{
		InterruptRestrictions.Remove(Source);
	}
}

#pragma endregion