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
	StunStatus.CrowdControlType = ECrowdControlType::Stun;
	IncapStatus.CrowdControlType = ECrowdControlType::Incapacitate;
	RootStatus.CrowdControlType = ECrowdControlType::Root;
	SilenceStatus.CrowdControlType = ECrowdControlType::Silence;
	DisarmStatus.CrowdControlType = ECrowdControlType::Disarm;
}

void UCrowdControlHandler::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("%s does not implement combat interface, but has Crowd Control Handler."), *GetOwner()->GetActorLabel());
	if (GetOwnerRole() == ROLE_Authority)
	{
		BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
		checkf(IsValid(BuffHandler), TEXT("%s does not have a valid Buff Handler, which CC Handler depends on."), *GetOwner()->GetActorLabel());
		BuffCcRestriction.BindDynamic(this, &UCrowdControlHandler::CheckBuffRestrictedByCcImmunity);
		BuffHandler->AddIncomingBuffRestriction(BuffCcRestriction);
		OnBuffApplied.BindDynamic(this, &UCrowdControlHandler::CheckAppliedBuffForCcOrImmunity);
		BuffHandler->SubscribeToIncomingBuff(OnBuffApplied);
		OnBuffRemoved.BindDynamic(this, &UCrowdControlHandler::CheckRemovedBuffForCcOrImmunity);
		BuffHandler->SubscribeToIncomingBuffRemove(OnBuffRemoved);
		for (ECrowdControlType const CcType : DefaultCrowdControlImmunities)
		{
			CrowdControlImmunities.Add(CcType, 1);
		}
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
#pragma region Conversions

FGameplayTag UCrowdControlHandler::CcTypeToTag(ECrowdControlType const CcType)
{
	switch (CcType)
	{
	case ECrowdControlType::Stun :
		return StunTag();
	case ECrowdControlType::Incapacitate :
		return IncapacitateTag();
	case ECrowdControlType::Root :
		return RootTag();
	case ECrowdControlType::Silence :
		return SilenceTag();
	case ECrowdControlType::Disarm :
		return DisarmTag();
	default :
		return FGameplayTag();
	}
}

FGameplayTag UCrowdControlHandler::CcTypeToImmunity(ECrowdControlType const CcType)
{
	switch (CcType)
	{
		case ECrowdControlType::Stun :
			return StunImmunityTag();
		case ECrowdControlType::Incapacitate :
			return IncapImmunityTag();
		case ECrowdControlType::Root :
			return RootImmunityTag();
		case ECrowdControlType::Silence :
			return SilenceImmunityTag();
		case ECrowdControlType::Disarm :
			return DisarmImmunityTag();
		default :
			return FGameplayTag();
	}
}

ECrowdControlType UCrowdControlHandler::CcTagToType(FGameplayTag const& CcTag)
{
	if (!CcTag.IsValid() || !CcTag.MatchesTag(GenericCrowdControlTag()) || CcTag.MatchesTagExact(GenericCrowdControlTag()))
	{
		return ECrowdControlType::None;
	}
	if (CcTag.MatchesTagExact(StunTag()))
	{
		return ECrowdControlType::Stun;
	}
	if (CcTag.MatchesTagExact(IncapacitateTag()))
	{
		return ECrowdControlType::Incapacitate;
	}
	if (CcTag.MatchesTagExact(RootTag()))
	{
		return ECrowdControlType::Root;
	}
	if (CcTag.MatchesTagExact(SilenceTag()))
	{
		return ECrowdControlType::Silence;
	}
	if (CcTag.MatchesTagExact(DisarmTag()))
	{
		return ECrowdControlType::Disarm;
	}
	return ECrowdControlType::None;
}

ECrowdControlType UCrowdControlHandler::CcImmunityToType(FGameplayTag const& ImmunityTag)
{
	if (!ImmunityTag.IsValid() || !ImmunityTag.MatchesTag(GenericCcImmunityTag()) || ImmunityTag.MatchesTagExact(GenericCcImmunityTag()))
	{
		return ECrowdControlType::None;
	}
	if (ImmunityTag.MatchesTagExact(StunImmunityTag()))
	{
		return ECrowdControlType::Stun;
	}
	if (ImmunityTag.MatchesTagExact(IncapImmunityTag()))
	{
		return ECrowdControlType::Incapacitate;
	}
	if (ImmunityTag.MatchesTagExact(RootImmunityTag()))
	{
		return ECrowdControlType::Root;
	}
	if (ImmunityTag.MatchesTagExact(SilenceImmunityTag()))
	{
		return ECrowdControlType::Silence;
	}
	if (ImmunityTag.MatchesTagExact(DisarmImmunityTag()))
	{
		return ECrowdControlType::Disarm;
	}
	return ECrowdControlType::None;
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

bool UCrowdControlHandler::IsCrowdControlActive(ECrowdControlType const CcType) const
{
	FCrowdControlStatus const* CcStruct = GetCcStructConst(CcType);
	if (CcStruct)
	{
		return CcStruct->bActive;
	}
	return false;
}

FCrowdControlStatus UCrowdControlHandler::GetCrowdControlStatus(ECrowdControlType const CcType) const
{
	FCrowdControlStatus const* CcStruct = GetCcStructConst(CcType);
	if (CcStruct)
	{
		return *CcStruct;
	}
	return FCrowdControlStatus();
}

void UCrowdControlHandler::GetActiveCrowdControls(TSet<ECrowdControlType>& OutCcs) const
{
	if (StunStatus.bActive)
	{
		OutCcs.Add(ECrowdControlType::Stun);
	}
	if (IncapStatus.bActive)
	{
		OutCcs.Add(ECrowdControlType::Incapacitate);
	}
	if (RootStatus.bActive)
	{
		OutCcs.Add(ECrowdControlType::Root);
	}
	if (SilenceStatus.bActive)
	{
		OutCcs.Add(ECrowdControlType::Silence);
	}
	if (DisarmStatus.bActive)
	{
		OutCcs.Add(ECrowdControlType::Disarm);
	}
}

FCrowdControlStatus* UCrowdControlHandler::GetCcStruct(ECrowdControlType const CcType)
{
	switch (CcType)
	{
	case ECrowdControlType::Stun :
		return &StunStatus;
	case ECrowdControlType::Incapacitate :
		return &IncapStatus;
	case ECrowdControlType::Root :
		return &RootStatus;
	case ECrowdControlType::Silence :
		return &SilenceStatus;
	case ECrowdControlType::Disarm :
		return &DisarmStatus;
	default :
		return nullptr;
	}
}

FCrowdControlStatus const* UCrowdControlHandler::GetCcStructConst(ECrowdControlType const CcType) const
{
	switch (CcType)
	{
	case ECrowdControlType::Stun :
		return &StunStatus;
	case ECrowdControlType::Incapacitate :
		return &IncapStatus;
	case ECrowdControlType::Root :
		return &RootStatus;
	case ECrowdControlType::Silence :
		return &SilenceStatus;
	case ECrowdControlType::Disarm :
		return &DisarmStatus;
	default :
		return nullptr;
	}
}

void UCrowdControlHandler::PurgeCcOfType(ECrowdControlType const CcType)
{
	FCrowdControlStatus* CcStruct = GetCcStruct(CcType);
	if (!CcStruct)
	{
		return;
	}
	TArray<UBuff*> NonMutableBuffs = CcStruct->Sources;
	for (UBuff* Buff : NonMutableBuffs)
	{
		BuffHandler->RemoveBuff(Buff, EBuffExpireReason::Dispel);
	}
}

void UCrowdControlHandler::CheckAppliedBuffForCcOrImmunity(FBuffApplyEvent const& BuffEvent)
{
	if (BuffEvent.ActionTaken == EBuffApplyAction::Failed || BuffEvent.ActionTaken == EBuffApplyAction::Stacked || !IsValid(BuffEvent.AffectedBuff))
	{
		return;
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.AffectedBuff->GetBuffTags(BuffTags);
	if (BuffEvent.ActionTaken == EBuffApplyAction::NewBuff)
	{
		TArray<ECrowdControlType> NewImmunities;
		for (FGameplayTag const& Tag : BuffTags)
		{
			if (Tag.MatchesTag(GenericCrowdControlTag()) && !Tag.MatchesTagExact(GenericCrowdControlTag()))
			{
				FCrowdControlStatus* CcStruct = GetCcStruct(CcTagToType(Tag));
				if (CcStruct)
				{
					FCrowdControlStatus const Previous = *CcStruct;
					if (CcStruct->AddNewBuff(BuffEvent.AffectedBuff))
					{
						OnCrowdControlChanged.Broadcast(Previous, *CcStruct);
					}
				}
			}
			else if (Tag.MatchesTag(GenericCcImmunityTag()) && !Tag.MatchesTagExact(GenericCcImmunityTag()))
			{
				int32& CurrentNum = CrowdControlImmunities.FindOrAdd(CcImmunityToType(Tag));
				CurrentNum++;
				if (CurrentNum == 1)
				{
					NewImmunities.Add(CcImmunityToType(Tag));
				}
			}
		}
		if (NewImmunities.Num() > 0)
		{
			for (ECrowdControlType const CcType : NewImmunities)
			{
				PurgeCcOfType(CcType);
			}
		}
	}
	else if (BuffEvent.ActionTaken == EBuffApplyAction::Refreshed || BuffEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
	{
		for (FGameplayTag const& Tag : BuffTags)
		{
			if (Tag.MatchesTag(GenericCrowdControlTag()) && !Tag.MatchesTagExact(GenericCrowdControlTag()))
			{
				FCrowdControlStatus* CcStruct = GetCcStruct(CcTagToType(Tag));
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

void UCrowdControlHandler::CheckRemovedBuffForCcOrImmunity(FBuffRemoveEvent const& RemoveEvent)
{
	if (!RemoveEvent.Result || !IsValid(RemoveEvent.RemovedBuff))
	{
		return;
	}
	FGameplayTagContainer BuffTags;
	RemoveEvent.RemovedBuff->GetBuffTags(BuffTags);
	for (FGameplayTag const& Tag : BuffTags)
	{
		if (Tag.MatchesTag(GenericCrowdControlTag()) && !Tag.MatchesTagExact(GenericCrowdControlTag()))
		{
			FCrowdControlStatus* CcStruct = GetCcStruct(CcTagToType(Tag));
			if (CcStruct)
			{
				FCrowdControlStatus const Previous = *CcStruct;
				if (CcStruct->RemoveBuff(RemoveEvent.RemovedBuff))
				{
					OnCrowdControlChanged.Broadcast(Previous, *CcStruct);
				}
			}
		}
		else if (Tag.MatchesTag(GenericCcImmunityTag()) && !Tag.MatchesTagExact(GenericCcImmunityTag()))
		{
			int32* CurrentNum = CrowdControlImmunities.Find(CcImmunityToType(Tag));
			if (CurrentNum)
			{
				*CurrentNum -= 1;
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
		PurgeCcOfType(ECrowdControlType::Incapacitate);
	}
}

#pragma endregion 
#pragma region Immunity

bool UCrowdControlHandler::IsImmuneToCrowdControl(ECrowdControlType const CcType) const
{
	return CrowdControlImmunities.FindRef(CcType) > 0;
}

void UCrowdControlHandler::GetImmunedCrowdControls(TSet<ECrowdControlType>& OutImmunes) const
{
	for (TTuple<ECrowdControlType, int32> const& CcTuple : CrowdControlImmunities)
	{
		if (CcTuple.Value > 0)
		{
			OutImmunes.Add(CcTuple.Key);
		}
	}
}

bool UCrowdControlHandler::CheckBuffRestrictedByCcImmunity(FBuffApplyEvent const& BuffEvent)
{
	if (!IsValid(BuffEvent.BuffClass))
	{
		return false;
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	for (TTuple<ECrowdControlType, int32> const& Immunity : CrowdControlImmunities)
	{
		if (Immunity.Value > 0 && BuffTags.HasTagExact(CcTypeToTag(Immunity.Key)))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion 