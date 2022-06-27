#include "CrowdControlHandler.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"
#include "BuffHandler.h"
#include "DamageHandler.h"

#pragma region Setup

UCrowdControlHandler::UCrowdControlHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UCrowdControlHandler::InitializeComponent()
{
	Super::InitializeComponent();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Crowd Control Handler."));
	BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
	DamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
	StunStatus.CrowdControlType = FSaiyoraCombatTags::Get().Cc_Stun;
	IncapStatus.CrowdControlType = FSaiyoraCombatTags::Get().Cc_Incapacitate;
	RootStatus.CrowdControlType = FSaiyoraCombatTags::Get().Cc_Root;
	SilenceStatus.CrowdControlType = FSaiyoraCombatTags::Get().Cc_Silence;
	DisarmStatus.CrowdControlType = FSaiyoraCombatTags::Get().Cc_Disarm;
}

void UCrowdControlHandler::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		checkf(IsValid(BuffHandler), TEXT("Owner does not have a valid Buff Handler, which CC Handler depends on."));
		BuffHandler->OnIncomingBuffApplied.AddDynamic(this, &UCrowdControlHandler::CheckAppliedBuffForCc);
		BuffHandler->OnIncomingBuffRemoved.AddDynamic(this, &UCrowdControlHandler::CheckRemovedBuffForCc);
		if (IsValid(DamageHandler))
		{
			DamageHandler->OnIncomingHealthEvent.AddDynamic(this, &UCrowdControlHandler::RemoveIncapacitatesOnDamageTaken);
		}
	}
}

void UCrowdControlHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCrowdControlHandler, StunStatus);
	DOREPLIFETIME(UCrowdControlHandler, IncapStatus);
	DOREPLIFETIME(UCrowdControlHandler, RootStatus);
	DOREPLIFETIME(UCrowdControlHandler, SilenceStatus);
	DOREPLIFETIME(UCrowdControlHandler, DisarmStatus);
}

#pragma endregion
#pragma region Status

bool UCrowdControlHandler::IsCrowdControlActive(const FGameplayTag CcTag) const
{
	if (FCrowdControlStatus const* CcStruct = GetCcStructConst(CcTag))
	{
		return CcStruct->bActive;
	}
	return false;
}

void UCrowdControlHandler::GetActiveCrowdControls(FGameplayTagContainer& OutCcs) const
{
	OutCcs.Reset();
	if (StunStatus.bActive)
	{
		OutCcs.AddTag(FSaiyoraCombatTags::Get().Cc_Stun);
	}
	if (IncapStatus.bActive)
	{
		OutCcs.AddTag(FSaiyoraCombatTags::Get().Cc_Incapacitate);
	}
	if (RootStatus.bActive)
	{
		OutCcs.AddTag(FSaiyoraCombatTags::Get().Cc_Root);
	}
	if (SilenceStatus.bActive)
	{
		OutCcs.AddTag(FSaiyoraCombatTags::Get().Cc_Silence);
	}
	if (DisarmStatus.bActive)
	{
		OutCcs.AddTag(FSaiyoraCombatTags::Get().Cc_Disarm);
	}
}

FCrowdControlStatus* UCrowdControlHandler::GetCcStruct(const FGameplayTag CcTag)
{
	if (!CcTag.IsValid() || !CcTag.MatchesTag(FSaiyoraCombatTags::Get().CrowdControl) || CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().CrowdControl))
	{
		return nullptr;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Stun))
	{
		return &StunStatus;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Incapacitate))
	{
		return &IncapStatus;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Root))
	{
		return &RootStatus;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Silence))
	{
		return &SilenceStatus;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Disarm))
	{
		return &DisarmStatus;
	}
	return nullptr;
}

FCrowdControlStatus const* UCrowdControlHandler::GetCcStructConst(const FGameplayTag CcTag) const
{
	if (!CcTag.IsValid() || !CcTag.MatchesTag(FSaiyoraCombatTags::Get().CrowdControl) || CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().CrowdControl))
	{
		return nullptr;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Stun))
	{
		return &StunStatus;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Incapacitate))
	{
		return &IncapStatus;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Root))
	{
		return &RootStatus;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Silence))
	{
		return &SilenceStatus;
	}
	if (CcTag.MatchesTagExact(FSaiyoraCombatTags::Get().Cc_Disarm))
	{
		return &DisarmStatus;
	}
	return nullptr;
}

void UCrowdControlHandler::CheckAppliedBuffForCc(const FBuffApplyEvent& BuffEvent)
{
	if (BuffEvent.ActionTaken == EBuffApplyAction::Failed || BuffEvent.ActionTaken == EBuffApplyAction::Stacked || !IsValid(BuffEvent.AffectedBuff))
	{
		return;
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.AffectedBuff->GetBuffTags(BuffTags);
	if (BuffEvent.ActionTaken == EBuffApplyAction::NewBuff)
	{
		for (const FGameplayTag Tag : BuffTags)
		{
			if (Tag.IsValid() && Tag.MatchesTag(FSaiyoraCombatTags::Get().CrowdControl) && !Tag.MatchesTagExact(FSaiyoraCombatTags::Get().CrowdControl))
			{
				if (FCrowdControlStatus* CcStruct = GetCcStruct(Tag))
				{
					const FCrowdControlStatus Previous = *CcStruct;
					if (CcStruct->AddNewBuff(BuffEvent.AffectedBuff))
					{
						OnCrowdControlChanged.Broadcast(Previous, *CcStruct);
					}
				}
			}
		}
	}
	else if (BuffEvent.ActionTaken == EBuffApplyAction::Refreshed || BuffEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
	{
		for (const FGameplayTag Tag : BuffTags)
		{
			if (Tag.IsValid() && Tag.MatchesTag(FSaiyoraCombatTags::Get().CrowdControl) && !Tag.MatchesTagExact(FSaiyoraCombatTags::Get().CrowdControl))
			{
				if (FCrowdControlStatus* CcStruct = GetCcStruct(Tag))
				{
					const FCrowdControlStatus Previous = *CcStruct;
					if (CcStruct->RefreshBuff(BuffEvent.AffectedBuff))
					{
						OnCrowdControlChanged.Broadcast(Previous, *CcStruct);
					}
				}
			}
		}
	}
}

void UCrowdControlHandler::CheckRemovedBuffForCc(const FBuffRemoveEvent& RemoveEvent)
{
	if (!RemoveEvent.Result || !IsValid(RemoveEvent.RemovedBuff))
	{
		return;
	}
	FGameplayTagContainer BuffTags;
	RemoveEvent.RemovedBuff->GetBuffTags(BuffTags);
	for (const FGameplayTag Tag : BuffTags)
	{
		if (Tag.IsValid() && Tag.MatchesTag(FSaiyoraCombatTags::Get().CrowdControl) && !Tag.MatchesTagExact(FSaiyoraCombatTags::Get().CrowdControl))
		{
			if (FCrowdControlStatus* CcStruct = GetCcStruct(Tag))
			{
				const FCrowdControlStatus Previous = *CcStruct;
				if (CcStruct->RemoveBuff(RemoveEvent.RemovedBuff))
				{
					OnCrowdControlChanged.Broadcast(Previous, *CcStruct);
				}
			}
		}
	}
}

void UCrowdControlHandler::OnRep_StunStatus(const FCrowdControlStatus& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, StunStatus);
}

void UCrowdControlHandler::OnRep_IncapStatus(const FCrowdControlStatus& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, IncapStatus);
}

void UCrowdControlHandler::OnRep_RootStatus(const FCrowdControlStatus& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, RootStatus);
}

void UCrowdControlHandler::OnRep_SilenceStatus(const FCrowdControlStatus& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, SilenceStatus);
}

void UCrowdControlHandler::OnRep_DisarmStatus(const FCrowdControlStatus& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, DisarmStatus);
}

void UCrowdControlHandler::RemoveIncapacitatesOnDamageTaken(const FHealthEvent& HealthEvent)
{
	//Currently any non-zero and non-DoT damage will break all incaps.
	if (HealthEvent.Info.EventType == EHealthEventType::Damage && HealthEvent.Result.AppliedValue > 0.0f && HealthEvent.Info.HitStyle != EEventHitStyle::Chronic)
	{
		for(UBuff* Cc : IncapStatus.Sources)
		{
			BuffHandler->RemoveBuff(Cc, EBuffExpireReason::Dispel);
		}
	}
}

#pragma endregion 