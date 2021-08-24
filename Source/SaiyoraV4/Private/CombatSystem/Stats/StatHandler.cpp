// Fill out your copyright notice in the Description page of Project Settings.


#include "StatHandler.h"
#include "SaiyoraBuffLibrary.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatStat.h"
#include "SaiyoraCombatInterface.h"

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
			UCombatStat* NewStat = Stats.Add(InitInfo->StatTag, NewObject<UCombatStat>(GetOwner()));
			if(IsValid(NewStat))
			{
				NewStat->Init(*InitInfo, this);
				if (InitInfo->bShouldReplicate)
				{
					
				}
			}
			/*
			FCombatStat& NewStat = Stats.Add(InitInfo->StatTag);
			NewStat.StatTag = InitInfo->StatTag;
			NewStat.bShouldReplicate = InitInfo->bShouldReplicate;
			NewStat.StatValue = FCombatFloatValue(InitInfo->DefaultValue, InitInfo->bModifiable, true, InitInfo->MinClamp, InitInfo->bCappedHigh, InitInfo->MaxClamp);
			NewStat.Setup();
			if (NewStat.bShouldReplicate)
			{
				FReplicatedStat ReplicatedStat;
				ReplicatedStat.StatTag = NewStat.StatTag;
				ReplicatedStat.Value = NewStat.StatValue.GetValue();
				ReplicatedStats.MarkItemDirty(ReplicatedStats.Items.Add_GetRef(ReplicatedStat));
			}
			if (!NewStat.StatValue.IsModifiable())
			{
				UnmodifiableStats.AddTag(NewStat.StatTag);
			}*/
		}
	}
}


void UStatHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStatHandler, ReplicatedStats);
}

bool UStatHandler::GetStatValid(FGameplayTag const StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()))
	{
		return false;
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		return Stats.Contains(StatTag);
	}
	for (FReplicatedStat const& RepStat : ReplicatedStats.Items)
	{
		if (RepStat.StatTag == StatTag)
		{
			return true;
		}
	}
	return false;
}

float UStatHandler::GetStatValue(FGameplayTag const StatTag) const
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		FCombatStat const* Stat = Stats.Find(StatTag);
		if (Stat)
		{
			return Stat->StatValue.GetValue();
		}
	}
	else
	{
		for (FReplicatedStat const& RepStat : ReplicatedStats.Items)
		{
			if (RepStat.StatTag == StatTag)
			{
				return RepStat.Value;
			}
		}
	}
	return -1.0f;
}

int32 UStatHandler::AddStatModifier(FGameplayTag const StatTag, FCombatModifier const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || Modifier.GetModType() == EModifierType::Invalid || !StatTag.MatchesTag(GenericStatTag()))
	{
		return -1;
	}
	FCombatStat* Stat = Stats.Find(StatTag);
	if (!Stat)
	{
		return -1;
	}
	float const Previous = Stat->StatValue.GetValue();
	int32 const ModID = Stat->StatValue.AddModifier(Modifier);
	if (Stat->bShouldReplicate && Previous != Stat->StatValue.GetValue())
	{
		ReplicatedStats.UpdateStatValue(StatTag, Stat->StatValue.GetValue());
	}
	return ModID;
}

void UStatHandler::RemoveStatModifier(FGameplayTag const StatTag, int32 const ModifierID)
{
	if (GetOwnerRole() != ROLE_Authority || !StatTag.MatchesTag(GenericStatTag()) || ModifierID == -1)
	{
		return;
	}
	FCombatStat* Stat = Stats.Find(StatTag);
	if (!Stat)
	{
		return;
	}
	float const Previous = Stat->StatValue.GetValue();
	Stat->StatValue.RemoveModifier(ModifierID);
	if (Stat->bShouldReplicate && Previous != Stat->StatValue.GetValue())
	{
		ReplicatedStats.UpdateStatValue(StatTag, Stat->StatValue.GetValue());
	}
}

void UStatHandler::SubscribeToStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		FCombatStat* Stat = Stats.Find(StatTag);
		if (!Stat)
		{
			return;
		}
		Stat->OnStatChanged.AddUnique(Callback);
		return;
	}
	for (FReplicatedStat& RepStat : ReplicatedStats.Items)
	{
		if (RepStat.StatTag == StatTag)
		{
			RepStat.ClientOnChanged.AddUnique(Callback);
			return;
		}
	}
}

void UStatHandler::UnsubscribeFromStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		FCombatStat* Stat = Stats.Find(StatTag);
		if (!Stat)
		{
			return;
		}
		Stat->OnStatChanged.Remove(Callback);
		return;
	}
	for (FReplicatedStat& RepStat : ReplicatedStats.Items)
	{
		if (RepStat.StatTag == StatTag)
		{
			RepStat.ClientOnChanged.Remove(Callback);
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
