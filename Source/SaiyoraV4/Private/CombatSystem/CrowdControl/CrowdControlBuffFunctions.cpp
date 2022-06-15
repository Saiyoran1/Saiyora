#include "CrowdControlBuffFunctions.h"
#include "Buff.h"
#include "CrowdControlHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Cc Immunity

void UCrowdControlImmunityFunction::CrowdControlImmunity(UBuff* Buff, FGameplayTag const ImmunedCrowdControl)
{
	if (!IsValid(Buff) || !ImmunedCrowdControl.IsValid() || !ImmunedCrowdControl.MatchesTag(FCcTags::GenericCrowdControl) ||
		ImmunedCrowdControl.MatchesTagExact(FCcTags::GenericCrowdControl) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UCrowdControlImmunityFunction* NewCcImmunityFunction = Cast<UCrowdControlImmunityFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewCcImmunityFunction))
	{
		return;
	}
	NewCcImmunityFunction->SetRestrictionVars(ImmunedCrowdControl);
}

bool UCrowdControlImmunityFunction::RestrictCcBuff(FBuffApplyEvent const& ApplyEvent)
{
	FGameplayTagContainer BuffTags;
	ApplyEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	return BuffTags.HasTagExact(ImmuneCc);
}

void UCrowdControlImmunityFunction::SetRestrictionVars(FGameplayTag const ImmunedCc)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetCcHandler = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwningBuff()->GetAppliedTo());
		TargetBuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwningBuff()->GetAppliedTo());
		ImmuneCc = ImmunedCc;
		CcImmunity.BindDynamic(this, &UCrowdControlImmunityFunction::RestrictCcBuff);
	}
}

void UCrowdControlImmunityFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetBuffHandler) && IsValid(TargetCcHandler))
	{
		FCrowdControlStatus const Status = TargetCcHandler->GetCrowdControlStatus(ImmuneCc);
		if (Status.bActive)
		{
			TArray<UBuff*> Ccs = Status.Sources;
			for (UBuff* Cc : Ccs)
			{
				TargetBuffHandler->RemoveBuff(Cc, EBuffExpireReason::Dispel);
			}
		}
		TargetBuffHandler->AddIncomingBuffRestriction(GetOwningBuff(), CcImmunity);
	}
}

void UCrowdControlImmunityFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetBuffHandler))
	{
		TargetBuffHandler->RemoveIncomingBuffRestriction(GetOwningBuff());
	}
}

#pragma endregion