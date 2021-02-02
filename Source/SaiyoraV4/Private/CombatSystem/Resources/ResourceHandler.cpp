// Fill out your copyright notice in the Description page of Project Settings.


#include "ResourceHandler.h"

#include "GameplayTasksComponent.h"
#include "Resource.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "CombatAbility.h"

const FGameplayTag UResourceHandler::GenericResourceTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Resource")), false);

UResourceHandler::UResourceHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UResourceHandler::BeginPlay()
{
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

void UResourceHandler::CommitAbilityCosts(UCombatAbility* Ability, int32 const CastID,
	TArray<FAbilityCost> const& Costs)
{
	for (FAbilityCost const& Cost : Costs)
	{
		UResource* Resource = FindActiveResource(Cost.ResourceTag);
		if (!IsValid(Resource))
		{
			continue;
		}
		Resource->CommitAbilityCost(Ability, CastID, Cost);
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
	NewResource->AuthInitializeResource(this, InitInfo);
	if (NewResource->GetInitialized())
	{
		ActiveResources.Add(NewResource->GetResourceTag(), NewResource);
		OnResourceAdded.Broadcast(NewResource->GetResourceTag());
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

//TODO: PlayerResourceHandler for prediction.
/*void UPlayerResourceHandler::PredictResourceDelta(FGameplayTag const& ResourceTag, int32 const CastID,
	float const ResourceDelta)
{
	FPredictedResource* PredictedResource = Cast<FPredictedResource>(ClientResources.Find(ResourceTag));
	if (PredictedResource)
	{
		PredictedResource->PredictedResourceDeltas.Add(CastID, ResourceDelta);
		float const PreviousValue = ClientResource->DisplayValue;
		float const NewValue = ClientResource->UpdatePredictedValue();
		if (NewValue != PreviousValue)
		{
			ClientResource->OnResourceChanged.Broadcast(ResourceTag, PreviousValue, NewValue);
		}
	}
}

void UPlayerResourceHandler::RollbackPredictedDeltas(int32 const CastID, FGameplayTagContainer const& PredictedResources)
{
	TArray<FGameplayTag> TagArray;
	PredictedResources.GetGameplayTagArray(TagArray);
	for (FGameplayTag Tag : TagArray)
	{
		FClientResource* ClientResource = ClientResources.Find(Tag);
		if (ClientResource)
		{
			float const PreviousValue = ClientResource->DisplayValue;
			float const NewValue = ClientResource->RollbackPredictedDelta(CastID);
			if (PreviousValue != NewValue)
			{
				ClientResource->OnResourceChanged.Broadcast(Tag, PreviousValue, NewValue);
			}
		}
	}
}
*/