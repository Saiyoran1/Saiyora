#include "StatHandler.h"
#include "UnrealNetwork.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"

#pragma region Init

UStatHandler::UStatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UStatHandler::InitializeComponent()
{
	checkf(GetOwner()->Implements<USaiyoraCombatInterface>(), TEXT("Owner does not implement combat interface, but has Stat Handler."));
	if (GetOwnerRole() == ROLE_Authority)
	{
		//Copy the editor-exposed stats to a replicated array so that clients don't get duplicates.
		Stats.Items = CombatStats;
		for (FCombatStat& Stat : Stats.Items)
		{
			const FModifiableFloatCallback Callback = FModifiableFloatCallback::CreateLambda([&](const float OldValue, const float NewValue)
			{
				Stats.MarkItemDirty(Stat);
				Stat.OnStatChanged.Broadcast(Stat.StatTag, NewValue);
			});
			Stat.SetUpdatedCallback(Callback);
			Stat.Init();
			Stats.MarkItemDirty(Stat);
		}
	}
}

void UStatHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UStatHandler, Stats);
}

#if WITH_EDITOR
void UStatHandler::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	//When changing the stat template in the editor, clear out existing stats and populate the array from the table's rows.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UStatHandler, StatTemplate))
	{
		CombatStats.Empty();
		if (IsValid(StatTemplate))
		{
			TArray<FStatInitInfo*> StatInitInfo;
			StatTemplate->GetAllRows(TEXT("Stat Init Info"), StatInitInfo);
			for (const FStatInitInfo* InitInfo : StatInitInfo)
			{
				if (InitInfo)
				{
					CombatStats.Add(FCombatStat(InitInfo));
				}
			}
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

#pragma endregion
#pragma region Stat Management

bool UStatHandler::IsStatValid(const FGameplayTag StatTag) const
{
	if (StatTag.IsValid() && StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		//We check the editor-exposed version of the stat instead of the actual replicated version because clients may not have the replicated ones on startup.
		for (const FCombatStat& Stat : CombatStats)
		{
			if (Stat.StatTag.MatchesTagExact(StatTag))
			{
				return true;
			}
		}
	}
	return false;
}

float UStatHandler::GetStatValue(const FGameplayTag StatTag) const
{
	if (StatTag.IsValid() && StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		for (const FCombatStat& Stat : Stats.Items)
		{
			if (Stat.StatTag.MatchesTagExact(StatTag))
			{
				return Stat.GetCurrentValue();
			}
		}
		//If we didn't find the stat value, check defaults. It's likely that if we are a client, we didn't get the replicated stat yet.
		for (const FCombatStat& Stat : CombatStats)
		{
			if (Stat.StatTag.MatchesTagExact(StatTag))
			{
				return Stat.GetDefaultValue();
			}
		}
	}
	return -1.0f;
}

bool UStatHandler::IsStatModifiable(const FGameplayTag StatTag) const
{
	if (StatTag.IsValid() && StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		//Check the editor-exposed values, since clients may not have received the replicated values on startup.
		for (const FCombatStat& Stat : CombatStats)
		{
			if (Stat.StatTag.MatchesTagExact(StatTag))
			{
				return Stat.IsModifiable();
			}
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
	bool bFound = false;
	for (FCombatStat& Stat : Stats.Items)
	{
		if (Stat.StatTag.MatchesTagExact(StatTag))
		{
			Stat.OnStatChanged.AddUnique(Callback);
			bFound = true;
			break;
		}
	}
	//If we didn't find the stat, it's possible that we are a client and the stat just hasn't replicated.
	//In this case, we save the delegate off to bind and execute later when the stat replicates down.
	if (!bFound)
	{
		Stats.PendingSubscriptions.Add(StatTag, Callback);
	}
}

void UStatHandler::UnsubscribeFromStatChanged(const FGameplayTag StatTag, const FStatCallback& Callback)
{
	if (!Callback.IsBound() || !StatTag.IsValid() || !StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) || StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		return;
	}
	bool bFound = false;
	for (FCombatStat& Stat : Stats.Items)
	{
		if (Stat.StatTag.MatchesTagExact(StatTag))
		{
			Stat.OnStatChanged.Remove(Callback);
			bFound = true;
			break;
		}
	}
	//If we didn't find the stat, it's possible that we are a client and the stat just hasn't replicated.
	//In this case, we save the delegate off to bind and execute later when the stat replicates down.
	//This undoes that process if something unsubscribes before the stat becomes valid.
	if (!bFound)
	{
		Stats.PendingSubscriptions.Remove(StatTag, Callback);
	}
}

#pragma endregion
#pragma region Modifiers

FCombatModifierHandle UStatHandler::AddStatModifier(const FGameplayTag StatTag, const FCombatModifier& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && Modifier.Type != EModifierType::Invalid &&
		StatTag.IsValid() && StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		for (FCombatStat& Stat : Stats.Items)
		{
			if (Stat.StatTag.MatchesTagExact(StatTag))
			{
				return Stat.AddModifier(Modifier);
			}
		}
	}
	return FCombatModifierHandle::Invalid;
}

void UStatHandler::RemoveStatModifier(const FGameplayTag StatTag, const FCombatModifierHandle& ModifierHandle)
{
	if (GetOwnerRole() == ROLE_Authority && ModifierHandle.IsValid() && StatTag.IsValid() &&
		StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		for (FCombatStat& Stat : Stats.Items)
		{
			if (Stat.StatTag.MatchesTagExact(StatTag))
			{
				Stat.RemoveModifier(ModifierHandle);
				return;
			}
		}
	}
}

void UStatHandler::UpdateStatModifier(const FGameplayTag StatTag, const FCombatModifierHandle& ModifierHandle,
	const FCombatModifier& Modifier)
{
	if (GetOwnerRole() == ROLE_Authority && ModifierHandle.IsValid() && StatTag.IsValid() &&
		StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		for (FCombatStat& Stat : Stats.Items)
		{
			if (Stat.StatTag.MatchesTagExact(StatTag))
			{
				Stat.UpdateModifier(ModifierHandle, Modifier);
				return;
			}
		}
	}
}

#pragma endregion 
