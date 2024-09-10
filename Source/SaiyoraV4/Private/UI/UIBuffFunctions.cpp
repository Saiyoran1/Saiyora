#include "UIBuffFunctions.h"
#include "Buff.h"
#include "CombatAbility.h"
#include "PlayerHUD.h"

void UAbilityProcFunction::SetModifierVars(const TSubclassOf<UCombatAbility> AbilityClass)
{
	TargetAbility = AbilityClass;
	ASaiyoraPlayerCharacter* PlayerTarget = Cast<ASaiyoraPlayerCharacter>(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(PlayerTarget) || !PlayerTarget->IsLocallyControlled())
	{
		return;
	}
	LocalPlayerHUD = PlayerTarget->GetPlayerHUD();
}

void UAbilityProcFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	Super::OnApply(ApplyEvent);

	if (!IsValid(LocalPlayerHUD))
	{
		return;
	}
	LocalPlayerHUD->AddAbilityProc(GetOwningBuff(), TargetAbility);
}

void UAbilityProcFunction::ProcAbility(UBuff* Buff, const TSubclassOf<UCombatAbility> AbilityClass)
{
	if (!IsValid(Buff) || !IsValid(AbilityClass))
	{
		return;
	}
	UAbilityProcFunction* NewAbilityProcFunction = Cast<UAbilityProcFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewAbilityProcFunction))
	{
		return;
	}
	NewAbilityProcFunction->SetModifierVars(AbilityClass);
}