#include "RootMotionHandler.h"
#include "PlayerCombatAbility.h"
#include "SaiyoraMovementComponent.h"

void URootMotionHandler::OnMispredicted(int32 const PredictionID, ECastFailReason const FailReason)
{
	if (PredictionID == this->PredictionID)
	{
		Expire();
	}
}

void URootMotionHandler::OnTimeout()
{
	Expire();
}

void URootMotionHandler::Expire()
{
	GetWorld()->GetTimerManager().ClearTimer(DurationHandle);
	if (IsValid(SourceHandler))
	{
		SourceHandler->UnsubscribeFromAbilityMispredicted(MispredictionCallback);
	}
	if (IsValid(TargetMovement))
	{
		TargetMovement->ExpireHandledRootMotion(this);
	}
}

void URootMotionHandler::Init(USaiyoraMovementComponent* Movement, UPlayerAbilityHandler* AbilityHandler,
	uint16 SourceID, bool const bDurationBased, float const ExpireDuration, int32 const AbilityPredictionID)
{
	if (!IsValid(Movement) || !IsValid(AbilityHandler) || SourceID == (uint16)ERootMotionSourceID::Invalid)
	{
		return;
	}
	if (AbilityHandler->GetOwnerRole() == ROLE_AutonomousProxy)
	{
		if (AbilityPredictionID == 0)
		{
			return;
		}
		SourceHandler = AbilityHandler;
		MispredictionCallback.BindDynamic(this, &URootMotionHandler::OnMispredicted);
		SourceHandler->SubscribeToAbilityMispredicted(MispredictionCallback);
	}
	PredictionID = AbilityPredictionID;
	RootMotionSourceID = SourceID;
	TargetMovement = Movement;
	if (bDurationBased && ExpireDuration > 0.0f)
	{
		bHasDuration = bDurationBased;
		Duration = ExpireDuration;
		GetWorld()->GetTimerManager().SetTimer(DurationHandle, this, &URootMotionHandler::OnTimeout, Duration);
	}
	SetupExpirationConditions();
}
