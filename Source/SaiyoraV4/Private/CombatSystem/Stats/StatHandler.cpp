// Fill out your copyright notice in the Description page of Project Settings.


#include "StatHandler.h"
#include "SaiyoraBuffLibrary.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"

//const FGameplayTag UStatHandler::GenericStatTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Stat")), false);

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
	ReplicatedStats.StatHandler = this;
	OwnerOnlyStats.StatHandler = this;
	//Only initialize stats on the server. Replication callbacks will initialize on the clients.
	if (GetOwnerRole() == ROLE_Authority)
	{
		TArray<FStatInfo*> InitialStatArray;
		InitialStats->GetAllRows<FStatInfo>(nullptr, InitialStatArray);
		for (FStatInfo const* InitInfo : InitialStatArray)
		{
			if (!InitInfo || !InitInfo->StatTag.MatchesTag(GenericStatTag()))
			{
				continue;
			}
			FStatInfo& NewStat = StatInfo.Add(InitInfo->StatTag, *InitInfo);
			NewStat.DefaultValue = NewStat.bCappedHigh ?
				FMath::Min(FMath::Max(NewStat.DefaultValue, NewStat.bCappedLow ? NewStat.MinClamp : 0.0f), NewStat.MaxClamp)
				: FMath::Max(NewStat.DefaultValue, NewStat.bCappedLow ? NewStat.MinClamp : 0.0f);
			NewStat.CurrentValue = NewStat.DefaultValue;
			NewStat.bInitialized = true;
			switch (NewStat.ReplicationNeeds)
			{
				case EReplicationNeeds::NotReplicated :
					break;
				case EReplicationNeeds::AllClients :
					{
						FReplicatedStat ReplicatedStat;
						ReplicatedStat.StatTag = NewStat.StatTag;
						ReplicatedStat.Value = NewStat.CurrentValue;
						ReplicatedStats.MarkItemDirty(ReplicatedStats.Items.Add_GetRef(ReplicatedStat));
					}
					break;
				case EReplicationNeeds::OwnerOnly :
					{
						FReplicatedStat OwnerOnlyStat;
						OwnerOnlyStat.StatTag = NewStat.StatTag;
						OwnerOnlyStat.Value = NewStat.CurrentValue;
						OwnerOnlyStats.MarkItemDirty(OwnerOnlyStats.Items.Add_GetRef(OwnerOnlyStat));
					}
					break;
				default :
					break;
			}
			if (!NewStat.bModifiable)
			{
				UnmodifiableStats.AddTag(NewStat.StatTag);
			}
		}
		//Initialize each editor-defined stat.
		/*
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
		*/
	}
}


void UStatHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStatHandler, ReplicatedStats);
	DOREPLIFETIME_CONDITION(UStatHandler, OwnerOnlyStats, COND_OwnerOnly);
}

bool UStatHandler::GetStatValid(FGameplayTag const StatTag) const
{
	return StatTag.IsValid() && StatInfo.Contains(StatTag);
}

float UStatHandler::GetStatValue(FGameplayTag const StatTag) const
{
	FStatInfo const* Info = GetStatInfoConstPtr(StatTag);
	if (Info && Info->bInitialized)
	{
		return Info->CurrentValue;
	}
	return -1.0f;
}

int32 UStatHandler::AddStatModifier(FGameplayTag const StatTag, FCombatModifier const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || Modifier.ModType == EModifierType::Invalid || !StatTag.MatchesTag(GenericStatTag()))
	{
		return -1;
	}
	FStatInfo* Info = GetStatInfoPtr(StatTag);
	if (!Info || !Info->bInitialized || !Info->bModifiable)
	{
		return -1;
	}
	int32 const ModID = FCombatModifier::GetID();
	Info->StatModifiers.Add(ModID, Modifier);
	RecalculateStat(StatTag);
	return ModID;
}

void UStatHandler::AddStatModifiers(TMap<FGameplayTag, FCombatModifier> const& Modifiers, TMap<FGameplayTag, int32>& OutIDs)
{
	OutIDs.Empty();
	for (TTuple<FGameplayTag, FCombatModifier> const& Mod : Modifiers)
	{
		OutIDs.Add(Mod.Key, AddStatModifier(Mod.Key, Mod.Value));
	}
}

void UStatHandler::RemoveStatModifier(FGameplayTag const StatTag, int32 const ModifierID)
{
	if (GetOwnerRole() != ROLE_Authority || !StatTag.MatchesTag(GenericStatTag()) || ModifierID == -1)
	{
		return;
	}
	FStatInfo* Info = GetStatInfoPtr(StatTag);
	if (!Info || !Info->bInitialized || !Info->bModifiable)
	{
		return;
	}
	if (Info->StatModifiers.Remove(ModifierID) > 0)
	{
		RecalculateStat(StatTag);
	}
}

void UStatHandler::RemoveStatModifiers(TMap<FGameplayTag, int32> const& ModifierIDs)
{
	for (TTuple<FGameplayTag, int32> const& ModID : ModifierIDs)
	{
		RemoveStatModifier(ModID.Key, ModID.Value);
	}
}

void UStatHandler::UpdateStackingModifiers(TSet<FGameplayTag> const& AffectedStats)
{
	for (FGameplayTag const& StatTag : AffectedStats)
	{
		FStatInfo const* Info = GetStatInfoConstPtr(StatTag);
		if (Info && Info->bInitialized && Info->bModifiable)
		{
			RecalculateStat(StatTag);
		}
	}
}

void UStatHandler::SubscribeToStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
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

void UStatHandler::UnsubscribeFromStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
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
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()))
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
		TArray<FCombatModifier> StatMods;
		Info->StatModifiers.GenerateValueArray(StatMods);
		NewValue = FCombatModifier::ApplyModifiers(StatMods, NewValue);
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
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()))
	{
		return nullptr;
	}
	return StatInfo.Find(StatTag);
}

FStatInfo const* UStatHandler::GetStatInfoConstPtr(FGameplayTag const& StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()))
	{
		return nullptr;
	}
	return StatInfo.Find(StatTag);
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
