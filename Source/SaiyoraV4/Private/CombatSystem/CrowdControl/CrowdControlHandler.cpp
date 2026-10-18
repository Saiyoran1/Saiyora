#include "CrowdControlHandler.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "UnrealNetwork.h"
#include "BuffHandler.h"
#include "CrowdControlBuffFunctions.h"
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

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	checkf(GetOwner()->Implements<USaiyoraCombatInterface>(), TEXT("Owner does not implement combat interface, but has Crowd Control Handler."));
	BuffHandlerRef = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwner());
	checkf(IsValid(BuffHandlerRef), TEXT("Owner does not have a valid Buff Handler, which CC Handler depends on."));
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(GetOwner());

	//Init the array of crowd control statuses
	if (GetOwner()->HasAuthority())
	{
		for (const ECrowdControl Cc : TEnumRange<ECrowdControl>())
		{
			FCrowdControlStatus& Status = CurrentCrowdControls.Items.AddDefaulted_GetRef();
			Status.CrowdControlType = Cc;
			Status.bDefaultImmuned = DefaultCrowdControlImmunities.Contains(Cc);
			CurrentCrowdControls.MarkItemDirty(Status);
		}
	}
}

void UCrowdControlHandler::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (IsValid(DamageHandlerRef))
		{
			DamageHandlerRef->OnIncomingHealthEvent.AddDynamic(this, &UCrowdControlHandler::RemoveIncapacitatesOnDamageTaken);
		}
	}
	CurrentCrowdControls.Init(this);
}

void UCrowdControlHandler::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCrowdControlHandler, CurrentCrowdControls);
}

#pragma endregion
#pragma region Status

void UCrowdControlHandler::AddCrowdControl(UBuff* Source, const ECrowdControl CcType)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	if (FCrowdControlStatus* Status = GetCcStruct(CcType))
	{
		if (Status->AddNewBuff(Source))
		{
			OnCrowdControlChanged.Broadcast(*Status);
			CurrentCrowdControls.MarkItemDirty(*Status);
		}
	}
}

void UCrowdControlHandler::RefreshCrowdControl(UBuff* Source)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	for (FCrowdControlStatus& Cc : CurrentCrowdControls.Items)
	{
		if (Cc.RefreshBuff(Source))
		{
			OnCrowdControlChanged.Broadcast(Cc);
			CurrentCrowdControls.MarkItemDirty(Cc);
		}
	}
}

void UCrowdControlHandler::RemoveCrowdControl(UBuff* Source)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	for (FCrowdControlStatus& Cc : CurrentCrowdControls.Items)
	{
		if (Cc.RemoveBuff(Source))
		{
			OnCrowdControlChanged.Broadcast(Cc);
			CurrentCrowdControls.MarkItemDirty(Cc);
		}
	}
}

bool UCrowdControlHandler::IsCrowdControlActive(const ECrowdControl CcType) const
{
	const FCrowdControlStatus* Status = GetCcStructConst(CcType);
	return Status != nullptr && Status->bActive;
}

FCrowdControlStatus* UCrowdControlHandler::GetCcStruct(const ECrowdControl CcType)
{
	for (FCrowdControlStatus& Cc : CurrentCrowdControls.Items)
	{
		if (Cc.CrowdControlType == CcType)
		{
			return &Cc;
		}
	}
	return nullptr;
}

FCrowdControlStatus const* UCrowdControlHandler::GetCcStructConst(const ECrowdControl CcType) const
{
	for (const FCrowdControlStatus& Cc : CurrentCrowdControls.Items)
	{
		if (Cc.CrowdControlType == CcType)
		{
			return &Cc;
		}
	}
	return nullptr;
}

void UCrowdControlHandler::RemoveIncapacitatesOnDamageTaken(const FHealthEvent& HealthEvent)
{
	if (!IsValid(BuffHandlerRef))
	{
		return;
	}
	//Currently any non-zero and non-DoT damage will break all incaps.
	if (HealthEvent.Info.EventType == EHealthEventType::Damage
		&& HealthEvent.Result.AppliedValue > 0.0f
		&& HealthEvent.Info.HitStyle != EEventHitStyle::Chronic)
	{
		DispelCcsOfType(ECrowdControl::Incapacitate);
	}
}

#pragma endregion
#pragma region Immunities

void UCrowdControlHandler::AddImmunities(UBuff* Source, const TSet<ECrowdControl>& Immunities)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	if (!IsValid(Source))
	{
		return;
	}
	TArray<ECrowdControl> BecameImmune;
	for (const ECrowdControl Immunity : Immunities)
	{
		if (FCrowdControlStatus* Status = GetCcStruct(Immunity))
		{
			const bool bNoImmunity = Status->Immunities.Num() == 0 && !Status->bDefaultImmuned;
			Status->Immunities.Add(Source);
			if (bNoImmunity)
			{
				BecameImmune.Add(Immunity);
			}
		}
	}
	//For any CC type that we just became immune to, we need to dispel any active CCs of that type.
	for (const ECrowdControl NewImmunity : BecameImmune)
	{
		DispelCcsOfType(NewImmunity);
	}
}

void UCrowdControlHandler::RemoveImmunities(UBuff* Source)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	if (!IsValid(Source))
	{
		return;
	}
	for (FCrowdControlStatus& Cc : CurrentCrowdControls.Items)
	{
		if (Cc.Immunities.Contains(Source))
		{
			Cc.Immunities.Remove(Source);
		}
	}
}

bool UCrowdControlHandler::IsImmuneToCc(const ECrowdControl CcType) const
{
	const FCrowdControlStatus* Status = GetCcStructConst(CcType);
	return Status != nullptr && (Status->bDefaultImmuned || Status->Immunities.Num() > 0);
}

void UCrowdControlHandler::DispelCcsOfType(const ECrowdControl CcType)
{
	if (const FCrowdControlStatus* Status = GetCcStructConst(CcType))
	{
		//Copy the buffs array, since we'll be removing those buffs while iterating.
		TSet<UBuff*> Buffs = Status->Sources;
		for (UBuff* IncapBuff : Buffs)
		{
			//We break Ccs by removing all buffs that are applying those CCs
			BuffHandlerRef->RemoveBuff(IncapBuff, EBuffExpireReason::Dispel);
		}
	}
}

#pragma endregion 