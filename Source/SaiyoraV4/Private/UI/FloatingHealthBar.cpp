#include "UI/FloatingHealthBar.h"

#include "AbilityComponent.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "DamageHandler.h"
#include "FloatingBuffIcon.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlayerCharacter.h"

void UFloatingHealthBar::Init(AActor* TargetActor)
{
	if (!IsValid(BuffIconClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("Floating health bar tried to init with invalid buff icon class."));
		RemoveFromParent();
		return;
	}
	
	LocalPlayer = USaiyoraCombatLibrary::GetLocalSaiyoraPlayer(GetOwningPlayer()->GetWorld());
	if (!IsValid(LocalPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Floating health bar got a null local player during Init."));
		RemoveFromParent();
		return;
	}
	
	if (!IsValid(TargetActor) || !TargetActor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Floating health bar tried to init with an invalid target actor, or its target actor didn't implement the combat interface."));
		RemoveFromParent();
		return;
	}
	
	TargetDamageHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(TargetActor);
	if (IsValid(TargetDamageHandler))
	{
		TargetDamageHandler->OnMaxHealthChanged.AddDynamic(this, &UFloatingHealthBar::UpdateHealth);
		TargetDamageHandler->OnHealthChanged.AddDynamic(this, &UFloatingHealthBar::UpdateHealth);
		UpdateHealth(nullptr, 0.0f, 0.0f);
		TargetDamageHandler->OnAbsorbChanged.AddDynamic(this, &UFloatingHealthBar::UpdateAbsorb);
		UpdateAbsorb(nullptr, 0.0f, 0.0f);
		TargetDamageHandler->OnLifeStatusChanged.AddDynamic(this, &UFloatingHealthBar::UpdateLifeStatus);
		UpdateLifeStatus(nullptr, ELifeStatus::Invalid, TargetDamageHandler->GetLifeStatus());
	}
	else if (IsValid(HealthOverlay))
	{
		HealthOverlay->RemoveFromParent();
	}
	
	TargetBuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(TargetActor);
	if (IsValid(TargetBuffHandler))
	{
		TargetBuffHandler->OnIncomingBuffApplied.AddDynamic(this, &UFloatingHealthBar::OnIncomingBuffApplied);
	}
	else
	{
		if (IsValid(BuffBox))
		{
			BuffBox->RemoveFromParent();
		}
		if (IsValid(DebuffBox))
		{
			DebuffBox->RemoveFromParent();
		}
	}

	TargetAbilityComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(TargetActor);
	if (IsValid(TargetAbilityComponent))
	{
		TargetAbilityComponent->OnCastStateChanged.AddDynamic(this, &UFloatingHealthBar::OnCastStateChanged);
		TargetAbilityComponent->OnAbilityCancelled.AddDynamic(this, &UFloatingHealthBar::OnCastCancelled);
		TargetAbilityComponent->OnAbilityInterrupted.AddDynamic(this, &UFloatingHealthBar::OnCastInterrupted);
		FCastingState CurrentCastState;
		CurrentCastState.bIsCasting = TargetAbilityComponent->IsCasting();
		CurrentCastState.CastLength = TargetAbilityComponent->GetCurrentCastLength();
		CurrentCastState.CastStartTime = (GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + TargetAbilityComponent->GetCastTimeRemaining()) - CurrentCastState.CastLength;
		CurrentCastState.CurrentCast = TargetAbilityComponent->GetCurrentCast();
		CurrentCastState.bInterruptible = TargetAbilityComponent->IsInterruptible();
		OnCastStateChanged(FCastingState(), CurrentCastState);
	}
	else
	{
		if (IsValid(CastBar))
		{
			CastBar->RemoveFromParent();
		}
		if (IsValid(CastBarText))
		{
			CastBarText->RemoveFromParent();
		}
	}
}

#pragma region Health

void UFloatingHealthBar::UpdateHealth(AActor* Actor, const float PreviousHealth, const float NewHealth)
{
	const float NewPercent = TargetDamageHandler->GetMaxHealth() <= 0.0f ? 0.0f : FMath::Clamp(TargetDamageHandler->GetCurrentHealth() / TargetDamageHandler->GetMaxHealth(), 0.0f, 1.0f);
	if (IsValid(HealthBar))
	{
		HealthBar->SetPercent(NewPercent);
	}
	if (IsValid(HealthText))
	{
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 0;
		HealthText->SetText(FText::AsNumber(FMath::RoundToInt(NewPercent * 100), &Options));
	}
}

void UFloatingHealthBar::UpdateAbsorb(AActor* Actor, const float PreviousHealth, const float NewHealth)
{
	const float NewPercent = TargetDamageHandler->GetMaxHealth() <= 0.0f ? 0.0f : FMath::Clamp(TargetDamageHandler->GetCurrentAbsorb() / TargetDamageHandler->GetMaxHealth(), 0.0f, 1.0f);
	if (IsValid(AbsorbBar))
	{
		AbsorbBar->SetPercent(NewPercent);
	}
}

void UFloatingHealthBar::UpdateLifeStatus(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus)
{
	if (NewStatus != ELifeStatus::Alive)
	{
		RemoveFromParent();
	}
}

#pragma endregion
#pragma region Buffs

void UFloatingHealthBar::OnIncomingBuffApplied(const FBuffApplyEvent& Event)
{
	//For now, we will only display buffs applied by the local player, to prevent clutter.
	//Probably need a system in place for determining "important" buffs and debuffs.
	if (!IsValid(Event.AffectedBuff) || Event.AffectedBuff->GetBuffType() == EBuffType::HiddenBuff
		|| Event.AppliedTo != TargetBuffHandler->GetOwner() || (Event.AffectedBuff->GetBuffType() == EBuffType::Debuff && Event.AppliedBy != LocalPlayer))
	{
		return;
	}
	UFloatingBuffIcon* BuffIcon = CreateWidget<UFloatingBuffIcon>(this, BuffIconClass);
	if (IsValid(BuffIcon))
	{
		UWrapBox* BoxToAddTo = Event.AffectedBuff->GetBuffType() == EBuffType::Buff ? BuffBox : DebuffBox;
		BoxToAddTo->AddChildToWrapBox(BuffIcon);
		BuffIcon->Init(Event.AffectedBuff);
	}
}

#pragma endregion