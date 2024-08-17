#include "PlayerHUD/CastBar.h"
#include "AbilityComponent.h"
#include "Border.h"
#include "Image.h"
#include "ProgressBar.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraUIDataAsset.h"
#include "TextBlock.h"
#include "UIFunctionLibrary.h"

void UCastBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (!IsValid(AbilityComponentRef))
	{
		return;
	}
	//If we're fading out after a completed cast, don't update progress or anything else.
	if (GetWorld()->GetTimerManager().IsTimerActive(FadeTimerHandle))
	{
		const float Opacity = FMath::Clamp(GetWorld()->GetTimerManager().GetTimerRemaining(FadeTimerHandle) / GetWorld()->GetTimerManager().GetTimerRate(FadeTimerHandle), 0.0f, 1.0f);
		SetRenderOpacity(Opacity);
		return;
	}
	//During a cast, update progress and duration text.
	if (IsValid(CastProgress))
	{
		const float Percent = AbilityComponentRef->GetCurrentCastLength() <= 0.0f ? 1.0f :
			1.0f - FMath::Clamp(AbilityComponentRef->GetCastTimeRemaining() / AbilityComponentRef->GetCurrentCastLength(), 0.0f, 1.0f);
		CastProgress->SetPercent(Percent);
	}
	if (bDisplayDuration && IsValid(DurationText))
	{
		DurationText->SetText(FText::FromString(UUIFunctionLibrary::GetTimeDisplayString(AbilityComponentRef->GetCastTimeRemaining())));
	}
}

void UCastBar::InitCastBar(AActor* OwnerActor, const bool bDisplayDurationText)
{
	if (!IsValid(OwnerActor) || !OwnerActor->Implements<USaiyoraCombatInterface>())
	{
		return;
	}
	AbilityComponentRef = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwnerActor);
	if (!IsValid(AbilityComponentRef))
	{
		return;
	}
	UIDataAsset = UUIFunctionLibrary::GetUIDataAsset(GetWorld());
	bDisplayDuration = bDisplayDurationText;
	if (IsValid(DurationText))
	{
		if (bDisplayDuration)
		{
			DurationText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			DurationText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (AbilityComponentRef->IsCasting())
	{
		OnCastStateChanged(FCastingState(), AbilityComponentRef->GetCurrentCastingState());
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	AbilityComponentRef->OnCastStateChanged.AddDynamic(this, &UCastBar::OnCastStateChanged);
	AbilityComponentRef->OnAbilityInterrupted.AddDynamic(this, &UCastBar::OnCastInterrupted);
	AbilityComponentRef->OnAbilityCancelled.AddDynamic(this, &UCastBar::OnCastCancelled);
	AbilityComponentRef->OnAbilityTick.AddDynamic(this, &UCastBar::OnCastTick);
}

void UCastBar::OnCastStateChanged(const FCastingState& PreviousState, const FCastingState& NewState)
{
	//This only handles showing the cast bar when a new cast starts. Hiding the cast bar is handled by Cancel/Interrupt/Tick callbacks to make sure we show those things.
	if (NewState.bIsCasting && IsValid(NewState.CurrentCast))
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
		SetRenderOpacity(1.0f);
		if (GetWorld()->GetTimerManager().IsTimerActive(FadeTimerHandle))
		{
			GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
		}
		if (IsValid(CastText))
		{
			CastText->SetText(FText::FromString(NewState.CurrentCast->GetAbilityName().ToString()));
		}
		if (IsValid(CastIcon))
		{
			CastIcon->SetBrushFromTexture(NewState.CurrentCast->GetAbilityIcon());
		}
		if (IsValid(CastProgress))
		{
			CastProgress->SetFillColorAndOpacity(UUIFunctionLibrary::GetElementalSchoolColor(GetWorld(), NewState.CurrentCast->GetAbilitySchool()));
		}
		if (IsValid(InterruptibleBorder))
		{
			if (NewState.bInterruptible)
			{
				InterruptibleBorder->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				InterruptibleBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		if (bDisplayDuration && IsValid(DurationText))
		{
			DurationText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void UCastBar::OnCastInterrupted(const FInterruptEvent& Event)
{
	if (IsValid(CastProgress))
	{
		const float Percent = FMath::Clamp((Event.InterruptTime - Event.InterruptedCastStart) / (Event.InterruptedCastEnd - Event.InterruptedCastStart), 0.0f, 1.0f);
		CastProgress->SetPercent(Percent);
		if (IsValid(UIDataAsset))
		{
			CastProgress->SetFillColorAndOpacity(UIDataAsset->FailureProgressColor);
		}
	}
	if (IsValid(CastText))
	{
		CastText->SetText(FText::FromString("INTERRUPTED"));
	}
	if (IsValid(DurationText))
	{
		DurationText->SetVisibility(ESlateVisibility::Hidden);
	}
	StartFade(1.0f);
}

void UCastBar::OnCastCancelled(const FCancelEvent& Event)
{
	if (IsValid(CastProgress))
	{
		const float Percent = FMath::Clamp((Event.CancelTime - Event.CancelledCastStart) / (Event.CancelledCastEnd - Event.CancelledCastStart), 0.0f, 1.0f);
		CastProgress->SetPercent(Percent);
		if (IsValid(UIDataAsset))
		{
			CastProgress->SetFillColorAndOpacity(UIDataAsset->FailureProgressColor);
		}
	}
	if (IsValid(CastText))
	{
		CastText->SetText(FText::FromString("CANCELLED"));
	}
	if (IsValid(DurationText))
	{
		DurationText->SetVisibility(ESlateVisibility::Hidden);
	}
	StartFade(0.5f);
}

void UCastBar::OnCastTick(const FAbilityEvent& Event)
{
	if (IsValid(Event.Ability)
		&& Event.Ability->GetCastType() == EAbilityCastType::Channel
		&& Event.Tick == Event.Ability->GetNonInitialTicks())
	{
		if (IsValid(CastProgress))
		{
			if (IsValid(UIDataAsset))
			{
				CastProgress->SetFillColorAndOpacity(UIDataAsset->SuccessProgressColor);
			}
			CastProgress->SetPercent(1.0f);
		}
		if (IsValid(DurationText))
		{
			DurationText->SetVisibility(ESlateVisibility::Hidden);
		}
		StartFade(0.2f);
	}
}

void UCastBar::StartFade(const float Duration)
{
	GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, this, &UCastBar::OnFadeTimeFinished, Duration);
}

void UCastBar::OnFadeTimeFinished()
{
	GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::Collapsed);
}