#include "Specialization/SpecializationStructs.h"

#include "ModernSpecialization.h"
#include "Specialization/AncientSpecialization.h"
#include "SaiyoraPlayerCharacter.h"
#include "Specialization/AncientTalent.h"

void FAncientTalentChoice::PostReplicatedAdd(const FAncientTalentSet& InArraySerializer)
{
	if (IsValid(InArraySerializer.OwningSpecialization) && IsValid(InArraySerializer.OwningSpecialization->GetOwningPlayer()))
	{
		if (IsValid(CurrentSelection))
		{
			ActiveTalent = NewObject<UAncientTalent>(InArraySerializer.OwningSpecialization->GetOwningPlayer(), CurrentSelection);
			if (IsValid(ActiveTalent))
			{
				ActiveTalent->SelectTalent(InArraySerializer.OwningSpecialization, TalentRow.BaseAbilityClass);
			}
		}
	}
}

void FAncientTalentChoice::PostReplicatedChange(const FAncientTalentSet& InArraySerializer)
{
	if (IsValid(InArraySerializer.OwningSpecialization) && IsValid(InArraySerializer.OwningSpecialization->GetOwningPlayer()))
	{
		const TSubclassOf<UAncientTalent> PreviousTalent = IsValid(ActiveTalent) ? ActiveTalent->GetClass() : nullptr;
		if (PreviousTalent != CurrentSelection)
		{
			if (IsValid(ActiveTalent))
			{
				ActiveTalent->UnselectTalent();
				ActiveTalent = nullptr;
			}
			if (IsValid(CurrentSelection))
			{
				ActiveTalent = NewObject<UAncientTalent>(InArraySerializer.OwningSpecialization->GetOwningPlayer(), CurrentSelection);
				if (IsValid(ActiveTalent))
				{
					ActiveTalent->SelectTalent(InArraySerializer.OwningSpecialization, TalentRow.BaseAbilityClass);
				}
			}
		}
	}
}