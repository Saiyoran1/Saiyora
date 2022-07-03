#include "CoreClasses/PlayerAnimInstance.h"
#include "SaiyoraPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	PlayerCharacter = Cast<ASaiyoraPlayerCharacter>(TryGetPawnOwner());
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<ASaiyoraPlayerCharacter>(TryGetPawnOwner());
	}
	if (!IsValid(PlayerCharacter))
	{
		return;
	}
	FVector Velocity =  PlayerCharacter->GetVelocity();
	Velocity.Z = 0.0f;
	Speed = Velocity.Size();
	bIsInAir = PlayerCharacter->GetMovementComponent()->IsFalling();
	bIsAccelerating = PlayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f;
}
