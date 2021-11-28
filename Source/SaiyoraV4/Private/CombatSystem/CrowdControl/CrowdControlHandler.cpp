#include "CrowdControlHandler.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"
#include "BuffHandler.h"
#include "DamageHandler.h"
#include "SaiyoraBuffLibrary.h"

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
	//Sub to buff handler to add CCs from buffs that are applied.
	//Restrict buffs with CC tags that match immunities.
	//Sub to damage handler to tell buff handler to remove all Incap buffs when damage is taken.
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (ECrowdControlType const CcType : DefaultCrowdControlImmunities)
		{
			//We don't call AddImmunity here because we probably shouldn't be trying to remove buffs in the first frame of the game, and the BuffHandler isn't valid anyway.
			CrowdControlImmunities.Add(CcType, 1);
		}
		if (GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
			DamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());
		}
		if (IsValid(BuffHandler))
		{
			BuffCcRestriction.BindDynamic(this, &UCrowdControlHandler::CheckBuffRestrictedByCcImmunity);
			BuffHandler->AddIncomingBuffRestriction(BuffCcRestriction);
			OnBuffApplied.BindDynamic(this, &UCrowdControlHandler::CheckAppliedBuffForCcOrImmunity);
			BuffHandler->SubscribeToIncomingBuff(OnBuffApplied);
			OnBuffRemoved.BindDynamic(this, &UCrowdControlHandler::CheckRemovedBuffForCcOrImmunity);
			BuffHandler->SubscribeToIncomingBuffRemove(OnBuffRemoved);
		}
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

void UCrowdControlHandler::AddImmunity(ECrowdControlType const CcType)
{
	if (CcType == ECrowdControlType::None)
	{
		return;
	}
	int32& CurrentNum = CrowdControlImmunities.FindOrAdd(CcType);
	int32 const PreviousNum = CurrentNum;
	CurrentNum++;
	if (PreviousNum == 0 && CurrentNum == 1)
	{
		PurgeCcOfType(CcType);
	}
}

void UCrowdControlHandler::RemoveImmunity(ECrowdControlType const CcType)
{
	if (CcType == ECrowdControlType::None)
	{
		return;
	}
	int32* CurrentNum = CrowdControlImmunities.Find(CcType);
	if (CurrentNum)
	{
		*CurrentNum -= 1;
		if (*CurrentNum <= 0)
		{
			CrowdControlImmunities.Remove(CcType);
		}
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
		//TODO: Check in BeginPlay that BuffHandler exists.
		BuffHandler->RemoveBuff(Buff, EBuffExpireReason::Dispel);
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

bool UCrowdControlHandler::CheckBuffRestrictedByCcImmunity(FBuffApplyEvent const& BuffEvent)
{
	if (!IsValid(BuffEvent.BuffClass))
	{
		return false;
	}
	FGameplayTagContainer BuffTags;
	BuffEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	for (TTuple<ECrowdControlType, int32> const& TagTuple : CrowdControlImmunities)
	{
		if (TagTuple.Value > 0 && BuffTags.HasTagExact(CcTypeToTag(TagTuple.Key)))
		{
			return true;
		}
	}
	return false;
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
		for (FGameplayTag const& Tag : BuffTags)
		{
			if (Tag.MatchesTag(GenericCrowdControlTag()))
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
			else if (Tag.MatchesTag(GenericCcImmunityTag()))
			{
				//TODO: There's a very rare but possible scenario where adding an immunity tag could remove the buff before all tags have been applied.
				//This could only be caused if a buff applied both a CC and immunity to that CC, OR if it immuned another buff who's removal removed this buff.
				//Only real fix for this would be to take out the removal part of AddImmunityTag, and only check in this function after all tags have been added whether we need to remove CCs.
				AddImmunity(CcImmunityToType(Tag));
			}
		}
	}
	else if (BuffEvent.ActionTaken == EBuffApplyAction::Refreshed || BuffEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
	{
		for (FGameplayTag const& Tag : BuffTags)
		{
			if (Tag.MatchesTag(GenericCrowdControlTag()))
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
		if (Tag.MatchesTag(GenericCrowdControlTag()))
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
		else if (Tag.MatchesTag(GenericCcImmunityTag()))
		{
			RemoveImmunity(CcImmunityToType(Tag));
		}
	}
}

void UCrowdControlHandler::RemoveIncapacitatesOnDamageTaken(FDamagingEvent const& DamageEvent)
{
	//TODO: Figure out exactly what qualifies for breaking incaps.
	//Currently any non-zero and non-DoT damage will break all incaps.
	if (DamageEvent.Result.AmountDealt > 0.0f && DamageEvent.Info.HitStyle != EDamageHitStyle::Chronic)
	{
		PurgeCcOfType(ECrowdControlType::Incapacitate);
	}
}