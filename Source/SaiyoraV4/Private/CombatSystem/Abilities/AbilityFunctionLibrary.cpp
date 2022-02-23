#include "AbilityFunctionLibrary.h"
#include "Hitbox.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraGameState.h"
#include "SaiyoraPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

float const UAbilityFunctionLibrary::CamTraceLength = 10000.0f;
float const UAbilityFunctionLibrary::RewindTraceRadius = 300.0f;
float const UAbilityFunctionLibrary::AimToleranceDegrees = 10.0f;

FAbilityOrigin UAbilityFunctionLibrary::MakeAbilityOrigin(FVector const& AimLocation, FVector const& AimDirection, FVector const& Origin)
{
	FAbilityOrigin Target;
	Target.Origin = Origin;
	Target.AimLocation = AimLocation;
	Target.AimDirection = AimDirection;
	return Target;
}

#pragma region Snapshotting

void UAbilityFunctionLibrary::RewindRelevantHitboxes(ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, TArray<AActor*> const& Targets,
		TArray<AActor*> const& ActorsToIgnore, TMap<UHitbox*, FTransform>& ReturnTransforms)
{
	if (!IsValid(Shooter))
	{
		return;
	}
	float const Ping = USaiyoraCombatLibrary::GetActorPing(Shooter);
	ASaiyoraGameState* GameState = Shooter->GetWorld()->GetGameState<ASaiyoraGameState>();
	if (Ping <= 0.0f || !IsValid(GameState))
	{
		return;
	}
	//Do a big trace in front of the camera to rewind all targets that could potentially intercept the trace.
	FVector const RewindTraceEnd = Origin.AimLocation + CamTraceLength * Origin.AimDirection;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11));
	TArray<FHitResult> RewindTraceResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.AimLocation, RewindTraceEnd, RewindTraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::None, RewindTraceResults, true, FLinearColor::White);
	//All targets we are interested in should also be rewound, even if they weren't in the trace.
	TArray<AActor*> RewindTargets = Targets;
	for (FHitResult const& Hit : RewindTraceResults)
	{
		if (IsValid(Hit.GetActor()))
		{
			RewindTargets.AddUnique(Hit.GetActor());
		}
	}
	for (AActor* RewindTarget : RewindTargets)
	{
		TArray<UHitbox*> HitboxComponents;
		RewindTarget->GetComponents<UHitbox>(HitboxComponents);
		for (UHitbox* Hitbox : HitboxComponents)
		{
			ReturnTransforms.Add(Hitbox, GameState->RewindHitbox(Hitbox, Ping));
		}
	}
}

void UAbilityFunctionLibrary::UnrewindHitboxes(TMap<UHitbox*, FTransform> const& ReturnTransforms)
{
	for (TTuple<UHitbox*, FTransform> const& ReturnTransform : ReturnTransforms)
	{
		if (IsValid(ReturnTransform.Key))
		{
			ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
		}
	}
}

#pragma endregion 
#pragma region Trace Validation

bool UAbilityFunctionLibrary::PredictLineTrace(ASaiyoraPlayerCharacter* Shooter, float const TraceLength,
	bool const bHostile, TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID,
	FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Result = FHitResult();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f)
	{
		return false;
	}
	FName AimSocket;
	USceneComponent* AimComponent = ISaiyoraCombatInterface::Execute_GetAimSocket(Shooter, AimSocket);
	FName OriginSocket;
	USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(Shooter, OriginSocket);
	
	if (IsValid(AimComponent))
	{
		OutOrigin.AimLocation = AimComponent->GetSocketLocation(AimSocket);
		OutOrigin.AimDirection = AimComponent->GetForwardVector();
		OutOrigin.Origin = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : OutOrigin.AimLocation;
	}
	else if (IsValid(OriginComponent))
	{
		OutOrigin.Origin = OriginComponent->GetSocketLocation(OriginSocket);
		OutOrigin.AimLocation = OutOrigin.Origin;
		OutOrigin.AimDirection = OriginComponent->GetForwardVector();
	}
	else
	{
		OutOrigin.Origin = Shooter->GetActorLocation();
		OutOrigin.AimLocation = Shooter->GetActorLocation();
		OutOrigin.AimDirection = Shooter->GetActorForwardVector();
	}

	FVector const CamTraceEnd = OutOrigin.AimLocation + (CamTraceLength * OutOrigin.AimDirection);
	FHitResult CamTraceResult;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, Result, true, FLinearColor::Yellow);

	OutTargetSet.SetID = TargetSetID;
	if (IsValid(Result.GetActor()))
	{
		OutTargetSet.Targets.Add(Result.GetActor());
	}
	
	return Result.bBlockingHit;
}

bool UAbilityFunctionLibrary::ValidateLineTrace(ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin, AActor* Target,
	float const TraceLength, bool const bHostile, TArray<AActor*> const& ActorsToIgnore)
{
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || !IsValid(Target) || Shooter == Target
		|| ActorsToIgnore.Contains(Target) || TraceLength <= 0.0f)
	{
		return false;
	}
	if (Shooter->IsLocallyControlled())
	{
		//If the prediction was done locally, we don't need to validate.
		return true;
	}
	//TODO: Validate aim location and origin (if using separate origin).
	TArray<AActor*> TargetArray;
	TargetArray.Add(Target);
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, TargetArray, ActorsToIgnore, ReturnTransforms);
	//Trace from the camera for a very large distance to find what we were aiming at.
	//TODO: Math to find distance and angle from origin point so that we can calculate the max distance we can trace before we outrange the origin.
	FVector const CamTraceEnd = Origin.AimLocation + (CamTraceLength * Origin.AimDirection.GetSafeNormal());
	FHitResult CamTraceResult;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();
	FHitResult OriginResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.Origin, OriginTraceEnd, TraceChannel, false, ActorsToIgnore,
		EDrawDebugTrace::None, OriginResult, true, FLinearColor::Yellow);
	
	bool bDidHit = false;
	if (IsValid(OriginResult.GetActor()) && OriginResult.GetActor() == Target)
	{
		bDidHit = true;
	}
	UnrewindHitboxes(ReturnTransforms);
	return bDidHit;
}

bool UAbilityFunctionLibrary::PredictMultiLineTrace(ASaiyoraPlayerCharacter* Shooter, float const TraceLength, bool const bHostile,
	TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID, TArray<FHitResult>& Results,
	FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Results.Empty();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f)
	{
		return false;
	}
	FName AimSocket;
	USceneComponent* AimComponent = ISaiyoraCombatInterface::Execute_GetAimSocket(Shooter, AimSocket);
	FName OriginSocket;
	USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(Shooter, OriginSocket);
	
	if (IsValid(AimComponent))
	{
		OutOrigin.AimLocation = AimComponent->GetSocketLocation(AimSocket);
		OutOrigin.AimDirection = AimComponent->GetForwardVector();
		OutOrigin.Origin = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : OutOrigin.AimLocation;
	}
	else if (IsValid(OriginComponent))
	{
		OutOrigin.Origin = OriginComponent->GetSocketLocation(OriginSocket);
		OutOrigin.AimLocation = OutOrigin.Origin;
		OutOrigin.AimDirection = OriginComponent->GetForwardVector();
	}
	else
	{
		OutOrigin.Origin = Shooter->GetActorLocation();
		OutOrigin.AimLocation = Shooter->GetActorLocation();
		OutOrigin.AimDirection = Shooter->GetActorForwardVector();
	}

	FVector const CamTraceEnd = OutOrigin.AimLocation + (CamTraceLength * OutOrigin.AimDirection);
	FHitResult CamTraceResult;
	//For the target trace, we use a channel that hitboxes will block (PlayerFriendly or PlayerHostile) so that we aim at the first thing in the center of the screen.
	ETraceTypeQuery const CamChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, CamTraceEnd, CamChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	//For the actual multi-trace, we use a channel that hitboxes will overlap instead of block (PlayerFriendlyOverlap or PlayerHostileOverlap) so that we get multiple results.
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::LineTraceMulti(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, Results, true, FLinearColor::Yellow);

	OutTargetSet.SetID = TargetSetID;
	for (FHitResult const& Result : Results)
	{
		if (IsValid(Result.GetActor()))
		{
			OutTargetSet.Targets.Add(Result.GetActor());
		}
	}
	
	return OutTargetSet.Targets.Num() > 0;
}

TArray<AActor*> UAbilityFunctionLibrary::ValidateMultiLineTrace(ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin,
	TArray<AActor*> const& Targets, float const TraceLength, bool const bHostile, TArray<AActor*> const& ActorsToIgnore)
{
	TArray<AActor*> ValidatedTargets;
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || Targets.Num() == 0 || TraceLength <= 0.0f)
	{
		return ValidatedTargets;
	}
	if (Shooter->IsLocallyControlled())
	{
		//If the prediction was done locally, we don't need to validate.
		return Targets;
	}
	//TODO: Validate aim location and origin (if using separate origin).
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, Targets, ActorsToIgnore, ReturnTransforms);
	//Trace from the camera for a very large distance to find what we were aiming at.
	//TODO: Math to find distance and angle from origin point so that we can calculate the max distance we can trace before we outrange the origin.
	FVector const CamTraceEnd = Origin.AimLocation + (CamTraceLength * Origin.AimDirection.GetSafeNormal());
	FHitResult CamTraceResult;
	ETraceTypeQuery const CamChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, CamTraceEnd,
		CamChannel, false, ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();
	TArray<FHitResult> Results;
	//For the actual multi-trace, we use a channel that hitboxes will overlap instead of block (PlayerFriendlyOverlap or PlayerHostileOverlap) so that we get multiple results.
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::LineTraceMulti(Shooter, Origin.Origin, OriginTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, Results, true, FLinearColor::Yellow);
	
	for (FHitResult const& Result : Results)
	{
		if (IsValid(Result.GetActor()) && Targets.Contains(Result.GetActor()))
		{
			ValidatedTargets.AddUnique(Result.GetActor());
		}
	}
	UnrewindHitboxes(ReturnTransforms);
	return ValidatedTargets;
}

bool UAbilityFunctionLibrary::PredictSphereTrace(ASaiyoraPlayerCharacter* Shooter, float const TraceLength,
	float const TraceRadius, bool const bHostile, TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID,
	FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Result = FHitResult();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}
	FName AimSocket;
	USceneComponent* AimComponent = ISaiyoraCombatInterface::Execute_GetAimSocket(Shooter, AimSocket);
	FName OriginSocket;
	USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(Shooter, OriginSocket);
	
	if (IsValid(AimComponent))
	{
		OutOrigin.AimLocation = AimComponent->GetSocketLocation(AimSocket);
		OutOrigin.AimDirection = AimComponent->GetForwardVector();
		OutOrigin.Origin = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : OutOrigin.AimLocation;
	}
	else if (IsValid(OriginComponent))
	{
		OutOrigin.Origin = OriginComponent->GetSocketLocation(OriginSocket);
		OutOrigin.AimLocation = OutOrigin.Origin;
		OutOrigin.AimDirection = OriginComponent->GetForwardVector();
	}
	else
	{
		OutOrigin.Origin = Shooter->GetActorLocation();
		OutOrigin.AimLocation = Shooter->GetActorLocation();
		OutOrigin.AimDirection = Shooter->GetActorForwardVector();
	}

	FVector const CamTraceEnd = OutOrigin.AimLocation + (CamTraceLength * OutOrigin.AimDirection);
	FHitResult CamTraceResult;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	UKismetSystemLibrary::SphereTraceSingle(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceRadius, TraceChannel, false, ActorsToIgnore,
		EDrawDebugTrace::None, Result, true, FLinearColor::Yellow);

	OutTargetSet.SetID = TargetSetID;
	if (IsValid(Result.GetActor()))
	{
		OutTargetSet.Targets.Add(Result.GetActor());
	}
	
	return Result.bBlockingHit;
}

bool UAbilityFunctionLibrary::ValidateSphereTrace(ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin const& Origin,
	AActor* Target, float const TraceLength, float const TraceRadius, bool const bHostile,
	TArray<AActor*> const& ActorsToIgnore)
{
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || !IsValid(Target) || Shooter == Target
		|| ActorsToIgnore.Contains(Target) || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}
	if (Shooter->IsLocallyControlled())
	{
		//If the prediction was done locally, we don't need to validate.
		return true;
	}
	//TODO: Validate aim location and origin (if using separate origin).
	TArray<AActor*> TargetArray;
	TargetArray.Add(Target);
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, TargetArray, ActorsToIgnore, ReturnTransforms);
	//Trace from the camera for a very large distance to find what we were aiming at.
	//TODO: Math to find distance and angle from origin point so that we can calculate the max distance we can trace before we outrange the origin.
	FVector const CamTraceEnd = Origin.AimLocation + (CamTraceLength * Origin.AimDirection.GetSafeNormal());
	FHitResult CamTraceResult;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, CamTraceEnd,
		TraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();
	FHitResult OriginResult;
	UKismetSystemLibrary::SphereTraceSingle(Shooter, Origin.Origin, OriginTraceEnd, TraceRadius, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginResult, true, FLinearColor::Yellow);
	
	bool bDidHit = false;
	if (IsValid(OriginResult.GetActor()) && OriginResult.GetActor() == Target)
	{
		bDidHit = true;
	}
	UnrewindHitboxes(ReturnTransforms);
	return bDidHit;
}

bool UAbilityFunctionLibrary::PredictMultiSphereTrace(ASaiyoraPlayerCharacter* Shooter, float const TraceLength,
	float const TraceRadius, bool const bHostile, TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID,
	TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Results.Empty();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}
	FName AimSocket;
	USceneComponent* AimComponent = ISaiyoraCombatInterface::Execute_GetAimSocket(Shooter, AimSocket);
	FName OriginSocket;
	USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(Shooter, OriginSocket);
	
	if (IsValid(AimComponent))
	{
		OutOrigin.AimLocation = AimComponent->GetSocketLocation(AimSocket);
		OutOrigin.AimDirection = AimComponent->GetForwardVector();
		OutOrigin.Origin = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : OutOrigin.AimLocation;
	}
	else if (IsValid(OriginComponent))
	{
		OutOrigin.Origin = OriginComponent->GetSocketLocation(OriginSocket);
		OutOrigin.AimLocation = OutOrigin.Origin;
		OutOrigin.AimDirection = OriginComponent->GetForwardVector();
	}
	else
	{
		OutOrigin.Origin = Shooter->GetActorLocation();
		OutOrigin.AimLocation = Shooter->GetActorLocation();
		OutOrigin.AimDirection = Shooter->GetActorForwardVector();
	}

	FVector const CamTraceEnd = OutOrigin.AimLocation + (CamTraceLength * OutOrigin.AimDirection);
	FHitResult CamTraceResult;
	ETraceTypeQuery const CamChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, CamTraceEnd, CamChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::SphereTraceMulti(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceRadius, TraceChannel, false, ActorsToIgnore,
		EDrawDebugTrace::None, Results, true, FLinearColor::Yellow);

	OutTargetSet.SetID = TargetSetID;
	for (FHitResult const& Result : Results)
	{
		if (IsValid(Result.GetActor()))
		{
			OutTargetSet.Targets.Add(Result.GetActor());
		}
	}
	
	return OutTargetSet.Targets.Num() > 0;
}

TArray<AActor*> UAbilityFunctionLibrary::ValidateMultiSphereTrace(ASaiyoraPlayerCharacter* Shooter,
	FAbilityOrigin const& Origin, TArray<AActor*> const& Targets, float const TraceLength, float const TraceRadius,
	bool const bHostile, TArray<AActor*> const& ActorsToIgnore)
{
	TArray<AActor*> ValidatedTargets;
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || Targets.Num() == 0 || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return ValidatedTargets;
	}
	if (Shooter->IsLocallyControlled())
	{
		//If the prediction was done locally, we don't need to validate.
		return Targets;
	}
	//TODO: Validate aim location and origin (if using separate origin).
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, Targets, ActorsToIgnore, ReturnTransforms);
	//Trace from the camera for a very large distance to find what we were aiming at.
	//TODO: Math to find distance and angle from origin point so that we can calculate the max distance we can trace before we outrange the origin.
	FVector const CamTraceEnd = Origin.AimLocation + (CamTraceLength * Origin.AimDirection.GetSafeNormal());
	FHitResult CamTraceResult;
	ETraceTypeQuery const CamChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, CamTraceEnd,
		CamChannel, false, ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();
	TArray<FHitResult> Results;
	//For the actual multi-trace, we use a channel that hitboxes will overlap instead of block (PlayerFriendlyOverlap or PlayerHostileOverlap) so that we get multiple results.
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::SphereTraceMulti(Shooter, Origin.Origin, OriginTraceEnd, TraceRadius, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, Results, true, FLinearColor::Yellow);
	
	for (FHitResult const& Result : Results)
	{
		if (IsValid(Result.GetActor()) && Targets.Contains(Result.GetActor()))
		{
			ValidatedTargets.AddUnique(Result.GetActor());
		}
	}
	UnrewindHitboxes(ReturnTransforms);
	return ValidatedTargets;
}

bool UAbilityFunctionLibrary::PredictSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, float const TraceLength,
                                                      float const TraceRadius, bool const bHostile, TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID,
                                                      FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Result = FHitResult();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}
	FName AimSocket;
	USceneComponent* AimComponent = ISaiyoraCombatInterface::Execute_GetAimSocket(Shooter, AimSocket);
	FName OriginSocket;
	USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(Shooter, OriginSocket);
	
	if (IsValid(AimComponent))
	{
		OutOrigin.AimLocation = AimComponent->GetSocketLocation(AimSocket);
		OutOrigin.AimDirection = AimComponent->GetForwardVector();
		OutOrigin.Origin = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : OutOrigin.AimLocation;
	}
	else if (IsValid(OriginComponent))
	{
		OutOrigin.Origin = OriginComponent->GetSocketLocation(OriginSocket);
		OutOrigin.AimLocation = OutOrigin.Origin;
		OutOrigin.AimDirection = OriginComponent->GetForwardVector();
	}
	else
	{
		OutOrigin.Origin = Shooter->GetActorLocation();
		OutOrigin.AimLocation = Shooter->GetActorLocation();
		OutOrigin.AimDirection = Shooter->GetActorForwardVector();
	}

	FVector const CamTraceEnd = OutOrigin.AimLocation + (CamTraceLength * OutOrigin.AimDirection);
	FHitResult CamTraceResult;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	FHitResult OriginTraceResult;
	//Find the point where line of sight from the origin is blocked (if there is one).
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginTraceResult, true, FLinearColor::Red);

	//Perform the actual sphere trace to the point where line of sight was blocked (or our original trace target if it wasn't blocked).
	FVector const SphereTraceEnd = OriginTraceResult.bBlockingHit ? OriginTraceResult.ImpactPoint : OriginTraceEnd;
	TArray<FHitResult> SphereTraceResults;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(bHostile ? ECC_GameTraceChannel11 : ECC_GameTraceChannel10));
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, OutOrigin.Origin, SphereTraceEnd, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::None, SphereTraceResults, true, FLinearColor::Yellow);

	bool bValidResult = false;
	for (FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (SphereTraceResult.bBlockingHit && IsValid(SphereTraceResult.GetActor()))
		{
			//Check line of sight to the actor hit. The first actor that is hit that is also in LoS will be our target.
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.Origin, SphereTraceResult.ImpactPoint, TraceChannel, false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true, FLinearColor::Blue);
			if (!LineOfSightResult.bBlockingHit)
			{
				bValidResult = true;
				Result = SphereTraceResult;
				break;
			}
		}
	}
	if (!bValidResult)
	{
		//If we didn't get any valid targets matching our object type, use the end of the original line of sight trace as our hit result.
		Result = OriginTraceResult;
	}

	OutTargetSet.SetID = TargetSetID;
	if (Result.bBlockingHit && IsValid(Result.GetActor()))
	{
		OutTargetSet.Targets.Add(Result.GetActor());
	}
	
	return Result.bBlockingHit;
}

bool UAbilityFunctionLibrary::ValidateSphereSightTrace(ASaiyoraPlayerCharacter* Shooter,
	FAbilityOrigin const& Origin, AActor* Target, float const TraceLength, float const TraceRadius, bool const bHostile,
	TArray<AActor*> const& ActorsToIgnore)
{
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || !IsValid(Target) || Shooter == Target
		|| ActorsToIgnore.Contains(Target) || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}
	if (Shooter->IsLocallyControlled())
	{
		//If the prediction was done locally, we don't need to validate.
		return true;
	}
	//TODO: Validate aim location and origin (if using separate origin).
	TArray<AActor*> TargetArray;
	TargetArray.Add(Target);
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, TargetArray, ActorsToIgnore, ReturnTransforms);
	FVector const CamTraceEnd = Origin.AimLocation + (CamTraceLength * Origin.AimDirection);
	FHitResult CamTraceResult;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();
	FHitResult OriginTraceResult;
	//Find the point where line of sight from the origin is blocked (if there is one).
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.Origin, OriginTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginTraceResult, true, FLinearColor::Red);

	//Perform the actual sphere trace to the point where line of sight was blocked (or our original trace target if it wasn't blocked).
	FVector const SphereTraceEnd = OriginTraceResult.bBlockingHit ? OriginTraceResult.ImpactPoint : OriginTraceEnd;
	TArray<FHitResult> SphereTraceResults;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(bHostile ? ECC_GameTraceChannel11 : ECC_GameTraceChannel10));
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.Origin, SphereTraceEnd, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::None, SphereTraceResults, true, FLinearColor::Yellow);

	bool bDidHit = false;
	for (FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (SphereTraceResult.bBlockingHit && IsValid(SphereTraceResult.GetActor()) && SphereTraceResult.GetActor() == Target)
		{
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.Origin, SphereTraceResult.ImpactPoint, TraceChannel, false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true, FLinearColor::Blue);
			if (!LineOfSightResult.bBlockingHit)
			{
				bDidHit = true;
			}
			break;
		}
	}
	if (!bDidHit)
	{
		if (OriginTraceResult.bBlockingHit && IsValid(OriginTraceResult.GetActor()) && OriginTraceResult.GetActor() == Target)
		{
			//The passed target was the result of using the LoS check on the client, and matches what the server's LoS check hits.
			//This is NOT a target of the desired object type, but likely a blocking wall or obstacle. This is still correct (in that it matches the client's prediction).
			bDidHit = true;
		}
	}
	UnrewindHitboxes(ReturnTransforms);
	return bDidHit;
}

bool UAbilityFunctionLibrary::PredictMultiSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, float const TraceLength,
	float const TraceRadius, bool const bHostile, TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID,
	TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Results.Empty();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}
	FName AimSocket;
	USceneComponent* AimComponent = ISaiyoraCombatInterface::Execute_GetAimSocket(Shooter, AimSocket);
	FName OriginSocket;
	USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(Shooter, OriginSocket);
	
	if (IsValid(AimComponent))
	{
		OutOrigin.AimLocation = AimComponent->GetSocketLocation(AimSocket);
		OutOrigin.AimDirection = AimComponent->GetForwardVector();
		OutOrigin.Origin = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : OutOrigin.AimLocation;
	}
	else if (IsValid(OriginComponent))
	{
		OutOrigin.Origin = OriginComponent->GetSocketLocation(OriginSocket);
		OutOrigin.AimLocation = OutOrigin.Origin;
		OutOrigin.AimDirection = OriginComponent->GetForwardVector();
	}
	else
	{
		OutOrigin.Origin = Shooter->GetActorLocation();
		OutOrigin.AimLocation = Shooter->GetActorLocation();
		OutOrigin.AimDirection = Shooter->GetActorForwardVector();
	}

	FVector const CamTraceEnd = OutOrigin.AimLocation + (CamTraceLength * OutOrigin.AimDirection);
	FHitResult CamTraceResult;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	FHitResult OriginTraceResult;
	//Find the point where line of sight from the origin is blocked (if there is one).
	ETraceTypeQuery const OriginTraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.Origin, OriginTraceEnd, OriginTraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginTraceResult, true, FLinearColor::Red);

	//Perform the actual sphere trace to the point where line of sight was blocked (or our original trace target if it wasn't blocked).
	FVector const SphereTraceEnd = OriginTraceResult.bBlockingHit ? OriginTraceResult.ImpactPoint : OriginTraceEnd;
	TArray<FHitResult> SphereTraceResults;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(bHostile ? ECC_GameTraceChannel11 : ECC_GameTraceChannel10));
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, OutOrigin.Origin, SphereTraceEnd, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::None, SphereTraceResults, true, FLinearColor::Yellow);

	for (FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (IsValid(SphereTraceResult.GetActor()))
		{
			//Check line of sight to the actor hit. The first actor that is hit that is also in LoS will be our target.
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.Origin, SphereTraceResult.ImpactPoint, OriginTraceChannel, false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true, FLinearColor::Blue);
			if (!LineOfSightResult.bBlockingHit)
			{
				Results.Add(SphereTraceResult);
			}
		}
	}
	if (IsValid(OriginTraceResult.GetActor()))
	{
		//Include whatever stopped our trace, usually a wall or obstacle.
		Results.Add(OriginTraceResult);
	}

	OutTargetSet.SetID = TargetSetID;
	for (FHitResult const& Hit : Results)
	{
		if (IsValid(Hit.GetActor()))
		{
			OutTargetSet.Targets.AddUnique(Hit.GetActor());
		}
	}
	
	return OutTargetSet.Targets.Num() > 0;
}

TArray<AActor*> UAbilityFunctionLibrary::ValidateMultiSphereSightTrace(ASaiyoraPlayerCharacter* Shooter,
	FAbilityOrigin const& Origin, TArray<AActor*> const& Targets, float const TraceLength, float const TraceRadius,
	bool const bHostile, TArray<AActor*> const& ActorsToIgnore)
{
	TArray<AActor*> ValidatedTargets;
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || Targets.Num() == 0 || Targets.Contains(Shooter)
		|| TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return ValidatedTargets;
	}
	if (Shooter->IsLocallyControlled())
	{
		//If the prediction was done locally, we don't need to validate.
		return Targets;
	}
	//TODO: Validate aim location and origin (if using separate origin).
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, Targets, ActorsToIgnore, ReturnTransforms);
	FVector const CamTraceEnd = Origin.AimLocation + (CamTraceLength * Origin.AimDirection);
	FHitResult CamTraceResult;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel4 : ECC_GameTraceChannel3);
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true, FLinearColor::Green);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();
	ETraceTypeQuery const OriginTraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	FHitResult OriginTraceResult;
	//Find the point where line of sight from the origin is blocked (if there is one).
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.Origin, OriginTraceEnd, OriginTraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginTraceResult, true, FLinearColor::Red);

	//Perform the actual sphere trace to the point where line of sight was blocked (or our original trace target if it wasn't blocked).
	FVector const SphereTraceEnd = OriginTraceResult.bBlockingHit ? OriginTraceResult.ImpactPoint : OriginTraceEnd;
	TArray<FHitResult> SphereTraceResults;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(bHostile ? ECC_GameTraceChannel11 : ECC_GameTraceChannel10));
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.Origin, SphereTraceEnd, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::None, SphereTraceResults, true, FLinearColor::Yellow);

	for (FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (IsValid(SphereTraceResult.GetActor()) && Targets.Contains(SphereTraceResult.GetActor()))
		{
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.Origin, SphereTraceResult.ImpactPoint, OriginTraceChannel, false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true, FLinearColor::Blue);
			if (!LineOfSightResult.bBlockingHit)
			{
				ValidatedTargets.AddUnique(SphereTraceResult.GetActor());
			}
		}
	}
	if (IsValid(OriginTraceResult.GetActor()) && Targets.Contains(OriginTraceResult.GetActor()))
	{
		//Include the final blocking hit to our origin trace, which won't be of the correct type but will probably be a wall or something.
		ValidatedTargets.AddUnique(OriginTraceResult.GetActor());
	}
	UnrewindHitboxes(ReturnTransforms);
	return ValidatedTargets;
}

#pragma endregion
#pragma region Projectiles

APredictableProjectile* UAbilityFunctionLibrary::PredictProjectile(UCombatAbility* Ability,
	ASaiyoraPlayerCharacter* Shooter, TSubclassOf<APredictableProjectile> const ProjectileClass, FAbilityOrigin& OutOrigin)
{
	if (!IsValid(Ability) || !IsValid(Shooter) || !Shooter->IsLocallyControlled() || !IsValid(ProjectileClass))
	{
		return nullptr;
	}
	FName AimSocket;
	USceneComponent* AimComponent = ISaiyoraCombatInterface::Execute_GetAimSocket(Shooter, AimSocket);
	FName OriginSocket;
	USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(Shooter, OriginSocket);
	if (IsValid(AimComponent))
	{
		OutOrigin.AimLocation = AimComponent->GetSocketLocation(AimSocket);
		OutOrigin.AimDirection = AimComponent->GetForwardVector();
		OutOrigin.Origin = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : OutOrigin.AimLocation;
	}
	else if (IsValid(OriginComponent))
	{
		OutOrigin.Origin = OriginComponent->GetSocketLocation(OriginSocket);
		OutOrigin.AimLocation = OutOrigin.Origin;
		OutOrigin.AimDirection = OriginComponent->GetForwardVector();
	}
	else
	{
		OutOrigin.Origin = Shooter->GetActorLocation();
		OutOrigin.AimLocation = Shooter->GetActorLocation();
		OutOrigin.AimDirection = Shooter->GetActorForwardVector();
	}
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(OutOrigin.Origin);
	//Choose very far point straight from the camera as the default aim target.
	FVector const VisTraceEnd = OutOrigin.AimLocation + OutOrigin.AimDirection * CamTraceLength;
	//Get a vector representing aiming from the origin to this aim target.
	FVector const OriginAimDirection = (VisTraceEnd - OutOrigin.Origin).GetSafeNormal();
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11));
	TArray<FHitResult> VisTraceHits;
	FVector VisTraceResult = VisTraceEnd;
	//Trace against both friendly and enemy hitbox objects to find what we're aiming at along the line from the camera.
	UKismetSystemLibrary::LineTraceMultiForObjects(Shooter, OutOrigin.AimLocation, VisTraceEnd, ObjectTypes, false, TArray<AActor*>(), EDrawDebugTrace::None, VisTraceHits, true);
	for (FHitResult const& VisTraceHit : VisTraceHits)
	{
		//Of all the things in the line, we are looking for the first instance of a target within a 10 degree cone of the original aim vector.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceHit.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OriginAimDirection));
		if (Angle < AimToleranceDegrees)
		{
			VisTraceResult = VisTraceHit.ImpactPoint;
			break;
		}
	}
	//If we didn't find anything along the line that fits in the cone, just use the unaltered aim direction.
	SpawnTransform.SetRotation((VisTraceResult - OutOrigin.Origin).Rotation().Quaternion());
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Shooter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APredictableProjectile* NewProjectile = Shooter->GetWorld()->SpawnActor<APredictableProjectile>(ProjectileClass, SpawnTransform, SpawnParams);
	if (!IsValid(NewProjectile))
	{
		return nullptr;
	}
	NewProjectile->InitializeProjectile(Ability);
	return NewProjectile;
}

APredictableProjectile* UAbilityFunctionLibrary::ValidateProjectile(UCombatAbility* Ability,
	ASaiyoraPlayerCharacter* Shooter, TSubclassOf<APredictableProjectile> const ProjectileClass,
	FAbilityOrigin const& Origin)
{
	if (!IsValid(Ability) || !IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || !IsValid(ProjectileClass))
	{
		//TODO: If this fails, we probably need some way to let the client know his projectile isn't real...
		//Potentially spawn a "dummy" projectile that is invisible, that will replace the client projectile with nothing then destroy itself.
		return nullptr;
	}
	float const Ping = USaiyoraCombatLibrary::GetActorPing(Shooter);
	ASaiyoraGameState* GameState = Shooter->GetWorld()->GetGameState<ASaiyoraGameState>();
	//Get target's hitboxes, rewind them, and store their actual transforms for un-rewinding.
	TMap<UHitbox*, FTransform> ReturnTransforms;
	//If listen server (ping 0), skip rewinding.
	if (Ping > 0.0f && IsValid(GameState))
	{
		//Do a big trace in front of the camera to rewind all targets that could potentially intercept the trace.
		FVector const RewindTraceEnd = Origin.AimLocation + CamTraceLength * Origin.AimDirection;
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11));
		TArray<FHitResult> RewindTraceResults;
		UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.AimLocation, RewindTraceEnd, RewindTraceRadius, ObjectTypes, false,
			TArray<AActor*>(), EDrawDebugTrace::None, RewindTraceResults, true, FLinearColor::White);
		for (FHitResult const& Hit : RewindTraceResults)
		{
			if (IsValid(Hit.GetActor()))
			{
				TArray<UHitbox*> HitboxComponents;
				Hit.GetActor()->GetComponents<UHitbox>(HitboxComponents);
				for (UHitbox* Hitbox : HitboxComponents)
				{
					ReturnTransforms.Add(Hitbox, GameState->RewindHitbox(Hitbox, Ping));
				}
			}
		}
	}
	//TODO: Validate aim location.
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Origin.Origin);
	//Choose very far point straight from the camera as the default aim target.
	FVector const VisTraceEnd = Origin.AimLocation + Origin.AimDirection * CamTraceLength;
	//Get a vector representing aiming from the origin to this aim target.
	FVector const OriginAimDirection = (VisTraceEnd - Origin.Origin).GetSafeNormal();
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11));
	TArray<FHitResult> VisTraceHits;
	FVector VisTraceResult = VisTraceEnd;
	//Trace against both friendly and enemy hitbox objects to find what we're aiming at along the line from the camera.
	UKismetSystemLibrary::LineTraceMultiForObjects(Shooter, Origin.AimLocation, VisTraceEnd, ObjectTypes, false, TArray<AActor*>(), EDrawDebugTrace::None, VisTraceHits, true);
	for (FHitResult const& VisTraceHit : VisTraceHits)
	{
		//Of all the things in the line, we are looking for the first instance of a target within a 30 degree cone of the original aim vector.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceHit.ImpactPoint - Origin.Origin).GetSafeNormal(), OriginAimDirection));
		if (Angle < AimToleranceDegrees)
		{
			VisTraceResult = VisTraceHit.ImpactPoint;
			break;
		}
	}
	SpawnTransform.SetRotation((VisTraceResult - Origin.Origin).Rotation().Quaternion());
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Shooter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APredictableProjectile* NewProjectile = Shooter->GetWorld()->SpawnActor<APredictableProjectile>(ProjectileClass, SpawnTransform, SpawnParams);
	if (!IsValid(NewProjectile))
	{
		//TODO: Failure?
		return nullptr;
	}
	NewProjectile->InitializeProjectile(Ability);
	//Case A: 213ms
	//Case B: 12ms
	if (Ping > 0.0f && IsValid(GameState))
	{
		UProjectileMovementComponent* MoveComp = NewProjectile->FindComponentByClass<UProjectileMovementComponent>();
		//Calculate how many 50ms steps we need to go through before we'd arrive less than 50ms from current time.
		int32 const Steps = FMath::Floor(Ping / .05f);
		if (Steps == 0)
		{
			//Ping was less than 50ms, we can just do one step against the already rewound hitboxes.
			if (IsValid(MoveComp))
			{
				MoveComp->TickComponent(Ping, ELevelTick::LEVELTICK_All, &MoveComp->PrimaryComponentTick);
			}
			//Unrewind after tick.
			for (TTuple<UHitbox*, FTransform> const& ReturnTransform : ReturnTransforms)
			{
				ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
			}
		}
		else
		{
			//Only concerned with rewinding hitboxes hit with our initial rewind trace, otherwise performance costs could get crazy.
			TArray<UHitbox*> RelevantHitboxes;
			ReturnTransforms.GetKeys(RelevantHitboxes);
			for (int i = 0; i < Steps; i++)
			{
				//Don't need to rewind for the first loop iteration since we are already rewound.
				if (i != 0)
				{
					ReturnTransforms.Empty();
					for (UHitbox* Hitbox : RelevantHitboxes)
					{
						ReturnTransforms.Add(Hitbox, GameState->RewindHitbox(Hitbox, (Ping - (i * .05f))));
					}
				}
				//Tick the projectile through 50ms against rewound targets.
				if (IsValid(MoveComp))
				{
					MoveComp->TickComponent(.05f, ELevelTick::LEVELTICK_All, &MoveComp->PrimaryComponentTick);
				}
				//Unrewind all targets to set up for next rewind.
				for (TTuple<UHitbox*, FTransform> const& ReturnTransform : ReturnTransforms)
				{
					ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
				}
			}
			//Partial step for any remainder less than 50ms at the end.
			float const LastStepMs = Ping - (Steps * .05f);
			ReturnTransforms.Empty();
			for (UHitbox* Hitbox : RelevantHitboxes)
			{
				ReturnTransforms.Add(Hitbox, GameState->RewindHitbox(Hitbox, LastStepMs));
			}
			if (IsValid(MoveComp))
			{
				MoveComp->TickComponent(LastStepMs, ELevelTick::LEVELTICK_All, &MoveComp->PrimaryComponentTick);
			}
			for (TTuple<UHitbox*, FTransform> const& ReturnTransform : ReturnTransforms)
			{
				ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
			}
		}
	}
	return NewProjectile;
}

#pragma endregion 
