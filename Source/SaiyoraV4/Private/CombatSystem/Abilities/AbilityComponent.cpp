#include "AbilityComponent.h"

#include "AbilityStructs.h"
#include "CombatAbility.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"
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
		StatCooldownMod.BindDynamic(this, &UAbilityHandler::ModifyCooldownFromStat);
		AddConditionalCooldownModifier(StatCooldownMod);
		StatGlobalCooldownMod.BindDynamic(this, &UAbilityHandler::ModifyGlobalCooldownFromStat);
		AddConditionalGlobalCooldownModifier(StatGlobalCooldownMod);
		StatCastLengthMod.BindDynamic(this, &UAbilityHandler::ModifyCastLengthFromStat);
		AddConditionalCastLengthModifier(StatCastLengthMod);
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(CrowdControlHandlerRef))
		{
			CcCallback.BindDynamic(this, &UAbilityHandler::InterruptCastOnCrowdControl);
			CrowdControlHandler->SubscribeToCrowdControlChanged(CcCallback);
		}
		if (IsValid(DamageHandlerRef))
		{
			DeathCallback.BindDynamic(this, &UAbilityHandler::InterruptCastOnDeath);
			DamageHandler->SubscribeToLifeStatusChanged(DeathCallback);
		}
		for (TSubclassOf<UCombatAbility> const AbilityClass : DefaultAbilities)
		{
			AddNewAbility(AbilityClass);
		}
	}	
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
	
	
	if (Result.Ability->HasGlobalCooldown())
	{
		StartGlobal(Result.Ability);
	}
	
	Result.Ability->CommitCharges();
	
	if (Costs.Num() > 0 && IsValid(ResourceHandler))
	{
		ResourceHandler->CommitAbilityCosts(Result.Ability, Costs);
	}

	FCombatParameters BroadcastParams;
	
	switch (Result.Ability->GetCastType())
	{
	case EAbilityCastType::None :
		Result.FailReason = ECastFailReason::InvalidCastType;
		return Result;
	case EAbilityCastType::Instant :
		Result.ActionTaken = ECastAction::Success;
		Result.Ability->ServerTick(0, BroadcastParams);
		OnAbilityTick.Broadcast(Result);
		MulticastAbilityTick(Result, BroadcastParams);
		break;
	case EAbilityCastType::Channel :
		Result.ActionTaken = ECastAction::Success;
		StartCast(Result.Ability);
		if (Result.Ability->GetHasInitialTick())
		{
			Result.Ability->ServerTick(0, BroadcastParams);
			OnAbilityTick.Broadcast(Result);
			MulticastAbilityTick(Result, BroadcastParams);
		}
		break;
	default :
		Result.FailReason = ECastFailReason::InvalidCastType;
		return Result;
	}
	
	return Result;
}

bool UAbilityComponent::CanUseAbility(UCombatAbility* Ability, TMap<TSubclassOf<UResource>, float>& OutCosts,
	ECastFailReason& OutFailReason) const
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
	OutFailReason = Ability->IsCastable(OutCosts);
	if (OutFailReason != ECastFailReason::None)
	{
		return false;
	}
	//TODO: These checks should probably be done per-ability when adding and removing restrictions?
	TArray<FGameplayTag> RestrictedTags;
	AbilityUsageTagRestrictions.GenerateValueArray(RestrictedTags);
	FGameplayTagContainer RestrictedTagContainer;
	RestrictedTagContainer.CreateFromArray(RestrictedTags);
	FGameplayTagContainer AbilityTags;
	Ability->GetAbilityTags(AbilityTags);
	if (AbilityTags.HasAny(RestrictedTagContainer))
	{
		OutFailReason = ECastFailReason::CustomRestriction;
		return false;
	}
	TArray<TSubclassOf<UCombatAbility>> RestrictedClasses;
	AbilityUsageClassRestrictions.GenerateValueArray(RestrictedClasses);
	if (RestrictedClasses.Contains(Ability->GetClass()))
	{
		OutFailReason = ECastFailReason::CustomRestriction;
		return false;
	}
	return true;
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

void UAbilityComponent::AddAbilityTagRestriction(UObject* Source, FGameplayTag const Tag)
{
	if (!IsValid(Source) || !Tag.IsValid())
	{
		return;
	}
	bool const bAlreadyRestricted = AbilityUsageTagRestrictions.FindKey(Tag);
	AbilityUsageTagRestrictions.AddUnique(Source, Tag);
	if (!bAlreadyRestricted)
	{
		for (TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*> const& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Value))
			{
				FGameplayTagContainer AbilityTags;
				AbilityPair.Value->GetAbilityTags(AbilityTags);
				if (AbilityTags.HasTag(Tag))
				{
					//TODO: Notify ability of new tag restriction.
				}
			}
		}
	}
}

void UAbilityComponent::RemoveAbilityTagRestriction(UObject* Source, FGameplayTag const Tag)
{
	if (!IsValid(Source) || !Tag.IsValid())
	{
		return;
	}
	if (AbilityUsageTagRestrictions.Remove(Source, Tag) > 0 && !AbilityUsageTagRestrictions.FindKey(Tag))
	{
		for (TTuple<TSubclassOf<UCombatAbility>, UCombatAbility*> const& AbilityPair : ActiveAbilities)
		{
			if (IsValid(AbilityPair.Value))
			{
				FGameplayTagContainer AbilityTags;
				AbilityPair.Value->GetAbilityTags(AbilityTags);
				if (AbilityTags.HasTag(Tag))
				{
					//TODO: Notify ability tag restriction removed.
				}
			}
		}
	}
}

#pragma endregion
