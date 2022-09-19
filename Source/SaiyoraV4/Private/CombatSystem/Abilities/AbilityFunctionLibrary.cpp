#include "AbilityFunctionLibrary.h"
#include "CombatAbility.h"
#include "CombatStatusComponent.h"
#include "Hitbox.h"
#include "PredictableProjectile.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "CoreClasses/SaiyoraGameState.h"
#include "CoreClasses/SaiyoraPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

float const UAbilityFunctionLibrary::CAMTRACELENGTH = 10000.0f;
float const UAbilityFunctionLibrary::REWINDTRACERADIUS = 300.0f;
float const UAbilityFunctionLibrary::AIMTOLERANCEDEGREES = 15.0f;

#pragma region Helpers

FAbilityOrigin UAbilityFunctionLibrary::MakeAbilityOrigin(const FVector& AimLocation, const FVector& AimDirection, const FVector& Origin)
{
	FAbilityOrigin Target;
	Target.Origin = Origin;
	Target.AimLocation = AimLocation;
	Target.AimDirection = AimDirection;
	return Target;
}

void UAbilityFunctionLibrary::GenerateOriginInfo(const ASaiyoraPlayerCharacter* Shooter, FAbilityOrigin& OutOrigin)
{
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
}

float UAbilityFunctionLibrary::GetCameraTraceMaxRange(const FVector& CameraLoc, const FVector& AimDir, const FVector& OriginLoc,
    const float TraceRange)
{
	const float CamToOriginLength = (OriginLoc - CameraLoc).Size();
	const float CamAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct((OriginLoc - CameraLoc).GetSafeNormal(), AimDir));
	//Law of sines: sin(A)/A = sin(B)/B
	//sin(CamAngle)/TraceRange = sin(B)/CamToOriginLength
	//B = arcsin((sin(CamAngle) * CamToOriginLength)/TraceRange)
	const float TargetAngle = UKismetMathLibrary::DegAsin((UKismetMathLibrary::DegSin(CamAngle) * CamToOriginLength) / TraceRange);
	const float OriginAngle = 180.0f - (CamAngle + TargetAngle);
	//sin(OriginAngle)/x = sin(CamAngle)/TraceRange
	//sin(OriginAngle) * TraceRange / x = sin(CamAngle)
	//sin(OriginAngle) * TraceRange = x * sin(CamAngle)
	//x = (sin(OriginAngle) * TraceRange) / sin(CamAngle)
	return (UKismetMathLibrary::DegSin(OriginAngle) * TraceRange) / UKismetMathLibrary::DegSin(CamAngle);
}

FName UAbilityFunctionLibrary::GetRelevantTraceProfile(const ASaiyoraPlayerCharacter* Shooter, const bool bOverlap, const ESaiyoraPlane TracePlane,
	const EFaction TraceHostility)
{
	const UCombatStatusComponent* ShooterCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Shooter);
	const EFaction ShooterFaction = IsValid(ShooterCombatStatus) ? ShooterCombatStatus->GetCurrentFaction() : EFaction::Neutral;
	
	if (bOverlap)
	{
		switch (TraceHostility)
		{
			case EFaction::Friendly :
			{
				switch (TracePlane)
				{
				case ESaiyoraPlane::Ancient :
					return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_AncientOverlapPlayers : FSaiyoraCollision::CT_AncientOverlapNPCs;
				case ESaiyoraPlane::Modern :
					return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_ModernOverlapPlayers : FSaiyoraCollision::CT_ModernOverlapNPCs;
				default :
					return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_OverlapPlayers : FSaiyoraCollision::CT_OverlapNPCs;
				}
			}
			case EFaction::Enemy :
			{
				switch (TracePlane)
				{
				case ESaiyoraPlane::Ancient :
					return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_AncientOverlapNPCs : FSaiyoraCollision::CT_AncientOverlapPlayers;
				case ESaiyoraPlane::Modern :
					return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_ModernOverlapNPCs : FSaiyoraCollision::CT_ModernOverlapPlayers;
				default :
					return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_OverlapNPCs : FSaiyoraCollision::CT_OverlapPlayers;
				}
			}
			default :
			{
				switch (TracePlane)
				{
				case ESaiyoraPlane::Ancient :
					return FSaiyoraCollision::CT_AncientOverlapAll;
				case ESaiyoraPlane::Modern :
					return FSaiyoraCollision::CT_ModernOverlapAll;
				default :
					return FSaiyoraCollision::CT_OverlapAll;
				}
			}
		}
	}
	switch (TraceHostility)
	{
		case EFaction::Friendly :
		{
			switch (TracePlane)
			{
			case ESaiyoraPlane::Ancient :
				return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_AncientPlayers : FSaiyoraCollision::CT_AncientNPCs;
			case ESaiyoraPlane::Modern :
				return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_ModernPlayers : FSaiyoraCollision::CT_ModernNPCs;
			default :
				return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_Players : FSaiyoraCollision::CT_NPCs;
			}
		}
		case EFaction::Enemy :
		{
			switch (TracePlane)
			{
			case ESaiyoraPlane::Ancient :
				return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_AncientNPCs : FSaiyoraCollision::CT_AncientPlayers;
			case ESaiyoraPlane::Modern :
				return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_ModernNPCs : FSaiyoraCollision::CT_ModernPlayers;
			default :
				return ShooterFaction == EFaction::Friendly ? FSaiyoraCollision::CT_NPCs : FSaiyoraCollision::CT_Players;
			}
		}
		default :
		{
			switch (TracePlane)
			{
			case ESaiyoraPlane::Ancient :
				return FSaiyoraCollision::CT_AncientAll;
			case ESaiyoraPlane::Modern :
				return FSaiyoraCollision::CT_ModernAll;
			default :
				return FSaiyoraCollision::CT_All;
			}
		}
	}
}

void UAbilityFunctionLibrary::GetRelevantHitboxObjectTypes(const ASaiyoraPlayerCharacter* Shooter,
	const EFaction TraceHostility, TArray<TEnumAsByte<EObjectTypeQuery>>& OutObjectTypes)
{
	UCombatStatusComponent* CombatStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(Shooter);
	const EFaction ShooterFaction = IsValid(CombatStatusComponent) ? CombatStatusComponent->GetCurrentFaction() : EFaction::Neutral;
	switch (ShooterFaction)
	{
		case EFaction::Friendly :
		{
			switch (TraceHostility)
			{
				case EFaction::Friendly :
					OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_PlayerHitbox));
					return;
				case EFaction::Enemy :
					OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_NPCHitbox));
					return;
				default :
					OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_PlayerHitbox));
					OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_NPCHitbox));
					return;
			}
		}
		case EFaction::Enemy :
		{
			switch (TraceHostility)
			{
			case EFaction::Friendly :
				OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_NPCHitbox));
				
				return;
			case EFaction::Enemy :
				OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_PlayerHitbox));
				return;
			default :
				OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_PlayerHitbox));
				OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_NPCHitbox));
				return;
			}
		}
		default :
			OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_PlayerHitbox));
			OutObjectTypes.Add(UEngineTypes::ConvertToObjectType(FSaiyoraCollision::O_NPCHitbox));
	}
}

#pragma endregion 
#pragma region Snapshotting

void UAbilityFunctionLibrary::RewindRelevantHitboxes(const ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, const TArray<AActor*>& Targets,
                                                     const TArray<AActor*>& ActorsToIgnore, TMap<UHitbox*, FTransform>& ReturnTransforms)
{
	if (!IsValid(Shooter))
	{
		return;
	}
	const float Ping = USaiyoraCombatLibrary::GetActorPing(Shooter);
	ASaiyoraGameState* GameState = Shooter->GetWorld()->GetGameState<ASaiyoraGameState>();
	if (Ping <= 0.0f || !IsValid(GameState))
	{
		return;
	}
	//Do a big trace in front of the camera to rewind all targets that could potentially intercept the trace.
	const FVector RewindTraceEnd = Origin.AimLocation + CAMTRACELENGTH * Origin.AimDirection;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11));
	TArray<FHitResult> RewindTraceResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.AimLocation, RewindTraceEnd, REWINDTRACERADIUS, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::None, RewindTraceResults, true, FLinearColor::White);
	//All targets we are interested in should also be rewound, even if they weren't in the trace.
	TArray<AActor*> RewindTargets = Targets;
	for (const FHitResult& Hit : RewindTraceResults)
	{
		if (IsValid(Hit.GetActor()))
		{
			RewindTargets.AddUnique(Hit.GetActor());
		}
	}
	for (const AActor* RewindTarget : RewindTargets)
	{
		TArray<UHitbox*> HitboxComponents;
		RewindTarget->GetComponents<UHitbox>(HitboxComponents);
		for (UHitbox* Hitbox : HitboxComponents)
		{
			ReturnTransforms.Add(Hitbox, GameState->RewindHitbox(Hitbox, Ping));
		}
	}
}

void UAbilityFunctionLibrary::UnrewindHitboxes(const TMap<UHitbox*, FTransform>& ReturnTransforms)
{
	for (const TTuple<UHitbox*, FTransform>& ReturnTransform : ReturnTransforms)
	{
		if (IsValid(ReturnTransform.Key))
		{
			ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
		}
	}
}

#pragma endregion 
#pragma region Trace Validation

bool UAbilityFunctionLibrary::PredictLineTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength,
	const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID,
	FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Result = FHitResult();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f)
	{
		return false;
	}
	
	GenerateOriginInfo(Shooter, OutOrigin);

	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	const FVector CamTraceEnd = OutOrigin.AimLocation + (GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength) * OutOrigin.AimDirection);
	//This profile will collide with relevant hitboxes and level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, false, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	FHitResult CamTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true);

	//If there is a hit, check that it is in "front" of the origin (not between the origin and aim location). If so, use this as the new trace end.
	const FVector OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, Result, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	OutTargetSet.SetID = TargetSetID;
	if (IsValid(Result.GetActor()))
	{
		OutTargetSet.Targets.Add(Result.GetActor());
	}
	
	return Result.bBlockingHit;
}

bool UAbilityFunctionLibrary::ValidateLineTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin, AActor* Target,
	const float TraceLength, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore)
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

	//Rewind hitboxes of all predicted targets and other actors in the way.
	TArray<AActor*> TargetArray;
	TargetArray.Add(Target);
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, TargetArray, ActorsToIgnore, ReturnTransforms);
	
	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	const FVector CamTraceEnd = Origin.AimLocation + (GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength) * Origin.AimDirection.GetSafeNormal());
	//This profile will collide with relevant hitboxes and level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, false, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	FHitResult CamTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, Origin.AimLocation, CamTraceEnd, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true);

	//If there is a hit, check that it is in "front" of the origin (not between the origin and aim location). If so, use this as the new trace end.
	const FVector OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();
	
	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	FHitResult OriginResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, Origin.Origin, OriginTraceEnd, TraceProfile, false, ActorsToIgnore,
		EDrawDebugTrace::ForDuration, OriginResult, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	//Validate the predicted hit.
	bool bDidHit = false;
	if (IsValid(OriginResult.GetActor()) && OriginResult.GetActor() == Target)
	{
		bDidHit = true;
	}
	
	//Unrewind any hitboxes that were rewound previously.
	UnrewindHitboxes(ReturnTransforms);
	
	return bDidHit;
}

bool UAbilityFunctionLibrary::PredictMultiLineTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength, const ESaiyoraPlane TracePlane,
	const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID, TArray<FHitResult>& Results,
	FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Results.Empty();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f)
	{
		return false;
	}

	GenerateOriginInfo(Shooter, OutOrigin);

	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	FVector AimTarget = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);
	//This profile will overlap relevant hitboxes and collide with relevant level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, true, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	TArray<FHitResult> HitboxTraceHits;
	UKismetSystemLibrary::LineTraceMultiByProfile(Shooter, OutOrigin.AimLocation, AimTarget, TraceProfile, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);

	//Find the first hit that is within a reasonable cone in front of the origin.
	//Note that this is more strict than just "in front," because multi-traces tend to be aimed at groups of targets, and large skewed angles can result in misses on some of those targets.
	if (HitboxTraceHits.Num() > 0)
	{
		for (const FHitResult& TraceResult : HitboxTraceHits)
		{
			const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((TraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
			if (Angle < AIMTOLERANCEDEGREES)
			{
				AimTarget = OutOrigin.Origin + (TraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal() * TraceLength;
				break;
			}
		}
	}
	
	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	UKismetSystemLibrary::LineTraceMultiByProfile(Shooter, OutOrigin.Origin, AimTarget, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, Results, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	OutTargetSet.SetID = TargetSetID;
	for (const FHitResult& Result : Results)
	{
		if (IsValid(Result.GetActor()))
		{
			OutTargetSet.Targets.AddUnique(Result.GetActor());
		}
	}
	
	return OutTargetSet.Targets.Num() > 0;
}

TArray<AActor*> UAbilityFunctionLibrary::ValidateMultiLineTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin,
	const TArray<AActor*>& Targets, const float TraceLength, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore)
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

	//Rewind hitboxes of all predicted targets and other actors in the way.
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, Targets, ActorsToIgnore, ReturnTransforms);
	
	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	FVector AimTarget = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);
	//This profile will overlap relevant hitboxes and collide with relevant level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, true, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	TArray<FHitResult> HitboxTraceHits;
	UKismetSystemLibrary::LineTraceMultiByProfile(Shooter, Origin.AimLocation, AimTarget, TraceProfile, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);

	//Find the first hit that is within a reasonable cone in front of the origin.
	//Note that this is more strict than just "in front," because multi-traces tend to be aimed at groups of targets, and large skewed angles can result in misses on some of those targets.
	if (HitboxTraceHits.Num() > 0)
	{
		for (const FHitResult& TraceResult : HitboxTraceHits)
		{
			//Find the first thing in the line we hit that doesn't skew the angle outside of a small cone relative to the origin.
			const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((TraceResult.ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
			if (Angle < AIMTOLERANCEDEGREES)
			{
				AimTarget = Origin.Origin + (TraceResult.ImpactPoint - Origin.Origin).GetSafeNormal() * TraceLength;
				break;
			}
		}
	}

	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	TArray<FHitResult> Results;
	UKismetSystemLibrary::LineTraceMultiByProfile(Shooter, Origin.Origin, AimTarget, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, Results, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	//Validate predicted hits.
	for (const FHitResult& Result : Results)
	{
		if (IsValid(Result.GetActor()) && Targets.Contains(Result.GetActor()))
		{
			ValidatedTargets.AddUnique(Result.GetActor());
		}
	}
	
	//Unrewind any hitboxes that were rewound previously.
	UnrewindHitboxes(ReturnTransforms);
	
	return ValidatedTargets;
}

bool UAbilityFunctionLibrary::PredictSphereTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength,
	const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID,
	FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Result = FHitResult();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}

	GenerateOriginInfo(Shooter, OutOrigin);

	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	const FVector CamTraceEnd = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);
	//This profile will collide with relevant hitboxes and level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, false, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	FHitResult CamTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true);

	//If there is a hit, check that it is in "front" of the origin (not between the origin and aim location). If so, use this as the new trace end.
	const FVector OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();

	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	UKismetSystemLibrary::SphereTraceSingleByProfile(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceRadius, TraceProfile, false, ActorsToIgnore,
		EDrawDebugTrace::ForDuration, Result, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	OutTargetSet.SetID = TargetSetID;
	if (IsValid(Result.GetActor()))
	{
		OutTargetSet.Targets.Add(Result.GetActor());
	}
	
	return Result.bBlockingHit;
}

bool UAbilityFunctionLibrary::ValidateSphereTrace(ASaiyoraPlayerCharacter* Shooter, const FAbilityOrigin& Origin,
	AActor* Target, const float TraceLength, const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility,
	const TArray<AActor*>& ActorsToIgnore)
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

	//Rewind hitboxes of all predicted targets and other actors in the way.
	TArray<AActor*> TargetArray;
	TargetArray.Add(Target);
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, TargetArray, ActorsToIgnore, ReturnTransforms);
	
	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	const FVector CamTraceEnd = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);
	//This profile will collide with relevant hitboxes and level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, false, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	FHitResult CamTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, Origin.AimLocation, CamTraceEnd,
		TraceProfile, false, ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true);

	//If there is a hit, check that it is in "front" of the origin (not between the origin and aim location). If so, use this as the new trace end.
	const FVector OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();

	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	FHitResult OriginResult;
	UKismetSystemLibrary::SphereTraceSingleByProfile(Shooter, Origin.Origin, OriginTraceEnd, TraceRadius, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, OriginResult, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	//Validate the predicted hit.
	bool bDidHit = false;
	if (IsValid(OriginResult.GetActor()) && OriginResult.GetActor() == Target)
	{
		bDidHit = true;
	}

	//Unrewind any hitboxes that were rewound previously.
	UnrewindHitboxes(ReturnTransforms);
	
	return bDidHit;
}

bool UAbilityFunctionLibrary::PredictMultiSphereTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength,
	const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID,
	TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Results.Empty();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}

	GenerateOriginInfo(Shooter, OutOrigin);

	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	FVector AimTarget = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);
	//This profile will overlap relevant hitboxes and collide with relevant level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, true, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	TArray<FHitResult> HitboxTraceHits;
	UKismetSystemLibrary::LineTraceMultiByProfile(Shooter, OutOrigin.AimLocation, AimTarget, TraceProfile, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);

	//Find the first hit that is within a reasonable cone in front of the origin.
	//Note that this is more strict than just "in front," because multi-traces tend to be aimed at groups of targets, and large skewed angles can result in misses on some of those targets.
	if (HitboxTraceHits.Num() > 0)
	{
		for (const FHitResult& TraceResult : HitboxTraceHits)
		{
			const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((TraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
			if (Angle < AIMTOLERANCEDEGREES)
			{
				AimTarget = OutOrigin.Origin + (TraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal() * TraceLength;
				break;
			}
		}
	}

	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	UKismetSystemLibrary::SphereTraceMultiByProfile(Shooter, OutOrigin.Origin, AimTarget, TraceRadius, TraceProfile, false, ActorsToIgnore,
		EDrawDebugTrace::ForDuration, Results, true, FLinearColor::Green, FLinearColor::Red, 1.0f);
	
	OutTargetSet.SetID = TargetSetID;
	for (const FHitResult& Result : Results)
	{
		if (IsValid(Result.GetActor()))
		{
			OutTargetSet.Targets.Add(Result.GetActor());
		}
	}
	
	return OutTargetSet.Targets.Num() > 0;
}

TArray<AActor*> UAbilityFunctionLibrary::ValidateMultiSphereTrace(ASaiyoraPlayerCharacter* Shooter,
	const FAbilityOrigin& Origin, const TArray<AActor*>& Targets, const float TraceLength, const float TraceRadius,
	const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore)
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

	//Rewind hitboxes of all predicted targets and other actors in the way.
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, Targets, ActorsToIgnore, ReturnTransforms);

	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	FVector AimTarget = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);
	//This profile will overlap relevant hitboxes and collide with relevant level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, true, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	TArray<FHitResult> HitboxTraceHits;
	UKismetSystemLibrary::LineTraceMultiByProfile(Shooter, Origin.AimLocation, AimTarget, TraceProfile, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);

	//Find the first hit that is within a reasonable cone in front of the origin.
	//Note that this is more strict than just "in front," because multi-traces tend to be aimed at groups of targets, and large skewed angles can result in misses on some of those targets.
	if (HitboxTraceHits.Num() > 0)
	{
		for (const FHitResult& TraceResult : HitboxTraceHits)
		{
			const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((TraceResult.ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
			if (Angle < AIMTOLERANCEDEGREES)
			{
				AimTarget = Origin.Origin + (TraceResult.ImpactPoint - Origin.Origin).GetSafeNormal() * TraceLength;
				break;
			}
		}
	}

	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	TArray<FHitResult> Results;
	UKismetSystemLibrary::SphereTraceMultiByProfile(Shooter, Origin.Origin, AimTarget, TraceRadius, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, Results, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	//Validate predicted hits.
	for (const FHitResult& Result : Results)
	{
		if (IsValid(Result.GetActor()) && Targets.Contains(Result.GetActor()))
		{
			ValidatedTargets.AddUnique(Result.GetActor());
		}
	}

	//Unrewind any hitboxes that were rewound previously.
	UnrewindHitboxes(ReturnTransforms);
	
	return ValidatedTargets;
}

bool UAbilityFunctionLibrary::PredictSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength,
                                                      const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID,
                                                      FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Result = FHitResult();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}

	GenerateOriginInfo(Shooter, OutOrigin);

	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	const FVector CamTraceEnd = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);
	//This profile will collide with relevant hitboxes and level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, false, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	FHitResult CamTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true);

	//If there is a hit, check that it is in "front" of the origin (not between the origin and aim location). If so, use this as the new trace end.
	const FVector OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();

	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	FHitResult OriginTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginTraceResult, true);

	//If there is a hit, use this as the final trace end.
	const FVector SphereTraceEnd = OriginTraceResult.bBlockingHit ? OriginTraceResult.ImpactPoint : OriginTraceEnd;
	//Use object types instead of a profile here, so that level geometry is ignored and only hitboxes are considered. Level geometry was traced against already for targeting and will be checked later for line of sight.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	GetRelevantHitboxObjectTypes(Shooter, TraceHostility, ObjectTypes);
	//Trace from the origin to the final trace end. This is a multi-trace, since some targets may fail the line of sight requirement.
	TArray<FHitResult> SphereTraceResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, OutOrigin.Origin, SphereTraceEnd, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, SphereTraceResults, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	//Trace against level geometry for each possible target to check for line of sight to the origin.
	bool bValidResult = false;
	//This trace profile will overlap relevant hitboxes and collide with relevant level geometry. Since a single line trace is used, hitbox hits won't be returned.
	const FName LineOfSightTraceProfile = GetRelevantTraceProfile(Shooter, true, TracePlane, TraceHostility);
	for (const FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (IsValid(SphereTraceResult.GetActor()))
		{
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, OutOrigin.Origin, SphereTraceResult.ImpactPoint, LineOfSightTraceProfile, false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true);
			//If the line of sight trace hits nothing, the target is in line of sight of the origin, and is valid.
			if (!LineOfSightResult.bBlockingHit)
			{
				bValidResult = true;
				Result = SphereTraceResult;
				break;
			}
		}
	}
	//If no targets were hit, use the end of the origin trace.
	if (!bValidResult)
	{
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
	const FAbilityOrigin& Origin, AActor* Target, const float TraceLength, const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility,
	const TArray<AActor*>& ActorsToIgnore)
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

	//Rewind hitboxes of all predicted targets and other actors in the way.
	TArray<AActor*> TargetArray;
	TargetArray.Add(Target);
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, TargetArray, ActorsToIgnore, ReturnTransforms);

	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	const FVector CamTraceEnd = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);
	//This profile will collide with relevant hitboxes and level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, false, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	FHitResult CamTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, Origin.AimLocation, CamTraceEnd, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true);

	//If there is a hit, check that it is in "front" of the origin (not between the origin and aim location). If so, use this as the new trace end.
	const FVector OriginTraceEnd = Origin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - Origin.Origin), Origin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - Origin.Origin).GetSafeNormal();
	
	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	FHitResult OriginTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, Origin.Origin, OriginTraceEnd, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginTraceResult, true);

	//If there is a hit, use this as the final trace end.
	const FVector SphereTraceEnd = OriginTraceResult.bBlockingHit ? OriginTraceResult.ImpactPoint : OriginTraceEnd;
	//Use object types instead of a profile here, so that level geometry is ignored and only hitboxes are considered. Level geometry was traced against already for targeting and will be checked later for line of sight.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	GetRelevantHitboxObjectTypes(Shooter, TraceHostility, ObjectTypes);
	//Trace from the origin to the final trace end. This is a multi-trace, since some targets may fail the line of sight requirement.
	TArray<FHitResult> SphereTraceResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.Origin, SphereTraceEnd, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, SphereTraceResults, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	//Check line of sight only for the predicted hit target, ignoring other targets.
	bool bDidHit = false;
	//This trace profile will overlap relevant hitboxes and collide with relevant level geometry. Since a single line trace is used, hitbox hits won't be returned.
	const FName LineOfSightTraceProfile = GetRelevantTraceProfile(Shooter, true, TracePlane, TraceHostility);
	for (const FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (IsValid(SphereTraceResult.GetActor()) && SphereTraceResult.GetActor() == Target)
		{
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, Origin.Origin, SphereTraceResult.ImpactPoint, LineOfSightTraceProfile, false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true);
			//If the line of sight trace hits nothing, the target is in line of sight of the origin, and is valid.
			if (!LineOfSightResult.bBlockingHit)
			{
				bDidHit = true;
			}
			break;
		}
	}

	//Unrewind any hitboxes that were rewound previously.
	UnrewindHitboxes(ReturnTransforms);
	
	return bDidHit;
}

bool UAbilityFunctionLibrary::PredictMultiSphereSightTrace(ASaiyoraPlayerCharacter* Shooter, const float TraceLength,
	const float TraceRadius, const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore, const int32 TargetSetID,
	TArray<FHitResult>& Results, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	Results.Empty();
	OutOrigin = FAbilityOrigin();
	OutTargetSet = FAbilityTargetSet();
	if (!IsValid(Shooter) || !Shooter->IsLocallyControlled() || TraceLength <= 0.0f || TraceRadius <= 0.0f)
	{
		return false;
	}

	GenerateOriginInfo(Shooter, OutOrigin);

	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	FVector AimTarget = OutOrigin.AimLocation + OutOrigin.AimDirection * GetCameraTraceMaxRange(OutOrigin.AimLocation, OutOrigin.AimDirection, OutOrigin.Origin, TraceLength);
	//This profile will overlap relevant hitboxes and collide with relevant level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, true, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	TArray<FHitResult> HitboxTraceHits;
	UKismetSystemLibrary::LineTraceMultiByProfile(Shooter, OutOrigin.AimLocation, AimTarget, TraceProfile, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);

	//Find the first hit that is within a reasonable cone in front of the origin.
	//Note that this is more strict than just "in front," because multi-traces tend to be aimed at groups of targets, and large skewed angles can result in misses on some of those targets.
	if (HitboxTraceHits.Num() > 0)
	{
		for (const FHitResult& TraceResult : HitboxTraceHits)
		{
			const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((TraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
			if (Angle < AIMTOLERANCEDEGREES)
			{
				AimTarget = OutOrigin.Origin + (TraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal() * TraceLength;
				break;
			}
		}
	}
	
	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	FHitResult OriginTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, OutOrigin.Origin, AimTarget, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginTraceResult, true);

	//If there is a hit, use this as the final trace end.
	const FVector SphereTraceEnd = OriginTraceResult.bBlockingHit ? OriginTraceResult.ImpactPoint : AimTarget;
	//Use object types instead of a profile here, so that level geometry is ignored and only hitboxes are considered. Level geometry was traced against already for targeting and will be checked later for line of sight.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	GetRelevantHitboxObjectTypes(Shooter, TraceHostility, ObjectTypes);
	//Trace from the origin to the final trace end.
	TArray<FHitResult> SphereTraceResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, OutOrigin.Origin, SphereTraceEnd, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, SphereTraceResults, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	//Trace against level geometry for each possible target to check for line of sight to the origin.
	for (const FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (IsValid(SphereTraceResult.GetActor()))
		{
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, OutOrigin.Origin, SphereTraceResult.ImpactPoint, TraceProfile, false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true);
			//If the line of sight trace hits nothing, the target is in line of sight of the origin, and is valid.
			if (!LineOfSightResult.bBlockingHit)
			{
				Results.Add(SphereTraceResult);
			}
		}
	}

	OutTargetSet.SetID = TargetSetID;
	for (const FHitResult& Hit : Results)
	{
		if (IsValid(Hit.GetActor()))
		{
			OutTargetSet.Targets.AddUnique(Hit.GetActor());
		}
	}
	
	return OutTargetSet.Targets.Num() > 0;
}

TArray<AActor*> UAbilityFunctionLibrary::ValidateMultiSphereSightTrace(ASaiyoraPlayerCharacter* Shooter,
	const FAbilityOrigin& Origin, const TArray<AActor*>& Targets, const float TraceLength, const float TraceRadius,
	const ESaiyoraPlane TracePlane, const EFaction TraceHostility, const TArray<AActor*>& ActorsToIgnore)
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

	//Rewind hitboxes of all predicted targets and other actors in the way.
	TMap<UHitbox*, FTransform> ReturnTransforms;
	RewindRelevantHitboxes(Shooter, Origin, Targets, ActorsToIgnore, ReturnTransforms);
	
	//Set the default trace end to the the max range of the trace (adjusted by comparing to the origin), using aim direction and aim location.
	FVector AimTarget = Origin.AimLocation + Origin.AimDirection * GetCameraTraceMaxRange(Origin.AimLocation, Origin.AimDirection, Origin.Origin, TraceLength);
	//This profile will overlap relevant hitboxes and collide with relevant level geometry.
	const FName TraceProfile = GetRelevantTraceProfile(Shooter, true, TracePlane, TraceHostility);
	//Trace from aim location to the default trace end to see if anything is hit.
	TArray<FHitResult> HitboxTraceHits;
	UKismetSystemLibrary::LineTraceMultiByProfile(Shooter, Origin.AimLocation, AimTarget, TraceProfile, false,
		TArray<AActor*>(), EDrawDebugTrace::None, HitboxTraceHits, true);
	
	//Find the first hit that is within a reasonable cone in front of the origin.
	//Note that this is more strict than just "in front," because multi-traces tend to be aimed at groups of targets, and large skewed angles can result in misses on some of those targets.
	if (HitboxTraceHits.Num() > 0)
	{
		for (const FHitResult& TraceResult : HitboxTraceHits)
		{
			const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((TraceResult.ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
			if (Angle < AIMTOLERANCEDEGREES)
			{
				AimTarget = Origin.Origin + (TraceResult.ImpactPoint - Origin.Origin).GetSafeNormal() * TraceLength;
				break;
			}
		}
	}

	//Trace from the origin to the new trace end to make sure the origin is not obscured by other collision.
	FHitResult OriginTraceResult;
	UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, Origin.Origin, AimTarget, TraceProfile, false,
		ActorsToIgnore, EDrawDebugTrace::None, OriginTraceResult, true);

	//If there is a hit, use this as the final trace end.
	const FVector SphereTraceEnd = OriginTraceResult.bBlockingHit ? OriginTraceResult.ImpactPoint : AimTarget;
	//Use object types instead of a profile here, so that level geometry is ignored and only hitboxes are considered. Level geometry was traced against already for targeting and will be checked later for line of sight.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	GetRelevantHitboxObjectTypes(Shooter, TraceHostility, ObjectTypes);
	//Trace from the origin to the final trace end.
	TArray<FHitResult> SphereTraceResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.Origin, SphereTraceEnd, TraceRadius, ObjectTypes, false,
		ActorsToIgnore, EDrawDebugTrace::ForDuration, SphereTraceResults, true, FLinearColor::Green, FLinearColor::Red, 1.0f);

	//Check line of sight only for predicted hit targets.
	for (const FHitResult& SphereTraceResult : SphereTraceResults)
	{
		if (IsValid(SphereTraceResult.GetActor()) && Targets.Contains(SphereTraceResult.GetActor()))
		{
			TArray<AActor*> LineOfSightIgnoreActors = ActorsToIgnore;
			LineOfSightIgnoreActors.Add(SphereTraceResult.GetActor());
			FHitResult LineOfSightResult;
			UKismetSystemLibrary::LineTraceSingleByProfile(Shooter, Origin.Origin, SphereTraceResult.ImpactPoint, TraceProfile, false,
				LineOfSightIgnoreActors, EDrawDebugTrace::None, LineOfSightResult, true);
			//If the line of sight trace hits nothing, the target is in line of sight of the origin, and is valid.
			if (!LineOfSightResult.bBlockingHit)
			{
				ValidatedTargets.AddUnique(SphereTraceResult.GetActor());
			}
		}
	}

	//Unrewind any hitboxes that were rewound previously.
	UnrewindHitboxes(ReturnTransforms);
	
	return ValidatedTargets;
}

#pragma endregion
#pragma region Projectiles

APredictableProjectile* UAbilityFunctionLibrary::PredictProjectile(UCombatAbility* Ability,
	ASaiyoraPlayerCharacter* Shooter, const TSubclassOf<APredictableProjectile> ProjectileClass, FAbilityOrigin& OutOrigin)
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
	FVector AimTarget = OutOrigin.AimLocation + OutOrigin.AimDirection * CAMTRACELENGTH;
	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - OutOrigin.Origin).GetSafeNormal(), OutOrigin.AimDirection));
		if (Angle < AIMTOLERANCEDEGREES)
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
	ASaiyoraPlayerCharacter* Shooter, const TSubclassOf<APredictableProjectile> ProjectileClass,
	const FAbilityOrigin& Origin)
{
	if (!IsValid(Ability) || !IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || !IsValid(ProjectileClass))
	{
		//TODO: If this fails, we probably need some way to let the client know his projectile isn't real...
		//Potentially spawn a "dummy" projectile that is invisible, that will replace the client projectile with nothing then destroy itself.
		return nullptr;
	}
	const float Ping = USaiyoraCombatLibrary::GetActorPing(Shooter);
	ASaiyoraGameState* GameState = Shooter->GetWorld()->GetGameState<ASaiyoraGameState>();
	//Get target's hitboxes, rewind them, and store their actual transforms for un-rewinding.
	TMap<UHitbox*, FTransform> ReturnTransforms;
	//If listen server (ping 0), skip rewinding.
	if (Ping > 0.0f && IsValid(GameState))
	{
		//Do a big trace in front of the camera to rewind all targets that could potentially intercept the trace.
		const FVector RewindTraceEnd = Origin.AimLocation + CAMTRACELENGTH * Origin.AimDirection;
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel10));
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel11));
		TArray<FHitResult> RewindTraceResults;
		UKismetSystemLibrary::SphereTraceMultiForObjects(Shooter, Origin.AimLocation, RewindTraceEnd, REWINDTRACERADIUS, ObjectTypes, false,
			TArray<AActor*>(), EDrawDebugTrace::None, RewindTraceResults, true, FLinearColor::White);
		for (const FHitResult& Hit : RewindTraceResults)
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
	FVector AimTarget = Origin.AimLocation + Origin.AimDirection * CAMTRACELENGTH;
	//Trace from the camera forward to the default aim target, looking for anything that would block visibility that we could be aiming at.
	FHitResult VisTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, AimTarget, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		TArray<AActor*>(), EDrawDebugTrace::None, VisTraceResult, true);
	if (VisTraceResult.bBlockingHit)
	{
		//If there is something in the direction we're aiming, check if its within an allowed cone to prevent firing off at strange angles.
		const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct((VisTraceResult.ImpactPoint - Origin.Origin).GetSafeNormal(), Origin.AimDirection));
		if (Angle < AIMTOLERANCEDEGREES)
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
		const int32 Steps = FMath::Floor(Ping / .05f);
		if (Steps == 0)
		{
			//Ping was less than 50ms, we can just do one step against the already rewound hitboxes.
			if (IsValid(MoveComp))
			{
				MoveComp->TickComponent(Ping, ELevelTick::LEVELTICK_All, &MoveComp->PrimaryComponentTick);
			}
			//Unrewind after tick.
			for (const TTuple<UHitbox*, FTransform>& ReturnTransform : ReturnTransforms)
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
				for (const TTuple<UHitbox*, FTransform>& ReturnTransform : ReturnTransforms)
				{
					ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
				}
			}
			//Partial step for any remainder less than 50ms at the end.
			const float LastStepMs = Ping - (Steps * .05f);
			ReturnTransforms.Empty();
			for (UHitbox* Hitbox : RelevantHitboxes)
			{
				ReturnTransforms.Add(Hitbox, GameState->RewindHitbox(Hitbox, LastStepMs));
			}
			if (IsValid(MoveComp))
			{
				MoveComp->TickComponent(LastStepMs, ELevelTick::LEVELTICK_All, &MoveComp->PrimaryComponentTick);
			}
			for (const TTuple<UHitbox*, FTransform>& ReturnTransform : ReturnTransforms)
			{
				ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
			}
		}
	}
	return NewProjectile;
}

#pragma endregion 
