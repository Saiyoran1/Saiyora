// Fill out your copyright notice in the Description page of Project Settings.


#include "ResourceHandler.h"
#include "Resource.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "CombatAbility.h"
#include "SaiyoraCombatInterface.h"

UResourceHandler::UResourceHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UResourceHandler::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(GetOwner()) && GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		StatHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
	}
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (TTuple<TSubclassOf<UResource>, FResourceInitInfo> const& InitInfo : DefaultResources)
		{
			AddNewResource(InitInfo.Key, InitInfo.Value);
		}
	}
}

void UResourceHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool UResourceHandler::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	
	bWroteSomething |= Channel->ReplicateSubobjectList(ActiveResources, *Bunch, *RepFlags);
	bWroteSomething |= Channel->ReplicateSubobjectList(RecentlyRemovedResources, *Bunch, *RepFlags);

	return bWroteSomething;
}

UResource* UResourceHandler::FindActiveResource(TSubclassOf<UResource> const ResourceClass) const
{
	if (!IsValid(ResourceClass))
	{
		return nullptr;
	}
	UResource* Found = nullptr;
	for (UResource* Resource : ActiveResources)
	{
		if (Resource->GetClass() == ResourceClass)
		{
			Found = Resource;
			break;
		}
	}
	if (!IsValid(Found) || !Found->GetInitialized() || Found->GetDeactivated())
	{
		return nullptr;
	}
	return Found;
}

void UResourceHandler::NotifyOfReplicatedResource(UResource* ReplicatedResource)
{
	if (!IsValid(ReplicatedResource))
	{
		return;
	}
	UResource* ExistingResource = FindActiveResource(ReplicatedResource->GetClass());
	if (IsValid(ExistingResource))
	{
		return;
	}
	ActiveResources.Add(ReplicatedResource);
	OnResourceAdded.Broadcast(ReplicatedResource);
}

void UResourceHandler::NotifyOfRemovedReplicatedResource(UResource* Resource)
{
	if (ActiveResources.Remove(Resource) != 0)
	{
		OnResourceRemoved.Broadcast(Resource);
	}
}

bool UResourceHandler::CheckAbilityCostsMet(UCombatAbility* Ability, TArray<FAbilityCost>& OutCosts) const
{
	for (FAbilityCost& Cost : OutCosts)
	{
		UResource* Resource = FindActiveResource(Cost.ResourceClass);
		if (!IsValid(Resource))
		{
			return false;
		}
		if (!Resource->CalculateAndCheckAbilityCost(Ability, Cost.Cost, Cost.bStaticCost))
		{
			return false;
		}
	}
	return true;
}

void UResourceHandler::AuthCommitAbilityCosts(UCombatAbility* Ability, int32 const PredictionID,
	TArray<FAbilityCost> const& Costs)
{
	for (FAbilityCost const& Cost : Costs)
	{
		UResource* Resource = FindActiveResource(Cost.ResourceClass);
		if (!IsValid(Resource))
		{
			continue;
		}
		Resource->CommitAbilityCost(Ability, PredictionID, Cost.Cost);
	}
}

void UResourceHandler::PredictCommitAbilityCosts(UCombatAbility* Ability, int32 const PredictionID,
	TArray<FAbilityCost> const& Costs)
{
	for (FAbilityCost const& Cost : Costs)
	{
		UResource* Resource = FindActiveResource(Cost.ResourceClass);
		if (IsValid(Resource))
		{
			Resource->PredictAbilityCost(Ability, PredictionID, Cost.Cost);
		}
	}
}

void UResourceHandler::RollbackFailedCosts(TArray<TSubclassOf<UResource>> const& CostClasses, int32 const PredictionID)
{
	for (TSubclassOf<UResource> const ResourceClass : CostClasses)
	{
		UResource* Resource = FindActiveResource(ResourceClass);
		if (IsValid(Resource))
		{
			Resource->RollbackFailedCost(PredictionID);
		}
	}
}

void UResourceHandler::UpdatePredictedCostsFromServer(FServerAbilityResult const& ServerResult, TArray<TSubclassOf<UResource>> const& MispredictedCosts)
{
	for (FAbilityCost const& Cost : ServerResult.AbilityCosts)
	{
		UResource* Resource = FindActiveResource(Cost.ResourceClass);
		if (IsValid(Resource))
		{
			Resource->UpdateCostPredictionFromServer(ServerResult.PredictionID, Cost.Cost);
		}
	}
	for (TSubclassOf<UResource> const ResourceClass : MispredictedCosts)
	{
		UResource* Resource = FindActiveResource(ResourceClass);
		if (IsValid(Resource))
		{
			Resource->RollbackFailedCost(ServerResult.PredictionID);
		}
	}
}

void UResourceHandler::SubscribeToResourceAdded(FResourceInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnResourceAdded.AddUnique(Callback);
}

void UResourceHandler::UnsubscribeFromResourceAdded(FResourceInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnResourceAdded.Remove(Callback);
}

void UResourceHandler::SubscribeToResourceRemoved(FResourceInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnResourceRemoved.AddUnique(Callback);
}

void UResourceHandler::UnsubscribeFromResourceRemoved(FResourceInstanceCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnResourceRemoved.Remove(Callback);
}

void UResourceHandler::FinishRemoveResource(UResource* Resource)
{
	RecentlyRemovedResources.RemoveSingleSwap(Resource);
}

void UResourceHandler::AddNewResource(TSubclassOf<UResource> const ResourceClass, FResourceInitInfo const& InitInfo)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (!IsValid(ResourceClass) || CheckResourceAlreadyExists(ResourceClass))
	{
		return;
	}
	UResource* NewResource = NewObject<UResource>(GetOwner(), ResourceClass);
	ActiveResources.Add(NewResource);
	NewResource->AuthInitializeResource(this, StatHandler, InitInfo);
	if (NewResource->GetInitialized())
	{
		OnResourceAdded.Broadcast(NewResource);
	}
	else
	{
		ActiveResources.Remove(NewResource);
	}
}

void UResourceHandler::RemoveResource(TSubclassOf<UResource> const ResourceClass)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	UResource* Resource = FindActiveResource(ResourceClass);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->AuthDeactivateResource();
	if (Resource->GetDeactivated())
	{
		if (ActiveResources.Remove(Resource) != 0)
    	{
    		OnResourceRemoved.Broadcast(Resource);
    		RecentlyRemovedResources.Add(Resource);
			FTimerHandle RemovalHandle;
			FTimerDelegate RemovalDelegate;
           	RemovalDelegate.BindUFunction(this, FName(TEXT("FinishRemoveResource")), Resource);
           	GetWorld()->GetTimerManager().SetTimer(RemovalHandle, RemovalDelegate, 1.0f, false);
    	}
	}
}

bool UResourceHandler::CheckResourceAlreadyExists(TSubclassOf<UResource> const ResourceClass) const
{
	if (!IsValid(ResourceClass))
	{
		return false;
	}
	for (UResource* Resource : ActiveResources)
	{
		if (Resource->GetClass() == ResourceClass)
		{
			return true;
		}
	}
	return false;
}