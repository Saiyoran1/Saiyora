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
float const UAbilityFunctionLibrary::AimToleranceDegrees = 30.0f;

FAbilityOrigin UAbilityFunctionLibrary::MakeAbilityOrigin(FVector const& AimLocation, FVector const& AimDirection, FVector const& Origin)
{
	FAbilityOrigin Target;
	Target.Origin = Origin;
	Target.AimLocation = AimLocation;
	Target.AimDirection = AimDirection;
	return Target;
}

float UAbilityFunctionLibrary::GetCameraTraceMaxRange(FVector const& CameraLoc, FVector const& AimDir, FVector const& OriginLoc,
    float const TraceRange)
{
	float const CamToOriginLength = (OriginLoc - CameraLoc).Size();
	float const CamAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct((OriginLoc - CameraLoc).GetSafeNormal(), AimDir));
	//Law of sines: sin(A)/A = sin(B)/B
	//sin(CamAngle)/TraceRange = sin(B)/CamToOriginLength
	//B = arcsin((sin(CamAngle) * CamToOriginLength)/TraceRange)
	float const TargetAngle = UKismetMathLibrary::DegAsin((UKismetMathLibrary::DegSin(CamAngle) * CamToOriginLength) / TraceRange);
	float const OriginAngle = 180.0f - (CamAngle + TargetAngle);
	//sin(OriginAngle)/x = sin(CamAngle)/TraceRange
	//sin(OriginAngle) * TraceRange / x = sin(CamAngle)
	//sin(OriginAngle) * TraceRange = x * sin(CamAngle)
	//x = (sin(OriginAngle) * TraceRange) / sin(CamAngle)
	return (UKismetMathLibrary::DegSin(OriginAngle) * TraceRange) / UKismetMathLibrary::DegSin(CamAngle);
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

	//Choose max range point straight from the camera as the default aim target.
	FVector AimTarget = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);

	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = VisTraceResult.ImpactPoint;
		}
	}
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(bHostile ? UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11) : UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	TArray<FHitResult> HitboxTraceHits;
	//Trace against either friendly or enemy hitbox objects to find what we're aiming at along the line from the camera.
	UKismetSystemLibrary::LineTraceMultiForObjects(Shooter, OutOrigin.AimLocation, AimTarget, ObjectTypes, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);
	if (HitboxTraceHits.Num() > 0)
	{
		//Find the last thing in the line we were aiming at. If the angle to this last impact point is within a small cone of our original aim direction, we use that as our aim target.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((HitboxTraceHits.Last().ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = OutOrigin.Origin + (HitboxTraceHits.Last().ImpactPoint - OutOrigin.Origin).GetSafeNormal() * TraceLength;
		}
	}

	//For the actual multi-trace, we use a channel that hitboxes will overlap instead of block (PlayerFriendlyOverlap or PlayerHostileOverlap) so that we get multiple results.
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::LineTraceMulti(Shooter, OutOrigin.Origin, AimTarget, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, Results, true, FLinearColor::Yellow);

	OutTargetSet.SetID = TargetSetID;
	for (FHitResult const& Result : Results)
	{
		if (IsValid(Result.GetActor()))
		{
			OutTargetSet.Targets.AddUnique(Result.GetActor());
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
	//TODO: Math to find distance and angle from origin point so that we can calculate the max distance we can trace before we outrange the origin.
	
	//Choose max range point straight from the camera as the default aim target.
	FVector AimTarget = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);
	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = VisTraceResult.ImpactPoint;
		}
	}
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(bHostile ? UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11) : UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	TArray<FHitResult> HitboxTraceHits;
	//Trace against either friendly or enemy hitbox objects to find what we're aiming at along the line from the camera.
	UKismetSystemLibrary::LineTraceMultiForObjects(Shooter, Origin.AimLocation, AimTarget, ObjectTypes, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);
	if (HitboxTraceHits.Num() > 0)
	{
		//Find the last thing in the line we were aiming at. If the angle to this last impact point is within a small cone of our original aim direction, we use that as our aim target.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((HitboxTraceHits.Last().ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = Origin.Origin + (HitboxTraceHits.Last().ImpactPoint - Origin.Origin).GetSafeNormal() * TraceLength;
		}
	}

	TArray<FHitResult> Results;
	//For the actual multi-trace, we use a channel that hitboxes will overlap instead of block (PlayerFriendlyOverlap or PlayerHostileOverlap) so that we get multiple results.
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::LineTraceMulti(Shooter, Origin.Origin, AimTarget, TraceChannel, false,
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

	FVector const CamTraceEnd = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);
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
	FVector const CamTraceEnd = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);
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

	//Choose max range point straight from the camera as the default aim target.
	FVector AimTarget = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);

	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = VisTraceResult.ImpactPoint;
		}
	}
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(bHostile ? UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11) : UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	TArray<FHitResult> HitboxTraceHits;
	//Trace against either friendly or enemy hitbox objects to find what we're aiming at along the line from the camera.
	UKismetSystemLibrary::LineTraceMultiForObjects(Shooter, OutOrigin.AimLocation, AimTarget, ObjectTypes, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);
	if (HitboxTraceHits.Num() > 0)
	{
		//Find the last thing in the line we were aiming at. If the angle to this last impact point is within a small cone of our original aim direction, we use that as our aim target.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((HitboxTraceHits.Last().ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = OutOrigin.Origin + (HitboxTraceHits.Last().ImpactPoint - OutOrigin.Origin).GetSafeNormal() * TraceLength;
		}
	}

	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::SphereTraceMulti(Shooter, OutOrigin.Origin, AimTarget, TraceRadius, TraceChannel, false, ActorsToIgnore,
		EDrawDebugTrace::ForDuration, Results, true, FLinearColor::Yellow);

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

	//Choose max range point straight from the camera as the default aim target.
	FVector AimTarget = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);

	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = VisTraceResult.ImpactPoint;
		}
	}
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(bHostile ? UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11) : UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	TArray<FHitResult> HitboxTraceHits;
	//Trace against either friendly or enemy hitbox objects to find what we're aiming at along the line from the camera.
	UKismetSystemLibrary::LineTraceMultiForObjects(Shooter, Origin.AimLocation, AimTarget, ObjectTypes, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);
	if (HitboxTraceHits.Num() > 0)
	{
		//Find the last thing in the line we were aiming at. If the angle to this last impact point is within a small cone of our original aim direction, we use that as our aim target.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((HitboxTraceHits.Last().ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = Origin.Origin + (HitboxTraceHits.Last().ImpactPoint - Origin.Origin).GetSafeNormal() * TraceLength;
		}
	}
	
	TArray<FHitResult> Results;
	ETraceTypeQuery const TraceChannel = UEngineTypes::ConvertToTraceType(bHostile ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
	UKismetSystemLibrary::SphereTraceMulti(Shooter, Origin.Origin, AimTarget, TraceRadius, TraceChannel, false,
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

	FVector const CamTraceEnd = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);
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
	FVector const CamTraceEnd = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);
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

	//Choose max range point straight from the camera as the default aim target.
	FVector AimTarget = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);

	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = VisTraceResult.ImpactPoint;
		}
	}
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(bHostile ? UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11) : UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	TArray<FHitResult> HitboxTraceHits;
	//Trace against either friendly or enemy hitbox objects to find what we're aiming at along the line from the camera.
	UKismetSystemLibrary::LineTraceMultiForObjects(Shooter, OutOrigin.AimLocation, AimTarget, ObjectTypes, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);
	if (HitboxTraceHits.Num() > 0)
	{
		//Find the last thing in the line we were aiming at. If the angle to this last impact point is within a small cone of our original aim direction, we use that as our aim target.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((HitboxTraceHits.Last().ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = OutOrigin.Origin + (HitboxTraceHits.Last().ImpactPoint - OutOrigin.Origin).GetSafeNormal() * TraceLength;
		}
	}

	TArray<FHitResult> SphereTraceResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, OutOrigin.Origin, AimTarget, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, SphereTraceResults, true, FLinearColor::Yellow);

	for (FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (IsValid(SphereTraceResult.GetActor()))
		{
			//Check line of sight to the actor hit. The first actor that is hit that is also in LoS will be our target.
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.Origin, SphereTraceResult.ImpactPoint, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
				LineOfSightIgnoreActors, EDrawDebugTrace::ForDuration, LineOfSightResult, true, FLinearColor::Blue);
			if (!LineOfSightResult.bBlockingHit)
			{
				Results.Add(SphereTraceResult);
			}
		}
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
	
	//Choose max range point straight from the camera as the default aim target.
	FVector AimTarget = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);

	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = VisTraceResult.ImpactPoint;
		}
	}
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(bHostile ? UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11) : UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	TArray<FHitResult> HitboxTraceHits;
	//Trace against either friendly or enemy hitbox objects to find what we're aiming at along the line from the camera.
	UKismetSystemLibrary::LineTraceMultiForObjects(Shooter, Origin.AimLocation, AimTarget, ObjectTypes, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);
	if (HitboxTraceHits.Num() > 0)
	{
		//Find the last thing in the line we were aiming at. If the angle to this last impact point is within a small cone of our original aim direction, we use that as our aim target.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((HitboxTraceHits.Last().ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = Origin.Origin + (HitboxTraceHits.Last().ImpactPoint - Origin.Origin).GetSafeNormal() * TraceLength;
		}
	}
	
	TArray<FHitResult> SphereTraceResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.Origin, AimTarget, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::None, SphereTraceResults, true, FLinearColor::Yellow);

	for (FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (IsValid(SphereTraceResult.GetActor()) && Targets.Contains(SphereTraceResult.GetActor()))
		{
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.Origin, SphereTraceResult.ImpactPoint, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true, FLinearColor::Blue);
			if (!LineOfSightResult.bBlockingHit)
			{
				ValidatedTargets.AddUnique(SphereTraceResult.GetActor());
			}
		}
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
	if (Shooter->GetLocalRole() == ROLE_Authority)
	{
		return nullptr;
	}
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(OutOrigin.Origin);
	//Choose very far point straight from the camera as the default aim target.
	FVector AimTarget = OutOrigin.AimLocation + OutOrigin.AimDirection * CamTraceLength;
	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = VisTraceResult.ImpactPoint;
		}
	}
	SpawnTransform.SetRotation((AimTarget - OutOrigin.Origin).Rotation().Quaternion());
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
	FVector AimTarget = Origin.AimLocation + Origin.AimDirection * CamTraceLength;
	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		float const Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
		if (Angle < AimToleranceDegrees)
		{
			AimTarget = VisTraceResult.ImpactPoint;
		}
	}
	SpawnTransform.SetRotation((AimTarget - Origin.Origin).Rotation().Quaternion());
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
