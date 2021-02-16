// Fill out your copyright notice in the Description page of Project Settings.


#include "CrowdControlHandler.h"
#include "Buff.h"
#include "CrowdControl.h"
#include "UnrealNetwork.h"

const FGameplayTag UCrowdControlHandler::GenericCrowdControlTag = FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl")), false);

UCrowdControlHandler::UCrowdControlHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UCrowdControlHandler::InitializeComponent()
{
	ReplicatedCrowdControls.Handler = this;
	if (GetOwnerRole() == ROLE_Authority)
	{
		FCrowdControlRestriction ImmunityRestriction;
		ImmunityRestriction.BindUFunction(this, FName(TEXT("RestrictImmunedCrowdControls")));
		AddCrowdControlRestriction(ImmunityRestriction);
	}
}

void UCrowdControlHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCrowdControlHandler, ReplicatedCrowdControls);
}

FGameplayTagContainer UCrowdControlHandler::GetActiveCcTypes() const
{
	FGameplayTagContainer CCTags;
	for (TTuple<TSubclassOf<UCrowdControl>, UCrowdControl*> const CrowdControl : CrowdControls)
	{
		if (CrowdControl.Value->GetCurrentStatus().bActive)
		{
			CCTags.AddTag(CrowdControl.Value->GetCrowdControlTag());
		}
	}
	return CCTags;
}

UCrowdControl* UCrowdControlHandler::GetCrowdControlObject(TSubclassOf<UCrowdControl> const CrowdControlType)
{
	if (!IsValid(CrowdControlType))
	{
		return nullptr;
	}
	UCrowdControl* Control = CrowdControls.FindRef(CrowdControlType);
	if (!IsValid(Control))
	{
		Control = NewObject<UCrowdControl>(GetOwner(), CrowdControlType);
		Control->InitializeCrowdControl(this);
		CrowdControls.Add(CrowdControlType, Control);
	}
	return Control;
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

void UCrowdControlHandler::AddCrowdControlRestriction(FCrowdControlRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	if (CrowdControlRestrictions.AddUnique(Restriction) > 0)
	{
		RemoveNewlyRestrictedCrowdControls(Restriction);
	}
}

void UCrowdControlHandler::RemoveCrowdControlRestriction(FCrowdControlRestriction const& Restriction)
{
	if (GetOwnerRole() != ROLE_Authority || !Restriction.IsBound())
	{
		return;
	}
	CrowdControlRestrictions.RemoveSingleSwap(Restriction);
}

bool UCrowdControlHandler::RestrictImmunedCrowdControls(UBuff* Source,
                                                        TSubclassOf<UCrowdControl> CrowdControlType)
{
	if (!IsValid(CrowdControlType))
	{
		return false;
	}
	UCrowdControl* DefaultObject = CrowdControlType->GetDefaultObject<UCrowdControl>();
	if (!IsValid(DefaultObject))
	{
		return false;
	}
	return CrowdControlImmunities.HasTag(DefaultObject->GetCrowdControlTag());
}

void UCrowdControlHandler::RemoveNewlyRestrictedCrowdControls(FCrowdControlRestriction const& NewRestriction)
{
	for (TTuple<TSubclassOf<UCrowdControl>, UCrowdControl*> CrowdControl : CrowdControls)
	{
		if (IsValid(CrowdControl.Value))
		{
			CrowdControl.Value->RemoveNewlyRestrictedCrowdControls(NewRestriction);
		}
	}
}

bool UCrowdControlHandler::CheckCrowdControlRestricted(UBuff* Source, TSubclassOf<UCrowdControl> const CrowdControlType)
{
	for (FCrowdControlRestriction const& Restriction : CrowdControlRestrictions)
	{
		if (Restriction.Execute(Source, CrowdControlType))
		{
			return true;
		}
	}
	return false;
}

void UCrowdControlHandler::ApplyCrowdControl(UBuff* Source, TSubclassOf<UCrowdControl> const CrowdControlType)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(CrowdControlType))
	{
		return;
	}
	if (CheckCrowdControlRestricted(Source, CrowdControlType))
	{
		return;
	}
	UCrowdControl* CrowdControl = GetCrowdControlObject(CrowdControlType);
	FCrowdControlStatus const PreviousStatus = CrowdControl->GetCurrentStatus();
	FCrowdControlStatus const NewStatus = CrowdControl->AddCrowdControl(Source);
	if (PreviousStatus.bActive != NewStatus.bActive || PreviousStatus.DominantBuff != NewStatus.DominantBuff || PreviousStatus.EndTime != NewStatus.EndTime)
	{
		OnCrowdControlChanged.Broadcast(PreviousStatus, NewStatus);
		ReplicatedCrowdControls.UpdateArray(NewStatus);
	}
}

void UCrowdControlHandler::RemoveCrowdControl(UBuff* Source, TSubclassOf<UCrowdControl> const CrowdControlType)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(Source) || !IsValid(CrowdControlType))
	{
		return;
	}
	UCrowdControl* CrowdControl = GetCrowdControlObject(CrowdControlType);
	FCrowdControlStatus const PreviousStatus = CrowdControl->GetCurrentStatus();
	FCrowdControlStatus const NewStatus = CrowdControl->RemoveCrowdControl(Source);
	if (PreviousStatus.bActive != NewStatus.bActive || PreviousStatus.DominantBuff != NewStatus.DominantBuff || PreviousStatus.EndTime != NewStatus.EndTime)
	{
		OnCrowdControlChanged.Broadcast(PreviousStatus, NewStatus);
		ReplicatedCrowdControls.UpdateArray(NewStatus);
	}
}

void UCrowdControlHandler::UpdateCrowdControlStatus(TSubclassOf<UCrowdControl> const CrowdControlType,
	UBuff* RefreshedBuff)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(RefreshedBuff) || !IsValid(CrowdControlType))
	{
		return;
	}
	UCrowdControl* CrowdControl = GetCrowdControlObject(CrowdControlType);
	FCrowdControlStatus const PreviousStatus = CrowdControl->GetCurrentStatus();
	FCrowdControlStatus const NewStatus = CrowdControl->UpdateCrowdControl(RefreshedBuff);
	if (PreviousStatus.bActive != NewStatus.bActive || PreviousStatus.DominantBuff != NewStatus.DominantBuff || PreviousStatus.EndTime != NewStatus.EndTime)
	{
		OnCrowdControlChanged.Broadcast(PreviousStatus, NewStatus);
		ReplicatedCrowdControls.UpdateArray(NewStatus);
	}
}

void UCrowdControlHandler::UpdateCrowdControlFromReplication(FReplicatedCrowdControl const& ReplicatedCrowdControl)
{
	UCrowdControl* CrowdControl = GetCrowdControlObject(ReplicatedCrowdControl.CrowdControlClass);
	if (!IsValid(CrowdControl))
	{
		return;
	}
	FCrowdControlStatus const PreviousStatus = CrowdControl->GetCurrentStatus();
	CrowdControl->UpdateCrowdControlFromReplication(ReplicatedCrowdControl.bActive, ReplicatedCrowdControl.DominantBuff, ReplicatedCrowdControl.EndTime);
	FCrowdControlStatus const NewStatus = CrowdControl->GetCurrentStatus();
	if (PreviousStatus.bActive != NewStatus.bActive || PreviousStatus.DominantBuff != NewStatus.DominantBuff || PreviousStatus.EndTime != NewStatus.EndTime)
	{
		OnCrowdControlChanged.Broadcast(PreviousStatus, NewStatus);
	}
}

void UCrowdControlHandler::RemoveCrowdControlFromReplication(TSubclassOf<UCrowdControl> const CrowdControlClass)
{
	if (!IsValid(CrowdControlClass))
	{
		return;	
	}
	UCrowdControl* CrowdControl = CrowdControls.FindRef(CrowdControlClass);
	if (!IsValid(CrowdControl))
	{
		return;
	}
	FCrowdControlStatus const PreviousStatus = CrowdControl->GetCurrentStatus();
	CrowdControl->UpdateCrowdControlFromReplication(false, nullptr, 0.0f);
	FCrowdControlStatus const NewStatus = CrowdControl->GetCurrentStatus();
	if (PreviousStatus.bActive != NewStatus.bActive || PreviousStatus.DominantBuff != NewStatus.DominantBuff || PreviousStatus.EndTime != NewStatus.EndTime)
	{
		OnCrowdControlChanged.Broadcast(PreviousStatus, NewStatus);
	}
}
