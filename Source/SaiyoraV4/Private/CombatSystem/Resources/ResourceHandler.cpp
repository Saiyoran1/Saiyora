// Fill out your copyright notice in the Description page of Project Settings.


#include "ResourceHandler.h"

#include "GameplayTasksComponent.h"
#include "Resource.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "CombatAbility.h"
#include "SaiyoraCombatInterface.h"

const FGameplayTag UResourceHandler::GenericResourceTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Resource")), false);

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
		for (FResourceInitInfo const& InitInfo : DefaultResources)
		{
			AddNewResource(InitInfo);
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
	
	TArray<UResource*> ReplicableResources;
	ActiveResources.GenerateValueArray(ReplicableResources);
	bWroteSomething |= Channel->ReplicateSubobjectList(ReplicableResources, *Bunch, *RepFlags);
	bWroteSomething |= Channel->ReplicateSubobjectList(RecentlyRemovedResources, *Bunch, *RepFlags);

	return bWroteSomething;
}

UResource* UResourceHandler::FindActiveResource(FGameplayTag const& ResourceTag) const
{
	if (!ResourceTag.IsValid() || !ResourceTag.MatchesTag(GenericResourceTag))
	{
		return nullptr;
	}
	UResource* Found = ActiveResources.FindRef(ResourceTag);
	if (!IsValid(Found) || !Found->GetInitialized() || Found->GetDeactivated())
	{
		return nullptr;
	}
	return Found;
}

void UResourceHandler::NotifyOfReplicatedResource(UResource* ReplicatedResource)
{
	if (!ReplicatedResource->GetResourceTag().IsValid() || !ReplicatedResource->GetResourceTag().MatchesTag(GenericResourceTag))
	{
		return;
	}
	UResource* ExistingResource = FindActiveResource(ReplicatedResource->GetResourceTag());
	if (IsValid(ExistingResource))
	{
		return;
	}
	ActiveResources.Add(ReplicatedResource->GetResourceTag(), ReplicatedResource);
	OnResourceAdded.Broadcast(ReplicatedResource->GetResourceTag());
}

void UResourceHandler::NotifyOfRemovedReplicatedResource(FGameplayTag const& ResourceTag)
{
	if (ActiveResources.Remove(ResourceTag) != 0)
	{
		OnResourceRemoved.Broadcast(ResourceTag);
	}
}

FGameplayTagContainer UResourceHandler::GetActiveResources() const
{
	FGameplayTagContainer ResourceTags;
	for (TTuple<FGameplayTag, UResource*> Resource : ActiveResources)
	{
		if (IsValid(Resource.Value) && Resource.Value->GetInitialized() && !Resource.Value->GetDeactivated())
		{
			ResourceTags.AddTag(Resource.Key);
		}
	}
	return ResourceTags;
}

float UResourceHandler::GetResourceValue(FGameplayTag const& ResourceTag) const
{
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return -1.0f;
	}
	return Resource->GetCurrentValue();
}

float UResourceHandler::GetResourceMinimum(FGameplayTag const& ResourceTag) const
{
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return -1.0f;	
	}
	return Resource->GetMinimum();
}

float UResourceHandler::GetResourceMaximum(FGameplayTag const& ResourceTag) const
{
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return -1.0f;	
	}
	return Resource->GetMaximum();
}

void UResourceHandler::ModifyResource(FGameplayTag const& ResourceTag, float const Amount, UObject* Source, bool const bIgnoreModifiers)
{
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->ModifyResource(Source, Amount, bIgnoreModifiers);
}

bool UResourceHandler::CheckAbilityCostsMet(UCombatAbility* Ability, TArray<FAbilityCost>& OutCosts) const
{
	for (FAbilityCost& Cost : OutCosts)
	{
		UResource* Resource = FindActiveResource(Cost.ResourceTag);
		if (!IsValid(Resource))
		{
			return false;
		}
		if (!Resource->CalculateAndCheckAbilityCost(Ability, Cost))
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
		UResource* Resource = FindActiveResource(Cost.ResourceTag);
		if (!IsValid(Resource))
		{
			continue;
		}
		Resource->CommitAbilityCost(Ability, PredictionID, Cost);
	}
}

void UResourceHandler::PredictCommitAbilityCosts(UCombatAbility* Ability, int32 const PredictionID,
	TArray<FAbilityCost> const& Costs)
{
	for (FAbilityCost const& Cost : Costs)
	{
		UResource* Resource = FindActiveResource(Cost.ResourceTag);
		if (IsValid(Resource))
		{
			Resource->PredictAbilityCost(Ability, PredictionID, Cost);
		}
	}
}

void UResourceHandler::RollbackFailedCosts(FGameplayTagContainer const& CostTags, int32 const PredictionID)
{
	TArray<FGameplayTag> RollbackTags;
	CostTags.GetGameplayTagArray(RollbackTags);
	for (FGameplayTag const ResourceTag : RollbackTags)
	{
		UResource* Resource = FindActiveResource(ResourceTag);
		if (IsValid(Resource))
		{
			Resource->RollbackFailedCost(PredictionID);
		}
	}
}

void UResourceHandler::UpdatePredictedCostsFromServer(FServerAbilityResult const& ServerResult, TArray<FGameplayTag> const& MispredictedCosts)
{
	for (FAbilityCost const& Cost : ServerResult.AbilityCosts)
	{
		UResource* Resource = FindActiveResource(Cost.ResourceTag);
		if (IsValid(Resource))
		{
			Resource->UpdateCostPredictionFromServer(ServerResult.PredictionID, Cost);
		}
	}
	for (FGameplayTag const& MispredictionTag : MispredictedCosts)
	{
		UResource* Resource = FindActiveResource(MispredictionTag);
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

void UResourceHandler::SubscribeToResourceRemoved(FResourceTagCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnResourceRemoved.AddUnique(Callback);
}

void UResourceHandler::UnsubscribeFromResourceRemoved(FResourceTagCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnResourceRemoved.Remove(Callback);
}

void UResourceHandler::SubscribeToResourceChanged(FGameplayTag const& ResourceTag, FResourceValueCallback const& Callback)
{
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->SubscribeToResourceChanged(Callback);
}

void UResourceHandler::UnsubscribeFromResourceChanged(FGameplayTag const& ResourceTag,
	FResourceValueCallback const& Callback)
{
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->UnsubscribeFromResourceChanged(Callback);
}

void UResourceHandler::SubscribeToResourceModsChanged(FGameplayTag const& ResourceTag,
	FResourceTagCallback const& Callback)
{
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->SubscribeToResourceModsChanged(Callback);
}

void UResourceHandler::UnsubscribeFromResourceModsChanged(FGameplayTag const& ResourceTag,
	FResourceTagCallback const& Callback)
{
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->UnsubscribeFromResourceModsChanged(Callback);
}

void UResourceHandler::AddResourceDeltaModifier(FResourceDeltaModifier const& Modifier, FGameplayTag const& ResourceTag)
{
	if (!Modifier.IsBound())
	{
		return;
	}
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->AddResourceDeltaModifier(Modifier);
}

void UResourceHandler::RemoveResourceDeltaModifier(FResourceDeltaModifier const& Modifier,
	FGameplayTag const& ResourceTag)
{
	if (!Modifier.IsBound())
	{
		return;
	}
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->RemoveResourceDeltaModifier(Modifier);
}

void UResourceHandler::FinishRemoveResource(UResource* Resource)
{
	RecentlyRemovedResources.RemoveSingleSwap(Resource);
}

void UResourceHandler::AddNewResource(FResourceInitInfo const& InitInfo)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (!IsValid(InitInfo.ResourceClass) || CheckResourceAlreadyExists(InitInfo.ResourceClass->GetDefaultObject<UResource>()->GetResourceTag()))
	{
		return;
	}
	UResource* NewResource = NewObject<UResource>(GetOwner(), InitInfo.ResourceClass);
	ActiveResources.Add(NewResource->GetResourceTag(), NewResource);
	NewResource->AuthInitializeResource(this, StatHandler, InitInfo);
	if (NewResource->GetInitialized())
	{
		OnResourceAdded.Broadcast(NewResource->GetResourceTag());
	}
	else
	{
		ActiveResources.Remove(NewResource->GetResourceTag());
	}
}

void UResourceHandler::RemoveResource(FGameplayTag const& ResourceTag)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	UResource* Resource = FindActiveResource(ResourceTag);
	if (!IsValid(Resource))
	{
		return;
	}
	Resource->AuthDeactivateResource();
	if (Resource->GetDeactivated())
	{
		if (ActiveResources.Remove(ResourceTag) != 0)
    	{
    		OnResourceRemoved.Broadcast(ResourceTag);
    		RecentlyRemovedResources.Add(Resource);
			FTimerHandle RemovalHandle;
			FTimerDelegate RemovalDelegate;
           	RemovalDelegate.BindUFunction(this, FName(TEXT("FinishRemoveResource")), Resource);
           	GetWorld()->GetTimerManager().SetTimer(RemovalHandle, RemovalDelegate, 1.0f, false);
    	}
	}
}

bool UResourceHandler::CheckResourceAlreadyExists(FGameplayTag const& ResourceTag) const
{
	return ActiveResources.Contains(ResourceTag);
}