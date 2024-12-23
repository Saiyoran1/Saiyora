#include "UIBuffFunctions.h"
#include "Buff.h"
#include "PlayerHUD.h"

void UAbilityProcFunction::SetupBuffFunction()
{
	ASaiyoraPlayerCharacter* PlayerTarget = Cast<ASaiyoraPlayerCharacter>(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(PlayerTarget) || !PlayerTarget->IsLocallyControlled())
	{
		return;
	}
	LocalPlayerHUD = PlayerTarget->GetPlayerHUD();
}

void UAbilityProcFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(LocalPlayerHUD))
	{
		return;
	}
	LocalPlayerHUD->AddAbilityProc(GetOwningBuff(), AbilityClass);
}