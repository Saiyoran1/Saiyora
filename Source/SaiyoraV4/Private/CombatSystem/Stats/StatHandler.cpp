// Fill out your copyright notice in the Description page of Project Settings.


#include "StatHandler.h"
#include "SaiyoraBuffLibrary.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"

const FGameplayTag UStatHandler::GenericStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat")), false);

void UStatHandler::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
		}
		if (IsValid(BuffHandler))
		{
			//Setup a restriction on buffs that modify stats that this component has determined can not be modified.
			FBuffEventCondition BuffStatModCondition;
			BuffStatModCondition.BindUFunction(this, FName(TEXT("CheckBuffStatMods")));
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
	ReplicatedStats.StatHandler = this;
	OwnerOnlyStats.StatHandler = this;
	//Only initialize stats on the server. Replication callbacks will initialize on the clients.
	if (GetOwnerRole() == ROLE_Authority)
	{
		//Initialize each editor-defined stat.
		for (TTuple<FGameplayTag, FStatInfo>& InitInfo : StatInfo)
		{
			//If the tag provided isn't a Stat.Tag, don't initialize.
			if (!InitInfo.Key.IsValid() || !InitInfo.Key.MatchesTag(GenericStatTag))
			{
				continue;
			}
			
			//Clamp the editor-defined default value above global minimum of zero, or the custom minimum, if one exists.
			InitInfo.Value.DefaultValue = FMath::Max(InitInfo.Value.DefaultValue, InitInfo.Value.bCappedLow ? InitInfo.Value.MinClamp : 0.0f);
			//Clamp the editor-defined default value below the custom maximum, if one exists.
			if (InitInfo.Value.bCappedHigh)
			{
				InitInfo.Value.DefaultValue = FMath::Min(InitInfo.Value.DefaultValue, InitInfo.Value.MaxClamp);
			}
		
			//Set the stat and mark it as initialized on the server so we can access it with getters and actually use it.
			InitInfo.Value.CurrentValue = InitInfo.Value.DefaultValue;
			InitInfo.Value.bInitialized = true;

			//Set up replication for stats that need to be accessed on clients. Client prediction requires some for owners, and movement requires some for all clients.
			switch (InitInfo.Value.ReplicationNeeds)
			{
			case EReplicationNeeds::NotReplicated :
				break;
			case EReplicationNeeds::AllClients :
				{
					FReplicatedStat ReplicatedStat;
					ReplicatedStat.StatTag = InitInfo.Key;
					ReplicatedStat.Value = InitInfo.Value.CurrentValue;
					ReplicatedStats.MarkItemDirty(ReplicatedStats.Items.Add_GetRef(ReplicatedStat));
				}
				break;
			case EReplicationNeeds::OwnerOnly :
				{
					FReplicatedStat ReplicatedStat;
					ReplicatedStat.StatTag = InitInfo.Key;
					ReplicatedStat.Value = InitInfo.Value.CurrentValue;
					OwnerOnlyStats.MarkItemDirty(OwnerOnlyStats.Items.Add_GetRef(ReplicatedStat));
				}
				break;
			default :
				break;
			}

			//Mark the stat as unmodifiable if needed, for checks against incoming buffs.
			if (!InitInfo.Value.bModifiable)
			{
				UnmodifiableStats.AddTag(InitInfo.Key);
			}
		}
	}
}


void UStatHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStatHandler, ReplicatedStats);
	DOREPLIFETIME_CONDITION(UStatHandler, OwnerOnlyStats, COND_OwnerOnly);
}

bool UStatHandler::GetStatValid(FGameplayTag const& StatTag) const
{
	return StatTag.IsValid() && StatInfo.Contains(StatTag);
}

float UStatHandler::GetStatValue(FGameplayTag const& StatTag) const
{
	FStatInfo const* Info = GetStatInfoConstPtr(StatTag);
	if (Info && Info->bInitialized)
	{
		return Info->CurrentValue;
	}
	return -1.0f;
}

void UStatHandler::AddStatModifier(FStatModifier const& Modifier)
{
	if (!IsValid(Modifier.Modifier.Source) || Modifier.Modifier.ModType == EModifierType::Invalid)
	{
		return;
	}
	FStatInfo* Info = GetStatInfoPtr(Modifier.StatTag);
	if (Info && Info->bInitialized && Info->bModifiable)
	{
		int32 const PreviousMods = Info->StatModifiers.Num();
		Info->StatModifiers.AddUnique(Modifier);
		if (Info->StatModifiers.Num() != PreviousMods)
		{
			RecalculateStat(Modifier.StatTag);
		}
	}
}

void UStatHandler::AddStatModifiers(TArray<FStatModifier> const& Modifiers)
{
	TSet<FGameplayTag> ModifiedStats;
	for (FStatModifier const& StatMod : Modifiers)
	{
		if (!IsValid(StatMod.Modifier.Source) || StatMod.Modifier.ModType == EModifierType::Invalid)
		{
			continue;
		}
		FStatInfo* Info = GetStatInfoPtr(StatMod.StatTag);
		if (Info && Info->bInitialized && Info->bModifiable)
		{
			int32 const PreviousMods = Info->StatModifiers.Num();
			Info->StatModifiers.AddUnique(StatMod);
			if (Info->StatModifiers.Num() != PreviousMods)
			{
				ModifiedStats.Add(StatMod.StatTag);
			}
		}
	}
	for (FGameplayTag const& StatTag : ModifiedStats)
	{
		RecalculateStat(StatTag);
	}
}

void UStatHandler::RemoveStatModifier(FStatModifier const& Modifier)
{
	if (!IsValid(Modifier.Modifier.Source) || Modifier.Modifier.ModType == EModifierType::Invalid)
	{
		return;
	}
	FStatInfo* Info = GetStatInfoPtr(Modifier.StatTag);
	if (Info && Info->bInitialized && Info->bModifiable)
	{
		if (Info->StatModifiers.RemoveSingleSwap(Modifier) != 0)
		{
			RecalculateStat(Modifier.StatTag);
		}
	}
}

void UStatHandler::RemoveStatModifiers(UBuff* Source, TSet<FGameplayTag> const& AffectedStats)
{
	if (!IsValid(Source))
	{
		return;
	}
	TSet<FGameplayTag> ModifiedStats;
	for (FGameplayTag const& StatTag : AffectedStats)
	{
		if (StatTag.MatchesTag(GenericStatTag))
		{
			FStatInfo* Info = GetStatInfoPtr(StatTag);
			if (Info && Info->bInitialized && Info->bModifiable)
			{
				bool bSuccessfullyRemoved = false;
				for (int32 Index = Info->StatModifiers.Num()-1; Index >= 0; --Index)
				{
					if (Info->StatModifiers[Index].Modifier.Source == Source)
					{
						int32 const PreviousNum = Info->StatModifiers.Num();
						Info->StatModifiers.RemoveAt(Index);
						if (!bSuccessfullyRemoved && PreviousNum != Info->StatModifiers.Num())
						{
							bSuccessfullyRemoved = true;
						}
					}
				}
				if (bSuccessfullyRemoved)
				{
					ModifiedStats.Add(StatTag);
				}
			}
		}
	}
	for (FGameplayTag const& StatTag : ModifiedStats)
	{
		RecalculateStat(StatTag);
	}
}

void UStatHandler::UpdateStackingModifiers(UBuff* Source, TSet<FGameplayTag> const& AffectedStats)
{
	if (!IsValid(Source))
	{
		return;
	}
	for (FGameplayTag const& StatTag : AffectedStats)
	{
		if (StatTag.MatchesTag(GenericStatTag))
		{
			FStatInfo* Info = GetStatInfoPtr(StatTag);
			if (Info && Info->bInitialized && Info->bModifiable)
			{
				bool bSuccessfullyStacked = false;
				for (FStatModifier& StatMod : Info->StatModifiers)
				{
					if (StatMod.Modifier.Source == Source)
					{
						bSuccessfullyStacked = true;
					}
				}
				if (bSuccessfullyStacked)
				{
					RecalculateStat(StatTag);
				}
			}
		}
	}
}

void UStatHandler::SubscribeToStatChanged(FGameplayTag const& StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	FStatInfo* Info = GetStatInfoPtr(StatTag);
	if (Info && Info->bInitialized)
	{
		Info->OnStatChanged.AddUnique(Callback);
	}
}

void UStatHandler::UnsubscribeFromStatChanged(FGameplayTag const& StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	FStatInfo* Info = GetStatInfoPtr(StatTag);
	if (Info && Info->bInitialized)
	{
		Info->OnStatChanged.Remove(Callback);
	}
}

void UStatHandler::NotifyOfReplicatedStat(FGameplayTag const& StatTag, float const NewValue)
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag))
	{
		return;
	}
	FStatInfo* Info = GetStatInfoPtr(StatTag);
	if (!Info)
	{
		Info = &StatInfo.Add(StatTag);
		Info->bInitialized = true;
	}
	float const OldValue = Info->CurrentValue;
	if (OldValue != NewValue)
	{
		Info->CurrentValue = NewValue;
		Info->OnStatChanged.Broadcast(StatTag, NewValue);
	}
}

void UStatHandler::RecalculateStat(FGameplayTag const& StatTag)
{
	//Check the stat is initialized and is in our stat map.
	FStatInfo* Info = GetStatInfoPtr(StatTag);
	if (!Info || !Info->bInitialized)
	{
		return;
	}

	//Store old value for delegate usage and comparison to see if stat was actually recalculated.
	float const OldValue = Info->CurrentValue;
	float NewValue = Info->DefaultValue;

	//Modify based on current modifiers, if applicable.
	if (Info->bModifiable)
	{
		TArray<FCombatModifier> Mods;
		for (FStatModifier const& StatMod : Info->StatModifiers)
		{
			Mods.Add(StatMod.GetModifier());
		}
		NewValue = FCombatModifier::CombineModifiers(Mods, NewValue);
	}
	//Check to see if the stat value actually changed.
	if (NewValue != OldValue)
	{
		//Actually set the stat.
		Info->CurrentValue = NewValue;
		
		//Change replicated stat value if needed.
		switch (Info->ReplicationNeeds)
		{
			case EReplicationNeeds::NotReplicated :
				break;
			case EReplicationNeeds::AllClients :
				ReplicatedStats.UpdateStatValue(StatTag, NewValue);
				break;
			case EReplicationNeeds::OwnerOnly :
				OwnerOnlyStats.UpdateStatValue(StatTag, NewValue);
				break;
			default :
				break;
		}
		//Broadcast the change.
		Info->OnStatChanged.Broadcast(StatTag, NewValue);
	}
}

FStatInfo* UStatHandler::GetStatInfoPtr(FGameplayTag const& StatTag)
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag))
	{
		return nullptr;
	}
	return StatInfo.Find(StatTag);
}

FStatInfo const* UStatHandler::GetStatInfoConstPtr(FGameplayTag const& StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag))
	{
		return nullptr;
	}
	return StatInfo.Find(StatTag);
}

bool UStatHandler::CheckBuffStatMods(FBuffApplyEvent const& BuffEvent) const
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
