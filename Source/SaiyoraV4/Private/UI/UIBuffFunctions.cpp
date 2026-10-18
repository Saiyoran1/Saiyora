#include "UIBuffFunctions.h"
#include "Buff.h"
#include "PlayerHUD.h"

void UAbilityProcFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	const ASaiyoraPlayerCharacter* PlayerTarget = Cast<ASaiyoraPlayerCharacter>(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(PlayerTarget) || !PlayerTarget->IsLocallyControlled())
	{
		return;
	}
	LocalPlayerHUD = PlayerTarget->GetPlayerHUD();
	if (!IsValid(LocalPlayerHUD))
	{
		return;
	}
	LocalPlayerHUD->AddAbilityProc(GetOwningBuff(), AbilityClass);
}