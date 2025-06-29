#include "PartyFrame.h"
#include "BuffHandler.h"
#include "Buff.h"
#include "BuffIcon.h"
#include "CombatStatusComponent.h"
#include "DamageHandler.h"
#include "PlayerHUD.h"
#include "HealthBar.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlayerCharacter.h"
#include "GameFramework/PlayerState.h"

void UPartyFrame::InitFrame(UPlayerHUD* OwningHUD, ASaiyoraPlayerCharacter* Player)
{
	if (!IsValid(Player) || !IsValid(OwningHUD))
	{
		return;
	}
	PlayerCharacter = Player;
	OwnerHUD = OwningHUD;
	
	HealthBar->InitHealthBar(OwningHUD, Player);
	
	FString PlayerName = Player->GetPlayerState()->GetPlayerName();
	if (PlayerName.Len() > PlayerNameCharLimit)
	{
		PlayerName = PlayerName.Left(PlayerNameCharLimit);
		PlayerName.Append("...");
	}
	NameText->SetText(FText::FromString(PlayerName));

	if (UBuffHandler* BuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(Player))
	{
		BuffHandler->OnIncomingBuffApplied.AddDynamic(this, &UPartyFrame::OnBuffApplied);
		BuffHandler->OnIncomingBuffRemoved.AddDynamic(this, &UPartyFrame::OnBuffRemoved);
		TArray<UBuff*> ActiveBuffs;
		BuffHandler->GetBuffs(ActiveBuffs);
		for (UBuff* Buff : ActiveBuffs)
		{
			OnBuffApplied(Buff);
		}
	}
	//When players have a pending res while dead, a little icon appears on their party frame.
	DamageHandlerRef = ISaiyoraCombatInterface::Execute_GetDamageHandler(Player);
	if (IsValid(DamageHandlerRef))
	{
		DamageHandlerRef->OnPendingResAdded.AddDynamic(this, &UPartyFrame::OnPendingResAdded);
		DamageHandlerRef->OnPendingResRemoved.AddDynamic(this, &UPartyFrame::OnPendingResRemoved);
		DamageHandlerRef->OnLifeStatusChanged.AddDynamic(this, &UPartyFrame::OnLifeStatusChanged);
		UpdatePendingResVisibility();
	}
	//For players other than the local player, we'll play an animation when a plane swap happens, to indicate whether the player is xplane from the local player.
	if (!Player->IsLocallyControlled())
	{
		DynamicXPlaneOverlayMat = UMaterialInstanceDynamic::Create(XPlaneOverlayMaterial, this);
		if (IsValid(DynamicXPlaneOverlayMat))
		{
			XPlaneOverlayImage->SetBrushFromMaterial(DynamicXPlaneOverlayMat);
			DynamicXPlaneOverlayMat->SetScalarParameterValue("Alpha", 0.0f);
			XPlaneOverlayImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		const ASaiyoraPlayerCharacter* LocalPlayer = USaiyoraCombatLibrary::GetLocalSaiyoraPlayer(GetWorld());
		if (IsValid(LocalPlayer))
		{
			LocalPlayerCombatComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(LocalPlayer);
			AssignedPlayerCombatComp = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Player);
			if (IsValid(AssignedPlayerCombatComp) && IsValid(LocalPlayerCombatComp))
			{
				
				//If we have a valid combat status component for both the local player and the assigned player, bind to both of their plane swap delegates.
				AssignedPlayerCombatComp->OnPlaneSwapped.AddDynamic(this, &UPartyFrame::OnPlaneSwap);
				LocalPlayerCombatComp->OnPlaneSwapped.AddDynamic(this, &UPartyFrame::OnPlaneSwap);
				OnPlaneSwap(ESaiyoraPlane::Neither, AssignedPlayerCombatComp->GetCurrentPlane(), nullptr);
			}
		}
	}
	else
	{
		XPlaneOverlayImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPartyFrame::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsValid(XPlaneOverlayImage) && IsValid(DynamicXPlaneOverlayMat))
	{
		if ((bIsXPlaneFromLocalPlayer && XPlaneAlpha < 1.0f) || (!bIsXPlaneFromLocalPlayer && XPlaneAlpha > 0.0f))
		{
			XPlaneAlpha = FMath::Clamp(XPlaneAlpha + ((InDeltaTime / XPlaneAnimationLength) * (bIsXPlaneFromLocalPlayer ? 1.0f : -1.0f)), 0.0f, 1.0f);
			DynamicXPlaneOverlayMat->SetScalarParameterValue(FName("Alpha"), XPlaneAlpha);
		}
		const ESlateVisibility DesiredVisibility = XPlaneAlpha > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (XPlaneOverlayImage->GetVisibility() != DesiredVisibility)
		{
			XPlaneOverlayImage->SetVisibility(DesiredVisibility);
		}
	}
}

void UPartyFrame::OnBuffApplied(UBuff* Buff)
{
	if (!IsValid(Buff) || !IsValid(BuffIconClass))
	{
		return;
	}
	if (!Buff->ShouldDisplayOnPartyFrame())
	{
		return;
	}
	UBuffIcon* NewIcon = CreateWidget<UBuffIcon>(this, BuffIconClass);
	if (!IsValid(NewIcon))
	{
		return;
	}
	BuffIcons.Add(Buff, NewIcon);
	BuffBox->AddChildToHorizontalBox(NewIcon);
	NewIcon->Init(Buff);
}

void UPartyFrame::OnBuffRemoved(const FBuffRemoveEvent& RemoveEvent)
{
	UBuffIcon* Icon = BuffIcons.FindRef(RemoveEvent.RemovedBuff);
	if (IsValid(Icon))
	{
		Icon->Cleanup();
		Icon->RemoveFromParent();
		BuffIcons.Remove(RemoveEvent.RemovedBuff);
	}
}

void UPartyFrame::OnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source)
{
	if (!IsValid(LocalPlayerCombatComp) || !IsValid(AssignedPlayerCombatComp))
	{
		bIsXPlaneFromLocalPlayer = false;
		return;
	}
	bIsXPlaneFromLocalPlayer = UAbilityFunctionLibrary::IsXPlane(LocalPlayerCombatComp->GetCurrentPlane(), AssignedPlayerCombatComp->GetCurrentPlane());
}

void UPartyFrame::UpdatePendingResVisibility()
{
	if (!IsValid(DamageHandlerRef))
	{
		PendingResIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	if (DamageHandlerRef->GetLifeStatus() != ELifeStatus::Dead)
	{
		PendingResIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	PendingResIcon->SetVisibility(DamageHandlerRef->GetPendingResurrections().Num() > 0
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}