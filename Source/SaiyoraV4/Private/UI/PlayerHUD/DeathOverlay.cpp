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

	if (IsValid(ResurrectionLocationIndicatorClass))
	{
		ResurrectionLocationIndicator = GetWorld()->SpawnActor(ResurrectionLocationIndicatorClass);
		if (IsValid(ResurrectionLocationIndicator))
		{
			ResurrectionLocationIndicator->SetActorHiddenInGame(true);
			ResurrectionLocationIndicator->SetActorEnableCollision(false);
		}
	}
	
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
	//Set the location indicator actor to be hidden when the player is alive.
	if (!bShouldDisplay && IsValid(ResurrectionLocationIndicator))
	{
		ResurrectionLocationIndicator->SetActorHiddenInGame(true);
	}
	//When entering dead state, clear the flag from any previously accepted resurrection.
	//Immediately update the resurrection window.
	if (PreviousStatus != ELifeStatus::Dead && NewStatus == ELifeStatus::Dead)
	{
		SetWaitingForServer(false);
		UpdatePendingResurrection();
	}
}

void UDeathOverlay::UpdatePendingResurrection()
{
	bQueuedNeedsUpdate = false;
	const TArray<FPendingResurrection>& PendingResurrections = OwnerDamageHandler->GetPendingResurrections();
	//Refresh the "AlreadyDeclinedResIDs" list so that it doesn't keep resurrections no longer pending.
	const TArray<int> AlreadyDeclined = AlreadyDeclinedResIDs;
	AlreadyDeclinedResIDs.Empty();
	for (const FPendingResurrection& Resurrection : PendingResurrections)
	{
		if (AlreadyDeclined.Contains(Resurrection.ResID))
		{
			AlreadyDeclinedResIDs.Add(Resurrection.ResID);
		}
	}
	//If we've already accepted a res and are just waiting on replication, don't change anything until the server responds.
	if (bAwaitingServerResDecision)
	{
		return;
	}
	//Find the most recent pending resurrection we haven't already made a decision on. That's our new displayed res.
	bool bFoundRes = false;
	for (int i = PendingResurrections.Num() - 1; i >= 0; i--)
	{
		const FPendingResurrection& PendingResurrection = PendingResurrections[i];
		if (AlreadyDeclinedResIDs.Contains(PendingResurrection.ResID))
		{
			continue;
		}
		bFoundRes = true;
		PendingResID = PendingResurrection.ResID;
		RezBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
		if (IsValid(ResurrectionLocationIndicator))
		{
			ResurrectionLocationIndicator->SetActorLocation(PendingResurrection.ResLocation);
			ResurrectionLocationIndicator->SetActorHiddenInGame(OwnerDamageHandler->GetLifeStatus() != ELifeStatus::Dead);
		}
		break;
	}
	//If there's no pending resurrection we haven't already made a decision on, hide the pending resurrection window.
	if (!bFoundRes)
	{
		RezBorder->SetVisibility(ESlateVisibility::Collapsed);
		PendingResID = -1;
		if (IsValid(ResurrectionLocationIndicator))
		{
			ResurrectionLocationIndicator->SetActorHiddenInGame(true);
		}
	}
}

void UDeathOverlay::SetWaitingForServer(const bool bWaiting)
{
	bAwaitingServerResDecision = bWaiting;
	bQueuedNeedsUpdate = true;
	DefaultRespawnButton->SetIsEnabled(!bWaiting);
	AcceptPendingResButton->SetIsEnabled(!bWaiting);
	DeclinePendingResButton->SetIsEnabled(!bWaiting);
}

void UDeathOverlay::AcceptPendingRes()
{
	if (bAwaitingServerResDecision || PendingResID < 0 || AlreadyDeclinedResIDs.Contains(PendingResID))
	{
		return;
	}
	SetWaitingForServer(true);
	OwnerDamageHandler->MakeResurrectionDecision(PendingResID, true);
}

void UDeathOverlay::DeclinePendingRes()
{
	if (bAwaitingServerResDecision || PendingResID < 0 || AlreadyDeclinedResIDs.Contains(PendingResID))
	{
		return;
	}
	AlreadyDeclinedResIDs.Add(PendingResID);
	bQueuedNeedsUpdate = true;
	OwnerDamageHandler->MakeResurrectionDecision(PendingResID, false);
}

void UDeathOverlay::DefaultRespawn()
{
	if (bAwaitingServerResDecision)
	{
		return;
	}
	SetWaitingForServer(true);
	OwnerDamageHandler->RequestDefaultRespawn();
}