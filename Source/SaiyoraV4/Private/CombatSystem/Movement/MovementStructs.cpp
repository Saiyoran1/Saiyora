#include "Movement/MovementStructs.h"
#include "SaiyoraMovementComponent.h"

void FServerMoveStatChange::PostReplicatedAdd(const FServerMoveStatArray& InArraySerializer)
{
	if (IsValid(InArraySerializer.OwningComponent))
	{
		InArraySerializer.OwningComponent->UpdateMoveStatFromServer(ChangeID, StatTag, Value);
	}
}

void FServerMoveStatChange::PostReplicatedChange(const FServerMoveStatArray& InArraySerializer)
{
	if (IsValid(InArraySerializer.OwningComponent))
	{
		InArraySerializer.OwningComponent->UpdateMoveStatFromServer(ChangeID, StatTag, Value);
	}
}
