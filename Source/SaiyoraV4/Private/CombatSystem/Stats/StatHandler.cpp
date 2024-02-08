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
	Stats.Handler = this;
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (FCombatStat& Stat : Stats.Items)
		{
			const FModifiableFloatCallback Callback = FModifiableFloatCallback::CreateLambda([&](const float OldValue, const float NewValue)
			{
				Stats.MarkItemDirty(Stat);
				Stat.OnStatChanged.Broadcast(Stat.StatTag, NewValue);
			});
			Stat.StatValue.SetUpdatedCallback(Callback);
			Stat.StatValue.Init();
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
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UStatHandler, StatTemplate))
	{
		Stats.Items.Empty();
		if (IsValid(StatTemplate))
		{
			TArray<FStatInitInfo*> StatInitInfo;
			StatTemplate->GetAllRows(TEXT("Stat Init Info"), StatInitInfo);
			for (const FStatInitInfo* InitInfo : StatInitInfo)
			{
				if (InitInfo)
				{
					Stats.Items.Add(FCombatStat(InitInfo));
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
		for (const FCombatStat& Stat : Stats.Items)
		{
			if (Stat.StatTag == StatTag)
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
			if (Stat.StatTag == StatTag)
			{
				return Stat.StatValue.GetCurrentValue();
			}
		}
	}
	return -1.0f;
}

bool UStatHandler::IsStatModifiable(const FGameplayTag StatTag) const
{
	if (StatTag.IsValid() && StatTag.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !StatTag.MatchesTagExact(FSaiyoraCombatTags::Get().Stat))
	{
		for (const FCombatStat& Stat : Stats.Items)
		{
			if (Stat.StatTag == StatTag)
			{
				return Stat.StatValue.IsModifiable();
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
		if (Stat.StatTag == StatTag)
		{
			Stat.OnStatChanged.AddUnique(Callback);
			bFound = true;
			break;
		}
	}
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
		if (Stat.StatTag == StatTag)
		{
			Stat.OnStatChanged.Remove(Callback);
			bFound = true;
			break;
		}
	}
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
			if (Stat.StatTag == StatTag)
			{
				return Stat.StatValue.AddModifier(Modifier);
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
			if (Stat.StatTag == StatTag)
			{
				Stat.StatValue.RemoveModifier(ModifierHandle);
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
			if (Stat.StatTag == StatTag)
			{
				Stat.StatValue.UpdateModifier(ModifierHandle, Modifier);
				return;
			}
		}
	}
}

#pragma endregion 
