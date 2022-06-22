#include "CrowdControlBuffFunctions.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CrowdControlHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Cc Immunity

void UCrowdControlImmunityFunction::CrowdControlImmunity(UBuff* Buff, const FGameplayTag ImmunedCrowdControl)
{
	if (!IsValid(Buff) || !ImmunedCrowdControl.IsValid() || !ImmunedCrowdControl.MatchesTag(FSaiyoraCombatTags::Get().CrowdControl) ||
		ImmunedCrowdControl.MatchesTagExact(FSaiyoraCombatTags::Get().CrowdControl) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
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

bool UCrowdControlImmunityFunction::RestrictCcBuff(const FBuffApplyEvent& ApplyEvent)
{
	FGameplayTagContainer BuffTags;
	ApplyEvent.BuffClass.GetDefaultObject()->GetBuffTags(BuffTags);
	return BuffTags.HasTagExact(ImmuneCc);
}

void UCrowdControlImmunityFunction::SetRestrictionVars(const FGameplayTag ImmunedCc)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetCcHandler = ISaiyoraCombatInterface::Execute_GetCrowdControlHandler(GetOwningBuff()->GetAppliedTo());
		TargetBuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwningBuff()->GetAppliedTo());
		ImmuneCc = ImmunedCc;
		CcImmunity.BindDynamic(this, &UCrowdControlImmunityFunction::RestrictCcBuff);
	}
}

void UCrowdControlImmunityFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetBuffHandler) && IsValid(TargetCcHandler))
	{
		const FCrowdControlStatus Status = TargetCcHandler->GetCrowdControlStatus(ImmuneCc);
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

void UCrowdControlImmunityFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetBuffHandler))
	{
		TargetBuffHandler->RemoveIncomingBuffRestriction(GetOwningBuff());
	}
}

#pragma endregion