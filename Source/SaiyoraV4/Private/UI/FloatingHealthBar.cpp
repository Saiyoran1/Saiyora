#include "UI/FloatingHealthBar.h"
#include "BuffHandler.h"
#include "DamageHandler.h"
#include "SaiyoraCombatInterface.h"

void UFloatingHealthBar::Init(AActor* TargetActor)
{
	if (!IsValid(TargetActor) || !TargetActor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
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
		//TODO: Buff widget handling.	
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
}

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
