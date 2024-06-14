#include "Generator_PreviousMoveLocation.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatInterface.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"

UEQG_PreviousMoveLocation::UEQG_PreviousMoveLocation()
{
	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UEQG_PreviousMoveLocation::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	const UObject* Owner = QueryInstance.Owner.Get();
	if (!IsValid(Owner))
	{
		return;
	}
	const AActor* OwningActor = Cast<AActor>(Owner);
	if (!IsValid(OwningActor))
	{
		const UActorComponent* OwnerAsComponent = Cast<UActorComponent>(Owner);
		if (IsValid(OwnerAsComponent))
		{
			OwningActor = OwnerAsComponent->GetOwner();
			if (!IsValid(OwningActor))
			{
				return;
			}
		}
		else
		{
			return;
		}
	}
	if (!OwningActor->Implements<USaiyoraCombatInterface>())
	{
		return;
	}
	const UNPCAbilityComponent* AbilityComp = Cast<UNPCAbilityComponent>(ISaiyoraCombatInterface::Execute_GetAbilityComponent(OwningActor));
	if (!IsValid(AbilityComp))
	{
		return;
	}
	if (!AbilityComp->HasValidMoveGoal())
	{
		return;
	}
	QueryInstance.AddItemData<UEnvQueryItemType_Point>(AbilityComp->GetMoveGoal());
}