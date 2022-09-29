#include "Specialization/SpecializationStructs.h"
#include "Specialization/AncientSpecialization.h"

void FAncientTalentChoice::PostReplicatedAdd(const FAncientTalentSet& InArraySerializer)
{
	if (IsValid(InArraySerializer.OwningSpecialization))
	{
		InArraySerializer.OwningSpecialization->SelectTalent(BaseAbility, CurrentSelection);
	}
}

void FAncientTalentChoice::PostReplicatedChange(const FAncientTalentSet& InArraySerializer)
{
	if (IsValid(InArraySerializer.OwningSpecialization))
	{
		InArraySerializer.OwningSpecialization->SelectTalent(BaseAbility, CurrentSelection);
	}
}
