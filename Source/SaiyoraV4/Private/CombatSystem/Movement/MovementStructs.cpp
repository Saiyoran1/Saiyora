#include "Movement/MovementStructs.h"

const FGameplayTag FMovementTags::GenericExternalMovement = FGameplayTag::RequestGameplayTag(FName(TEXT("Movement")), false);
const FGameplayTag FMovementTags::GenericMovementRestriction = FGameplayTag::RequestGameplayTag(FName(TEXT("Movement.Restriction")), false);