#include "StatHandler.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatStat.h"
#include "SaiyoraCombatInterface.h"

void UStatHandler::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Stat Handler."), *GetOwner()->GetActorLabel());
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
		}
		if (IsValid(BuffHandler))
		{
			//Setup a restriction on buffs that modify stats that this component has determined can not be modified.
			FBuffRestriction BuffStatModCondition;
			BuffStatModCondition.BindDynamic(this, &UStatHandler::CheckBuffStatMods);
			BuffHandler->AddIncomingBuffRestriction(BuffStatModCondition);
		}
	}
}

UStatHandler::UStatHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UStatHandler::InitializeComponent()
{
	TArray<FStatInfo*> InitialStatArray;
	InitialStats->GetAllRows<FStatInfo>(nullptr, InitialStatArray);
	for (FStatInfo const* InitInfo : InitialStatArray)
	{
		if (!InitInfo || !InitInfo->StatTag.MatchesTag(GenericStatTag()))
		{
			continue;
		}
		StatDefaults.Add(InitInfo->StatTag, *InitInfo);
		if (GetOwnerRole() == ROLE_Authority)
		{
			if (InitInfo->bModifiable)
			{
				FCombatStat& NewStat = ModdableStats.Items.Add_GetRef(FCombatStat(*InitInfo));
				if (NewStat.ShouldReplicate())
				{
					ModdableStats.MarkItemDirty(NewStat);
				}
			}
		}
	}
}

void UStatHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UStatHandler, ModdableStats);
}

bool UStatHandler::IsStatValid(FGameplayTag const StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()) || StatTag.MatchesTagExact(GenericStatTag()))
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
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()) || StatTag.MatchesTagExact(GenericStatTag()))
	{
		return -1.0f;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (DefaultInfo.Value.bModifiable && (GetOwnerRole() == ROLE_Authority || DefaultInfo.Value.bShouldReplicate))
			{
				for (FCombatStat const& Stat : ModdableStats.Items)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag) && Stat.IsInitialized())
					{
						return Stat.GetValue();
					}
				}
			}
			return DefaultInfo.Value.DefaultValue;
		}
	}
	return -1.0f;
}

void UStatHandler::AddStatModifier(UBuff* Source, FGameplayTag const StatTag, FCombatModifier const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || Modifier.Type() == EModifierType::Invalid || !
		StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()) || StatTag.MatchesTagExact(GenericStatTag()) ||
		!IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (DefaultInfo.Value.bModifiable)
			{
				for (FCombatStat& Stat : ModdableStats)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag))
					{
						if (Stat.IsInitialized())
						{
							Stat.AddModifier(Modifier);
							if (Stat.ShouldReplicate())
							{
								ModdableStats.MarkItemDirty(Stat);
							}
							return;
						}
					}
				}
			}
			return;
		}
	}
}

void UStatHandler::RemoveStatModifier(UBuff* Source, FGameplayTag const StatTag)
{
	if (GetOwnerRole() != ROLE_Authority || !StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()) ||
		StatTag.MatchesTagExact(GenericStatTag()) || !IsValid(Source) || Source->GetAppliedTo() != GetOwner())
	{
		return;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (DefaultInfo.Value.bModifiable)
			{
				for (FCombatStat& Stat : ModdableStats)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag))
					{
						if (Stat.IsInitialized())
						{
							Stat.RemoveModifier(Source);
							if (Stat.ShouldReplicate())
							{
								ModdableStats.MarkItemDirty(Stat);
							}
							return;
						}
					}
				}
			}
			return;
		}
	}
}

void UStatHandler::SubscribeToStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound() || !StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()) || StatTag.MatchesTagExact(GenericStatTag()))
	{
		return;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (DefaultInfo.Value.bModifiable && (GetOwnerRole() == ROLE_Authority || DefaultInfo.Value.bShouldReplicate))
			{
				for (FCombatStat& Stat : ModdableStats)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag) && Stat.IsInitialized())
					{
						Stat.SubscribeToStatChanged(Callback);
						return;
					}
				}
			}
			return;
		}
	}
}

void UStatHandler::UnsubscribeFromStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound() || !StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()) || StatTag.MatchesTagExact(GenericStatTag()))
	{
		return;
	}
	for (TTuple<FGameplayTag, FStatInfo> const& DefaultInfo : StatDefaults)
	{
		if (DefaultInfo.Key.MatchesTagExact(StatTag))
		{
			if (DefaultInfo.Value.bModifiable && (GetOwnerRole() == ROLE_Authority || DefaultInfo.Value.bShouldReplicate))
			{
				for (FCombatStat& Stat : ModdableStats)
				{
					if (Stat.GetStatTag().MatchesTagExact(StatTag) && Stat.IsInitialized())
					{
						Stat.UnsubscribeFromStatChanged(Callback);
						return;
					}
				}
			}
			return;
		}
	}
}

bool UStatHandler::CheckBuffStatMods(FBuffApplyEvent const& BuffEvent)
{
	if (!IsValid(BuffEvent.BuffClass))
	{
		return false;
	}
	UBuff const* Default = BuffEvent.BuffClass.GetDefaultObject();
	if (!IsValid(Default))
	{
		return false;
	}
	FGameplayTagContainer BuffTags;
	Default->GetBuffTags(BuffTags);
	if (BuffTags.HasAny(UnmodifiableStats))
	{
		return true;
	}
	return false;
}
