#include "DeathOverlay.h"

#include "Buff.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraDevSettings.h"
#include "SaiyoraPlayerCharacter.h"
#include "SaiyoraUIDataAsset.h"

void UDeathOverlay::Init(ASaiyoraPlayerCharacter* OwningChar)
{
	if (!IsValid(OwningChar))
	{
		return;
	}
	OwnerDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(OwningChar);
	if (!IsValid(OwnerDamageHandler))
	{
		return;
	}
	OwnerDamageHandler->OnLifeStatusChanged.AddDynamic(this, &UDeathOverlay::OnLifeStatusChanged);
	OwnerDamageHandler->OnPendingResListChanged.AddDynamic(this, &UDeathOverlay::OnResurrectionListChanged);

	AcceptPendingResButton->OnClicked.AddDynamic(this, &UDeathOverlay::AcceptPendingRes);
	DeclinePendingResButton->OnClicked.AddDynamic(this, &UDeathOverlay::DeclinePendingRes);
	DefaultRespawnButton->OnClicked.AddDynamic(this, &UDeathOverlay::DefaultRespawn);
	
	UpdatePendingResurrection();
	OnLifeStatusChanged(OwningChar, ELifeStatus::Invalid, OwnerDamageHandler->GetLifeStatus());

	AddToViewport();
}

void UDeathOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bQueuedNeedsUpdate)
	{
		UpdatePendingResurrection();
	}
}

void UDeathOverlay::OnLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	const bool bShouldDisplay = NewStatus == ELifeStatus::Dead;
	const bool bIsDisplayed = GetVisibility() != ESlateVisibility::Collapsed;
	if (bShouldDisplay != bIsDisplayed)
	{
		SetVisibility(bShouldDisplay ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	//When entering dead state, clear the flag from any previously accepted resurrection.
	//Immediately update the resurrection window.
	if (PreviousStatus != ELifeStatus::Dead && NewStatus == ELifeStatus::Dead)
	{
		bAcceptedRes = false;
		UpdatePendingResurrection();
	}
}

void UDeathOverlay::UpdatePendingResurrection()
{
	bQueuedNeedsUpdate = false;
	const TArray<FPendingResurrection>& PendingResurrections = OwnerDamageHandler->GetPendingResurrections();
	//Refresh the "AlreadyDecidedResIDs" list so that it doesn't keep resurrections no longer pending.
	const TArray<int> AlreadyDecided = AlreadyDecidedResIDs;
	AlreadyDecidedResIDs.Empty();
	for (const FPendingResurrection& Resurrection : PendingResurrections)
	{
		if (AlreadyDecided.Contains(Resurrection.ResID))
		{
			AlreadyDecidedResIDs.Add(Resurrection.ResID);
		}
	}
	//If we've already accepted a res and are just waiting on replication, don't show pending resurrections.
	if (bAcceptedRes)
	{
		return;
	}
	//Find the first pending resurrection we haven't already made a decision on. That's our new displayed res.
	bool bFoundRes = false;
	for (int i = PendingResurrections.Num() - 1; i >= 0; i--)
	{
		const FPendingResurrection& PendingResurrection = PendingResurrections[i];
		if (AlreadyDecidedResIDs.Contains(PendingResurrection.ResID))
		{
			continue;
		}
		bFoundRes = true;
		PendingResID = PendingResurrection.ResID;
		RespawnResurrectSwitcher->SetActiveWidget(PendingResOverlay);
		bool bFoundIcon = false;
		if (IsValid(PendingResurrection.BuffClass))
		{
			const UBuff* DefaultBuff = PendingResurrection.BuffClass.GetDefaultObject();
			if (IsValid(DefaultBuff) && IsValid(DefaultBuff->GetBuffIcon()))
			{
				PendingResIcon->SetBrushFromTexture(DefaultBuff->GetBuffIcon());
				bFoundIcon = true;
			}
		}
		if (!bFoundIcon)
		{
			if (IsValid(UI_DA->DefaultPendingResurrectionIcon))
			{
				PendingResIcon->SetBrushFromTexture(UI_DA->DefaultPendingResurrectionIcon);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("No valid default pending resurrection icon set in the UI Data Asset!"));
			}
		}
		break;
	}
	//If there's no pending resurrection we haven't already made a decision on, swap back to the window showing the release option.
	if (!bFoundRes)
	{
		RespawnResurrectSwitcher->SetActiveWidget(DefaultRespawnOverlay);
		PendingResID = -1;
	}
}

void UDeathOverlay::AcceptPendingRes()
{
	if (bAcceptedRes || PendingResID < 0 || AlreadyDecidedResIDs.Contains(PendingResID))
	{
		return;
	}
	AlreadyDecidedResIDs.Add(PendingResID);
	bAcceptedRes = true;
	bQueuedNeedsUpdate = true;
	OwnerDamageHandler->MakeResurrectionDecision(PendingResID, true);
}

void UDeathOverlay::DeclinePendingRes()
{
	if (bAcceptedRes || PendingResID < 0 || AlreadyDecidedResIDs.Contains(PendingResID))
	{
		return;
	}
	AlreadyDecidedResIDs.Add(PendingResID);
	bQueuedNeedsUpdate = true;
	OwnerDamageHandler->MakeResurrectionDecision(PendingResID, false);
}

void UDeathOverlay::DefaultRespawn()
{
	if (bAcceptedRes)
	{
		return;
	}
	bAcceptedRes = true;
	bQueuedNeedsUpdate = true;
	OwnerDamageHandler->RequestDefaultRespawn();
}