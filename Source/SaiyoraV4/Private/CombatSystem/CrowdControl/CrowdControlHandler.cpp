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
	StunStatus.CrowdControlType = StunTag();
	IncapStatus.CrowdControlType = IncapacitateTag();
	RootStatus.CrowdControlType = RootTag();
	SilenceStatus.CrowdControlType = SilenceTag();
	DisarmStatus.CrowdControlType = DisarmTag();
}

void UCrowdControlHandler::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Crowd Control Handler."), *GetOwner()->GetActorLabel());
	if (GetOwnerRole() == ROLE_Authority)
	{
		BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
		checkf(IsValid(BuffHandler), TEXT("%s does not have a valid Buff Handler, which CC Handler depends on."), *GetOwner()->GetActorLabel());
		OnBuffApplied.BindDynamic(this, &UCrowdControlHandler::CheckAppliedBuffForCc);
		BuffHandler->SubscribeToIncomingBuff(OnBuffApplied);
		OnBuffRemoved.BindDynamic(this, &UCrowdControlHandler::CheckRemovedBuffForCc);
		BuffHandler->SubscribeToIncomingBuffRemove(OnBuffRemoved);
		DamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
		if (IsValid(DamageHandler))
		{
			OnDamageTaken.BindDynamic(this, &UCrowdControlHandler::RemoveIncapacitatesOnDamageTaken);
			DamageHandler->SubscribeToIncomingDamage(OnDamageTaken);
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
#pragma region Subscriptions

void UCrowdControlHandler::SubscribeToCrowdControlChanged(FCrowdControlCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnCrowdControlChanged.AddUnique(Callback);
}

void UCrowdControlHandler::UnsubscribeFromCrowdControlChanged(FCrowdControlCallback const& Callback)
{
	if (!Callback.IsBound())
	{
		return;
	}
	OnCrowdControlChanged.Remove(Callback);
}

#pragma endregion
#pragma region Status

bool UCrowdControlHandler::IsCrowdControlActive(FGameplayTag const CcTag) const
{
	FCrowdControlStatus const* CcStruct = GetCcStructConst(CcTag);
	if (CcStruct)
	{
		return CcStruct->bActive;
	}
	return false;
}

void UCrowdControlHandler::GetActiveCrowdControls(FGameplayTagContainer& OutCcs) const
{
	if (StunStatus.bActive)
	{
		OutCcs.AddTag(StunTag());
	}
	if (IncapStatus.bActive)
	{
		OutCcs.AddTag(IncapacitateTag());
	}
	if (RootStatus.bActive)
	{
		OutCcs.AddTag(RootTag());
	}
	if (SilenceStatus.bActive)
	{
		OutCcs.AddTag(SilenceTag());
	}
	if (DisarmStatus.bActive)
	{
		OutCcs.AddTag(DisarmTag());
	}
}

FCrowdControlStatus* UCrowdControlHandler::GetCcStruct(FGameplayTag const CcTag)
{
	if (!CcTag.IsValid() || !CcTag.MatchesTag(GenericCrowdControlTag()) || CcTag.MatchesTagExact(GenericCrowdControlTag()))
	{
		return nullptr;
	}
	if (CcTag.MatchesTagExact(StunTag()))
	{
		return &StunStatus;
	}
	if (CcTag.MatchesTagExact(IncapacitateTag()))
	{
		return &IncapStatus;
	}
	if (CcTag.MatchesTagExact(RootTag()))
	{
		return &RootStatus;
	}
	if (CcTag.MatchesTagExact(SilenceTag()))
	{
		return &SilenceStatus;
	}
	if (CcTag.MatchesTagExact(DisarmTag()))
	{
		return &DisarmStatus;
	}
	return nullptr;
}

FCrowdControlStatus const* UCrowdControlHandler::GetCcStructConst(FGameplayTag const CcTag) const
{
	if (!CcTag.IsValid() || !CcTag.MatchesTag(GenericCrowdControlTag()) || CcTag.MatchesTagExact(GenericCrowdControlTag()))
	{
		return nullptr;
	}
	if (CcTag.MatchesTagExact(StunTag()))
	{
		return &StunStatus;
	}
	if (CcTag.MatchesTagExact(IncapacitateTag()))
	{
		return &IncapStatus;
	}
	if (CcTag.MatchesTagExact(RootTag()))
	{
		return &RootStatus;
	}
	if (CcTag.MatchesTagExact(SilenceTag()))
	{
		return &SilenceStatus;
	}
	if (CcTag.MatchesTagExact(DisarmTag()))
	{
		return &DisarmStatus;
	}
	return nullptr;
}

void UCrowdControlHandler::CheckAppliedBuffForCc(FBuffApplyEvent const& BuffEvent)
{
	if (BuffEvent.ActionTaken == EBuffApplyAction::Failed || BuffEvent.ActionTaken == EBuffApplyAction::Stacked || !IsValid(BuffEvent.AffectedBuff))
	{
		return;
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.AffectedBuff->GetBuffTags(BuffTags);
	if (BuffEvent.ActionTaken == EBuffApplyAction::NewBuff)
	{
		for (FGameplayTag const& Tag : BuffTags)
		{
			if (Tag.IsValid() && Tag.MatchesTag(GenericCrowdControlTag()) && !Tag.MatchesTagExact(GenericCrowdControlTag()))
			{
				FCrowdControlStatus* CcStruct = GetCcStruct(Tag);
				if (CcStruct)
				{
					FCrowdControlStatus const Previous = *CcStruct;
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
		for (FGameplayTag const& Tag : BuffTags)
		{
			if (Tag.IsValid() && Tag.MatchesTag(GenericCrowdControlTag()) && !Tag.MatchesTagExact(GenericCrowdControlTag()))
			{
				FCrowdControlStatus* CcStruct = GetCcStruct(Tag);
				if (CcStruct)
				{
					FCrowdControlStatus const Previous = *CcStruct;
					if (CcStruct->RefreshBuff(BuffEvent.AffectedBuff))
					{
						OnCrowdControlChanged.Broadcast(Previous, *CcStruct);
					}
				}
			}
		}
	}
}

void UCrowdControlHandler::CheckRemovedBuffForCc(FBuffRemoveEvent const& RemoveEvent)
{
	if (!RemoveEvent.Result || !IsValid(RemoveEvent.RemovedBuff))
	{
		return;
	}
	FGameplayTagContainer BuffTags;
	RemoveEvent.RemovedBuff->GetBuffTags(BuffTags);
	for (FGameplayTag const& Tag : BuffTags)
	{
		if (Tag.IsValid() && Tag.MatchesTag(GenericCrowdControlTag()) && !Tag.MatchesTagExact(GenericCrowdControlTag()))
		{
			FCrowdControlStatus* CcStruct = GetCcStruct(Tag);
			if (CcStruct)
			{
				FCrowdControlStatus const Previous = *CcStruct;
				if (CcStruct->RemoveBuff(RemoveEvent.RemovedBuff))
				{
					OnCrowdControlChanged.Broadcast(Previous, *CcStruct);
				}
			}
		}
	}
}

void UCrowdControlHandler::OnRep_StunStatus(FCrowdControlStatus const& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, StunStatus);
}

void UCrowdControlHandler::OnRep_IncapStatus(FCrowdControlStatus const& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, IncapStatus);
}

void UCrowdControlHandler::OnRep_RootStatus(FCrowdControlStatus const& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, RootStatus);
}

void UCrowdControlHandler::OnRep_SilenceStatus(FCrowdControlStatus const& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, SilenceStatus);
}

void UCrowdControlHandler::OnRep_DisarmStatus(FCrowdControlStatus const& Previous)
{
	OnCrowdControlChanged.Broadcast(Previous, DisarmStatus);
}

void UCrowdControlHandler::RemoveIncapacitatesOnDamageTaken(FDamagingEvent const& DamageEvent)
{
	//Currently any non-zero and non-DoT damage will break all incaps.
	if (DamageEvent.Result.AmountDealt > 0.0f && DamageEvent.Info.HitStyle != EDamageHitStyle::Chronic)
	{
		for(UBuff* Cc : IncapStatus.Sources)
		{
			BuffHandler->RemoveBuff(Cc, EBuffExpireReason::Dispel);
		}
	}
}

#pragma endregion 