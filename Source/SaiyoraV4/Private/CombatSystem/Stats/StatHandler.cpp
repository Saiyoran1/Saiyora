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
		for (const FStatInfo* InitInfo : InitialStatArray)
		{
			if (!InitInfo || !InitInfo->StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || InitInfo->StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
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

void UStatHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UStatHandler, ReplicatedStats);
}

#pragma endregion
#pragma region Stat Management

bool UStatHandler::IsStatValid(const FGameplayTag StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		return false;
	}
	for (const TTuple<FGameplayTag, FStatInfo>& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			return true;
		}
	}
	return false;
}

float UStatHandler::GetStatValue(const FGameplayTag StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		return -1.0f;
	}
	bool bFoundInDefaults = false;
	bool bStatReplicates = false;
	float DefaultValue = -1.0f;
	for (const TTuple<FGameplayTag, FStatInfo>& DefaultInfo : StatDefaults)
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
		for (const FCombatStat& Stat : ReplicatedStats.Items)
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
		if (const FCombatStat* Stat = NonReplicatedStats.Find(StatTag))
		{
			return Stat->GetValue();
		}
	}
	return DefaultValue;
}

bool UStatHandler::IsStatModifiable(const FGameplayTag StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		return false;
	}
	for (const TTuple<FGameplayTag, FStatInfo>& DefaultInfo : StatDefaults)
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

void UStatHandler::SubscribeToStatChanged(const FGameplayTag StatTag, const FStatCallback& Callback)
{
	if (!Callback.IsBound() || !StatTag.IsValid() || !StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		return;
	}
	for (const TTuple<FGameplayTag, FStatInfo>& DefaultInfo : StatDefaults)
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
				if (FCombatStat* Stat = NonReplicatedStats.Find(StatTag))
				{
					Stat->SubscribeToStatChanged(Callback);
				}
			}	
			return;
		}
	}
}

void UStatHandler::UnsubscribeFromStatChanged(const FGameplayTag StatTag, const FStatCallback& Callback)
{
	if (!Callback.IsBound() || !StatTag.IsValid() || !StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		return;
	}
	for (const TTuple<FGameplayTag, FStatInfo>& DefaultInfo : StatDefaults)
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
				if (FCombatStat* Stat = NonReplicatedStats.Find(StatTag))
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

void UStatHandler::AddStatModifier(const FGameplayTag StatTag, const FCombatModifier& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || Modifier.Type == EModifierType::Invalid ||
		!StatTag.IsValid() || !StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat) ||
		!IsValid(Modifier.BuffSource) || Modifier.BuffSource->GetAppliedTo() != GetOwner())
	{
		return;
	}
	for (const TTuple<FGameplayTag, FStatInfo>& DefaultInfo : StatDefaults)
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
				if (FCombatStat* Stat = NonReplicatedStats.Find(StatTag))
				{
					Stat->AddModifier(Modifier);
				}
			}	
			return;
		}
	}
}

void UStatHandler::RemoveStatModifier(const FGameplayTag StatTag, const UBuff* Source)
{
	if (GetOwnerRole() != ROLE_Authority || !StatTag.IsValid() || !StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) ||
		StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	for (const TTuple<FGameplayTag, FStatInfo>& DefaultInfo : StatDefaults)
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
				if (FCombatStat* Stat = NonReplicatedStats.Find(StatTag))
				{
					Stat->RemoveModifier(Source);
				}
			}	
			return;
		}
	}
}

#pragma endregion 