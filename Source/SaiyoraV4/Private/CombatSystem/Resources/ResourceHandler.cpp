#include "ResourceHandler.h"
#include "Resource.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "CombatAbility.h"
#include "AbilityComponent.h"
#include "SaiyoraCombatInterface.h"

#pragma region Setup

UResourceHandler::UResourceHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UResourceHandler::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (const TTuple<TSubclassOf<UResource>, FResourceInitInfo>& InitInfo : DefaultResources)
		{
			AddNewResource(InitInfo.Key, InitInfo.Value);
		}
	}
}

void UResourceHandler::InitializeComponent()
{
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Resource Handler."));
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

#pragma endregion 
#pragma region Resource Management

UResource* UResourceHandler::FindActiveResource(const TSubclassOf<UResource> ResourceClass) const
{
	if (!IsValid(ResourceClass))
	{
		return nullptr;
	}
	for (UResource* Resource : ActiveResources)
	{
		if (IsValid(Resource) && Resource->GetClass() == ResourceClass)
		{
			if (Resource->IsInitialized() && !Resource->IsDeactivated())
			{
				return Resource;
			}
			break;
		}
	}
	return nullptr;
}

void UResourceHandler::AddNewResource(const TSubclassOf<UResource> ResourceClass, const FResourceInitInfo& InitInfo)
{
	if (GetOwnerRole() != ROLE_Authority || IsValid(FindActiveResource(ResourceClass)))
	{
		return;
	}
	UResource* NewResource = NewObject<UResource>(GetOwner(), ResourceClass);
	ActiveResources.Add(NewResource);
	NewResource->InitializeResource(this, InitInfo);
	if (NewResource->IsInitialized() && !NewResource->IsDeactivated())
	{
		OnResourceAdded.Broadcast(NewResource);
	}
	else
	{
		ActiveResources.Remove(NewResource);
	}
}

void UResourceHandler::RemoveResource(const TSubclassOf<UResource> ResourceClass)
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
	Resource->DeactivateResource();
	if (Resource->IsDeactivated())
	{
		if (ActiveResources.Remove(Resource) > 0)
		{
			OnResourceRemoved.Broadcast(Resource);
			RecentlyRemovedResources.Add(Resource);
			FTimerHandle RemovalHandle;
			const FTimerDelegate RemovalDelegate = FTimerDelegate::CreateUObject(this, &UResourceHandler::FinishRemoveResource, Resource);
			GetWorld()->GetTimerManager().SetTimer(RemovalHandle, RemovalDelegate, 1.0f, false);
		}
	}
}

void UResourceHandler::FinishRemoveResource(UResource* Resource)
{
	RecentlyRemovedResources.Remove(Resource);
}

void UResourceHandler::NotifyOfReplicatedResource(UResource* Resource)
{
	if (!IsValid(Resource) || IsValid(FindActiveResource(Resource->GetClass())))
	{
		return;
	}
	ActiveResources.Add(Resource);
	OnResourceAdded.Broadcast(Resource);
}

void UResourceHandler::NotifyOfRemovedReplicatedResource(UResource* Resource)
{
	if (IsValid(Resource) && ActiveResources.Remove(Resource) > 0)
	{
		OnResourceRemoved.Broadcast(Resource);
	}
}

#pragma endregion 
#pragma region Ability Costs

void UResourceHandler::CommitAbilityCosts(UCombatAbility* Ability, const int32 PredictionID)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy || !IsValid(Ability))
	{
		return;
	}
	TArray<FAbilityCost> Costs;
	Ability->GetAbilityCosts(Costs);
	for (const FAbilityCost& Cost : Costs)
	{
		if (IsValid(Cost.ResourceClass))
		{
			UResource* Resource = FindActiveResource(Cost.ResourceClass);
			if (IsValid(Resource))
			{
				Resource->CommitAbilityCost(Ability, Cost.Cost, PredictionID);
			}
			if (GetOwnerRole() == ROLE_AutonomousProxy)
			{
				CostPredictions.Add(PredictionID, Cost);
			}
		}
	}
	
}

void UResourceHandler::UpdatePredictedCostsFromServer(const FServerAbilityResult& ServerResult)
{
	TArray<FAbilityCost> PredictedCosts;
	CostPredictions.MultiFind(ServerResult.PredictionID, PredictedCosts);
	for (const FAbilityCost& Cost : ServerResult.AbilityCosts)
	{
		if (IsValid(Cost.ResourceClass))
		{
			for (int32 i = 0; i < PredictedCosts.Num(); i++)
			{
				if (IsValid(PredictedCosts[i].ResourceClass) && Cost.ResourceClass == PredictedCosts[i].ResourceClass)
				{
					if (Cost.Cost != PredictedCosts[i].Cost)
					{
						UResource* Resource = FindActiveResource(Cost.ResourceClass);
						if (IsValid(Resource))
						{
							Resource->UpdateCostPredictionFromServer(ServerResult.PredictionID, Cost.Cost);
						}
					}
					PredictedCosts.RemoveAt(i);
					break;
				}
			}
		}
	}
	for (const FAbilityCost& Misprediction : PredictedCosts)
	{
		if (IsValid(Misprediction.ResourceClass))
		{
			UResource* Resource = FindActiveResource(Misprediction.ResourceClass);
			if (IsValid(Resource))
			{
				Resource->UpdateCostPredictionFromServer(ServerResult.PredictionID, 0.0f);
			}
		}
	}
	CostPredictions.Remove(ServerResult.PredictionID);
}

#pragma endregion 