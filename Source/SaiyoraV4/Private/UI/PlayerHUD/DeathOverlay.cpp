#include "DeathOverlay.h"

#include "Buff.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraPlayerCharacter.h"

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
	OwnerDamageHandler->OnPendingResAdded.AddDynamic(this, &UDeathOverlay::OnPendingResUpdated);
	OwnerDamageHandler->OnPendingResRemoved.AddDynamic(this, &UDeathOverlay::OnPendingResUpdated);

	AcceptPendingResButton->OnClicked.AddDynamic(this, &UDeathOverlay::AcceptPendingRes);
	DeclinePendingResButton->OnClicked.AddDynamic(this, &UDeathOverlay::DeclinePendingRes);
	DefaultRespawnButton->OnClicked.AddDynamic(this, &UDeathOverlay::DefaultRespawn);
	
	bDoingInit = true;
	OnPendingResUpdated(FPendingResurrection());
	OnLifeStatusChanged(OwningChar, ELifeStatus::Invalid, OwnerDamageHandler->GetLifeStatus());
	bDoingInit = false;

	AddToViewport();
}

void UDeathOverlay::OnLifeStatusChanged(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (NewStatus == ELifeStatus::Dead)
	{
		if (!bWasDisplayed || bDoingInit)
		{
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			bWasDisplayed = true;
		}
	}
	else
	{
		if (bWasDisplayed || bDoingInit)
		{
			SetVisibility(ESlateVisibility::Collapsed);
			bWasDisplayed = false;
		}
	}
}

void UDeathOverlay::OnPendingResUpdated(const FPendingResurrection& PendingRes)
{
	const TArray<FPendingResurrection>& PendingResurrections = OwnerDamageHandler->GetPendingResurrections();
	if (PendingResurrections.Num() > 0)
	{
		RespawnResurrectSwitcher->SetActiveWidget(PendingResOverlay);
		const FPendingResurrection& MostRecentRes = PendingResurrections.Last();
		if (IsValid(MostRecentRes.Icon))
		{
			PendingResIcon->SetBrushFromTexture(MostRecentRes.Icon);
		}
		PendingResID = MostRecentRes.ResID;
	}
	else
	{
		RespawnResurrectSwitcher->SetActiveWidget(DefaultRespawnOverlay);
		PendingResID = -1;
	}
}

void UDeathOverlay::AcceptPendingRes()
{
	OwnerDamageHandler->AcceptResurrection(PendingResID);
}

void UDeathOverlay::DeclinePendingRes()
{
	OwnerDamageHandler->DeclineResurrection(PendingResID);
}

void UDeathOverlay::DefaultRespawn()
{
	OwnerDamageHandler->RequestDefaultRespawn();
}