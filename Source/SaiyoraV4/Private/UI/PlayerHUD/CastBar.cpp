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
	if (GetVisibility() == ESlateVisibility::Hidden)
	{
		return;
	}
	//During a cast, update progress and duration text.
	if (IsValid(CastProgress))
	{
		const float Percent = AbilityComponentRef->GetCurrentCastLength() <= 0.0f ? 1.0f :
			1.0f - FMath::Clamp(AbilityComponentRef->GetCastTimeRemaining() / AbilityComponentRef->GetCurrentCastLength(), 0.0f, 1.0f);
		CastProgress->SetPercent(Percent);
	}
	if (IsValid(DurationText))
	{
		DurationText->SetText(FText::FromString(UUIFunctionLibrary::GetTimeDisplayString(AbilityComponentRef->GetCastTimeRemaining())));
	}
}

void UCastBar::InitCastBar(AActor* OwnerActor)
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
	if (AbilityComponentRef->IsCasting())
	{
		OnCastStateChanged(FCastingState(), AbilityComponentRef->GetCurrentCastingState());
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(UIDataAsset) && IsValid(UninterruptibleIcon))
	{
		UninterruptibleIcon->SetBrushFromTexture(UIDataAsset->UninterruptibleCastIcon);
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
		if (IsValid(CastProgress) && IsValid(UIDataAsset))
		{
			const FLinearColor ProgressColor = NewState.bInterruptible ? UIDataAsset->GetSchoolColor(NewState.CurrentCast->GetAbilitySchool()) : UIDataAsset->UninterruptibleCastColor;
			CastProgress->SetFillColorAndOpacity(ProgressColor);
		}
		if (IsValid(UninterruptibleIcon))
		{
			if (NewState.bInterruptible)
			{
				UninterruptibleIcon->SetVisibility(ESlateVisibility::Hidden);
			}
			else
			{
				UninterruptibleIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		if (IsValid(DurationText))
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