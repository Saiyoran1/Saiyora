#include "Specialization/AncientTalent.h"
#include "AbilityComponent.h"
#include "SaiyoraPlayerCharacter.h"

void UAncientTalent::SelectTalent(UAncientSpecialization* OwningSpecialization,
	const TSubclassOf<UCombatAbility> BaseAbility)
{
	OwningSpec = OwningSpecialization;
	BaseAbilityClass = BaseAbility;
	if (IsValid(ReplaceBaseWith) && ReplaceBaseWith != BaseAbility)
	{
		ReplacePlayerAbilityWith(BaseAbility, ReplaceBaseWith);
	}
	OnTalentSelected();
}

void UAncientTalent::UnselectTalent()
{
	if (IsValid(ReplaceBaseWith) && ReplaceBaseWith != BaseAbilityClass)
	{
		ReplacePlayerAbilityWith(ReplaceBaseWith, BaseAbilityClass);
	}
	OnTalentUnselected();
}

void UAncientTalent::ReplacePlayerAbilityWith(const TSubclassOf<UCombatAbility> AbilityToReplace,
                                              const TSubclassOf<UCombatAbility> NewAbility)
{
	if (!IsValid(OwningSpec) || !IsValid(OwningSpec->GetOwningPlayer()))
	{
		return;
	}
	if (OwningSpec->GetOwningPlayer()->HasAuthority())
	{
		UAbilityComponent* AbilityComponent = ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningSpec->GetOwningPlayer());
		if (!IsValid(AbilityComponent))
		{
			return;
		}
		const UCombatAbility* Replacing = AbilityComponent->FindActiveAbility(AbilityToReplace);
		if (!IsValid(Replacing))
		{
			return;
		}
		if (AbilityComponent->RemoveAbility(AbilityToReplace))
		{
			AbilityComponent->AddNewAbility(NewAbility);
		}
	}
	if (OwningSpec->GetOwningPlayer()->IsLocallyControlled())
	{
		TMap<int32, TSubclassOf<UCombatAbility>> AncientMappings;
		OwningSpec->GetOwningPlayer()->GetAncientMappings(AncientMappings);
		const int32* IndexPtr = AncientMappings.FindKey(AbilityToReplace);
		if (IndexPtr)
		{
			OwningSpec->GetOwningPlayer()->SetAbilityMapping(ESaiyoraPlane::Ancient, *IndexPtr, NewAbility);
		}
	}
}
