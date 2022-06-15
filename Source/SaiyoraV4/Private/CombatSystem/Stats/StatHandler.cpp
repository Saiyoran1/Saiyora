#include "StatHandler.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Setup

UStatHandler::UStatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UStatHandler::InitializeComponent()
{
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Stat Handler."));
	ReplicatedStats.Handler = this;
	if (IsValid(InitialStats))
	{
		TArray<FStatInfo*> InitialStatArray;
		InitialStats->GetAllRows<FStatInfo>(nullptr, InitialStatArray);
		for (FStatInfo const* InitInfo : InitialStatArray)
		{
			if (!InitInfo || !InitInfo->StatTag.MatchesTag(FStatTags::GenericStat) || InitInfo->StatTag.MatchesTagExact(FStatTags::GenericStat))
			{
				continue;
			}
			StatDefaults.Add(InitInfo->StatTag, *InitInfo);
			if (GetOwnerRole() == ROLE_Authority)
			{
				if (!InitInfo->bModifiable)
				{
					StaticStats.AddTag(InitInfo->StatTag);
					continue;
				}
				if (InitInfo->bShouldReplicate)
				{
					FCombatStat& NewStat = ReplicatedStats.Items.Add_GetRef(FCombatStat(*InitInfo));
					ReplicatedStats.MarkItemDirty(NewStat);
				}
				else
				{
					NonReplicatedStats.Add(InitInfo->StatTag, FCombatStat(*InitInfo));
				}
			}
		}
	}
}

void UStatHandler::BeginPlay()
{
	Super::BeginPlay();
}

void UStatHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UStatHandler, ReplicatedStats);
}

#pragma endregion
#pragma region Stat Management

bool UStatHandler::IsStatValid(FGameplayTag const StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(FStatTags::GenericStat) || StatTag.MatchesTagExact(FStatTags::GenericStat))
	{
		return false;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			return true;
		}
	}
	return false;
}

float UStatHandler::GetStatValue(FGameplayTag const StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(FStatTags::GenericStat) || StatTag.MatchesTagExact(FStatTags::GenericStat))
	{
		return -1.0f;
	}
	bool bFoundInDefaults = false;
	bool bStatReplicates = false;
	float DefaultValue = -1.0f;
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (!DefaultInfo.Value.bModifiable || (!DefaultInfo.Value.bShouldReplicate && GetOwnerRole() != ROLE_Authority))
			{
				return DefaultInfo.Value.DefaultValue;
			}
			bFoundInDefaults = true;
			bStatReplicates = DefaultInfo.Value.bShouldReplicate;
			DefaultValue = DefaultInfo.Value.DefaultValue;
			break;
		}
	}
	if (!bFoundInDefaults)
	{
		return -1.0f;
	}
	if (bStatReplicates)
	{
		for (FCombatStat const& Stat : ReplicatedStats.Items)
		{
			if (Stat.GetStatTag().MatchesTagExact(StatTag))
			{
				if (Stat.IsInitialized())
				{
					return Stat.GetValue();
				}
				break;
			}
		}
	}
	else
	{
		FCombatStat const* Stat = NonReplicatedStats.Find(StatTag);
		if (Stat)
		{
			return Stat->GetValue();
		}
	}
	return DefaultValue;
}

bool UStatHandler::IsStatModifiable(FGameplayTag const StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(FStatTags::GenericStat) || StatTag.MatchesTagExact(FStatTags::GenericStat))
	{
		return false;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			return DefaultInfo.Value.bModifiable;
		}
	}
	return false;
}

#pragma endregion
#pragma region Subscriptions

void UStatHandler::SubscribeToStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound() || !StatTag.IsValid() || !StatTag.MatchesTag(FStatTags::GenericStat) || StatTag.MatchesTagExact(FStatTags::GenericStat))
	{
		return;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (!DefaultInfo.Value.bModifiable)
			{
				return;
			}
			if (DefaultInfo.Value.bShouldReplicate)
			{
				for (FCombatStat& Stat : ReplicatedStats.Items)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag))
					{
						if (Stat.IsInitialized())
						{
							Stat.SubscribeToStatChanged(Callback);
							return;
						}
						break;
					}
				}
				ReplicatedStats.PendingSubscriptions.AddUnique(StatTag, Callback);
			}
			else if (GetOwnerRole() == ROLE_Authority)
			{
				FCombatStat* Stat = NonReplicatedStats.Find(StatTag);
				if (Stat)
				{
					Stat->SubscribeToStatChanged(Callback);
				}
			}	
			return;
		}
	}
}

void UStatHandler::UnsubscribeFromStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound() || !StatTag.IsValid() || !StatTag.MatchesTag(FStatTags::GenericStat) || StatTag.MatchesTagExact(FStatTags::GenericStat))
	{
		return;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (!DefaultInfo.Value.bModifiable)
			{
				return;
			}
			if (DefaultInfo.Value.bShouldReplicate)
			{
				for (FCombatStat& Stat : ReplicatedStats.Items)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag))
					{
						if (Stat.IsInitialized())
						{
							Stat.UnsubscribeFromStatChanged(Callback);
							return;
						}
						break;
					}
				}
				ReplicatedStats.PendingSubscriptions.Remove(StatTag, Callback);
			}
			else if (GetOwnerRole() == ROLE_Authority)
			{
				FCombatStat* Stat = NonReplicatedStats.Find(StatTag);
				if (Stat)
				{
					Stat->UnsubscribeFromStatChanged(Callback);
				}
			}	
			return;
		}
	}
}

#pragma endregion
#pragma region Modifiers

void UStatHandler::AddStatModifier(FGameplayTag const StatTag, FCombatModifier const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || Modifier.Type == EModifierType::Invalid ||
		!StatTag.IsValid() || !StatTag.MatchesTag(FStatTags::GenericStat) || StatTag.MatchesTagExact(FStatTags::GenericStat) ||
		!IsValid(Modifier.Source) || Modifier.Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (!DefaultInfo.Value.bModifiable)
			{
				return;
			}
			if (DefaultInfo.Value.bShouldReplicate)
			{
				for (FCombatStat& Stat : ReplicatedStats.Items)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag))
					{
						if (Stat.IsInitialized())
						{
							Stat.AddModifier(Modifier);
							ReplicatedStats.MarkItemDirty(Stat);
						}
						return;
					}
				}
			}
			else
			{
				FCombatStat* Stat = NonReplicatedStats.Find(StatTag);
				if (Stat)
				{
					Stat->AddModifier(Modifier);
				}
			}	
			return;
		}
	}
}

void UStatHandler::RemoveStatModifier(FGameplayTag const StatTag, UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !StatTag.IsValid() || !StatTag.MatchesTag(FStatTags::GenericStat) ||
		StatTag.MatchesTagExact(FStatTags::GenericStat) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (!DefaultInfo.Value.bModifiable)
			{
				return;
			}
			if (DefaultInfo.Value.bShouldReplicate)
			{
				for (FCombatStat& Stat : ReplicatedStats.Items)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag))
					{
						if (Stat.IsInitialized())
						{
							Stat.RemoveModifier(Source);
							ReplicatedStats.MarkItemDirty(Stat);
						}
						return;
					}
				}
			}
			else
			{
				FCombatStat* Stat = NonReplicatedStats.Find(StatTag);
				if (Stat)
				{
					Stat->RemoveModifier(Source);
				}
			}	
			return;
		}
	}
}

#pragma endregion 