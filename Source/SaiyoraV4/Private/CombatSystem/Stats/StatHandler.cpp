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
			if (!NewStat->IsModifiable())
			{
				UnmodifiableStats.AddTag(NewStat->GetStatTag());
			}
		}
	}
}

void UStatHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool UStatHandler::GetStatValid(FGameplayTag const StatTag) const
{
	if (!StatTag.IsValid() || !StatTag.MatchesTag(GenericStatTag()))
	{
		return false;
	}
	UCombatStat* Stat = Stats.FindRef(StatTag);
	return IsValid(Stat) && Stat->IsInitialized();
}

float UStatHandler::GetStatValue(FGameplayTag const StatTag) const
{
	UCombatStat* FoundStat = Stats.FindRef(StatTag);
	if (IsValid(FoundStat))
	{
		return FoundStat->GetValue();
	}
	return -1.0f;
}

int32 UStatHandler::AddStatModifier(FGameplayTag const StatTag, FCombatModifier const& Modifier)
{
	if (GetOwnerRole() != ROLE_Authority || Modifier.GetModType() == EModifierType::Invalid || !StatTag.MatchesTag(GenericStatTag()))
	{
		return -1;
	}
	UCombatStat* Stat = Stats.FindRef(StatTag);
	if (!IsValid(Stat))
	{
		return -1;
	}
	return Stat->AddModifier(Modifier);
}

void UStatHandler::RemoveStatModifier(FGameplayTag const StatTag, int32 const ModifierID)
{
	if (GetOwnerRole() != ROLE_Authority || !StatTag.MatchesTag(GenericStatTag()) || ModifierID == -1)
	{
		return;
	}
	UCombatStat* Stat = Stats.FindRef(StatTag);
	if (!IsValid(Stat))
	{
		return;
	}
	Stat->RemoveModifier(ModifierID);
}

void UStatHandler::SubscribeToStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	UCombatStat* Stat = Stats.FindRef(StatTag);
	if (!IsValid(Stat))
	{
		return;
	}
	Stat->SubscribeToStatChanged(Callback);
}

void UStatHandler::UnsubscribeFromStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	UCombatStat* Stat = Stats.FindRef(StatTag);
	if (!IsValid(Stat))
	{
		return;
	}
	Stat->UnsubscribeFromStatChanged(Callback);
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
