#include "CrowdControlBuffFunctions.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CrowdControlHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Cc Immunity

bool UCrowdControlImmunityFunction::RestrictCcBuff(const FBuffApplyEvent& ApplyEvent)
{
	FGameplayTagContainer BuffTags;
	ApplyEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	return BuffTags.HasTagExact(ImmuneCc);
}

void UCrowdControlImmunityFunction::SetupBuffFunction()
{
	TargetCcHandler = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwningBuff()->GetAppliedTo());
	TargetBuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwningBuff()->GetAppliedTo());
	CcImmunity.BindDynamic(this, &UCrowdControlImmunityFunction::RestrictCcBuff);
}

void UCrowdControlImmunityFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetCcHandler))
	{
		//Remove any buffs currently appling the CC
		const FCrowdControlStatus Status = TargetCcHandler->GetCrowdControlStatus(ImmuneCc);
		if (Status.bActive)
		{
			for (UBuff* Cc : Status.Sources)
			{
				TargetBuffHandler->RemoveBuff(Cc, EBuffExpireReason::Dispel);
			}
		}
	}
	if (IsValid(TargetBuffHandler))
	{
		//Restrict future buffs from applying the CC
		TargetBuffHandler->AddIncomingBuffRestriction(CcImmunity);
	}
}

void UCrowdControlImmunityFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetBuffHandler))
	{
		TargetBuffHandler->RemoveIncomingBuffRestriction(CcImmunity);
	}
}

#pragma endregion