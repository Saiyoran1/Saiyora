#include "AbilityFunctionLibrary.h"
#include "Hitbox.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "GameFramework/GameState.h"
#include "Kismet/KismetSystemLibrary.h"

float const UAbilityFunctionLibrary::SnapshotInterval = 0.03f;
float const UAbilityFunctionLibrary::CamTraceLength = 10000.0f;
float const UAbilityFunctionLibrary::MaxLagCompensation = 0.3f;
FTimerHandle UAbilityFunctionLibrary::SnapshotHandle = FTimerHandle();
TMap<UHitbox*, FRewindRecord> UAbilityFunctionLibrary::Snapshots = TMap<UHitbox*, FRewindRecord>();

FAbilityOrigin UAbilityFunctionLibrary::MakeAbilityOrigin(FVector const& AimLocation, FVector const& AimDirection, FVector const& Origin)
{
	FAbilityOrigin Target;
	Target.Origin = Origin;
	Target.AimLocation = AimLocation;
	Target.AimDirection = AimDirection;
	return Target;
}

#pragma region Snapshotting

void UAbilityFunctionLibrary::RegisterNewHitbox(UHitbox* Hitbox)
{
	if (Snapshots.Contains(Hitbox))
	{
		return;
	}
	Snapshots.Add(Hitbox);
	if (Snapshots.Num() == 1)
	{
		FTimerDelegate const SnapshotDel = FTimerDelegate::CreateStatic(&UAbilityFunctionLibrary::CreateSnapshot);
		Hitbox->GetWorld()->GetTimerManager().SetTimer(SnapshotHandle, SnapshotDel, SnapshotInterval, true);
	}
}

void UAbilityFunctionLibrary::CreateSnapshot()
{
	AGameStateBase* GameState = nullptr;
	for (TTuple<UHitbox*, FRewindRecord>& Snapshot : Snapshots)
	{
		if (IsValid(Snapshot.Key))
		{
			if (!IsValid(GameState))
			{
				GameState = Snapshot.Key->GetWorld()->GetGameState();
			}
			Snapshot.Value.Transforms.Add(TTuple<float, FTransform>(GameState->GetServerWorldTimeSeconds(), Snapshot.Key->GetComponentTransform()));
			//Everything else in here is just making sure we aren't saving snapshots significantly older than the max lag compensation.
			int32 LowestAcceptableIndex = -1;
			for (int i = 0; i < Snapshot.Value.Transforms.Num(); i++)
			{
				if (Snapshot.Value.Transforms[i].Key < GameState->GetServerWorldTimeSeconds() - MaxLagCompensation)
				{
					LowestAcceptableIndex = i;
				}
				else
				{
					break;
				}
			}
			if (LowestAcceptableIndex > -1)
			{
				Snapshot.Value.Transforms.RemoveAt(0, LowestAcceptableIndex + 1);
			}
		}
	}
}

FTransform UAbilityFunctionLibrary::RewindHitbox(UHitbox* Hitbox, float const Ping)
{
	//Clamp rewinding between 0 (current time) and max lag compensation.
	float const RewindTime = FMath::Clamp(Ping, 0.0f, MaxLagCompensation);
	FTransform const OriginalTransform = Hitbox->GetComponentTransform();
	//Zero rewind time means just use the current transform.
	if (RewindTime == 0.0f)
	{
		return OriginalTransform;
	}
	AGameStateBase* GameState = Hitbox->GetWorld()->GetGameState();
	float const Timestamp = GameState->GetServerWorldTimeSeconds() - RewindTime;
	FRewindRecord* Record = Snapshots.Find(Hitbox);
	//If this hitbox wasn't registered or hasn't had a snapshot yet, we won't rewind it.
	if (!Record)
	{
		return OriginalTransform;
	}
	if (Record->Transforms.Num() == 0)
	{
		return OriginalTransform;
	}
	int32 AfterIndex = -1;
	//Iterate trying to find the first record that comes AFTER the timestamp.
	for (int i = 0; i < Record->Transforms.Num(); i++)
	{
		if (Timestamp <= Record->Transforms[i].Key)
		{
			AfterIndex = i;
			break;
		}
	}
	//If the very first snapshot is after the timestamp, immediately apply max lag compensation (rewinding to the oldest snapshot).
	if (AfterIndex == 0)
	{
		Hitbox->SetWorldTransform(Record->Transforms[0].Value);
		UKismetSystemLibrary::DrawDebugBox(Hitbox, Hitbox->GetComponentLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Blue, Hitbox->GetComponentRotation(), 5.0f, 2);
		UKismetSystemLibrary::DrawDebugBox(Hitbox, OriginalTransform.GetLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Green, OriginalTransform.Rotator(), 5.0f, 2);
		return OriginalTransform;
	}
	float BeforeTimestamp = GameState->GetServerWorldTimeSeconds();
	float AfterTimestamp = GameState->GetServerWorldTimeSeconds();
	FTransform BeforeTransform = Hitbox->GetComponentTransform();
	FTransform AfterTransform = Hitbox->GetComponentTransform();
	if (Record->Transforms.IsValidIndex(AfterIndex))
	{
		BeforeTimestamp = Record->Transforms[AfterIndex - 1].Key;
		BeforeTransform = Record->Transforms[AfterIndex - 1].Value;
		AfterTimestamp = Record->Transforms[AfterIndex].Key;
		AfterTransform = Record->Transforms[AfterIndex].Value;
	}
	else
	{
		//If we didn't find a record after the timestamp, we can interpolate from the last record to current position.
		BeforeTimestamp = Record->Transforms[Record->Transforms.Num() - 1].Key;
		BeforeTransform = Record->Transforms[Record->Transforms.Num() - 1].Value;
		AfterTimestamp = GameState->GetServerWorldTimeSeconds();
		AfterTransform = Hitbox->GetComponentTransform();
	}
	//Find out what fraction of the way from the before timestamp to the after timestamp our target timestamp is.
	float const SnapshotGap = AfterTimestamp - BeforeTimestamp;
	float const SnapshotFraction = (Timestamp - BeforeTimestamp) / SnapshotGap;
	//Interpolate location.
	FVector const LocDiff = AfterTransform.GetLocation() - BeforeTransform.GetLocation();
	FVector const InterpVector = LocDiff * SnapshotFraction;
	Hitbox->SetWorldLocation(BeforeTransform.GetLocation() + InterpVector);
	//Interpolate rotation.
	FRotator const RotDiff = AfterTransform.Rotator() - BeforeTransform.Rotator();
	FRotator const InterpRotator = RotDiff * SnapshotFraction;
	Hitbox->SetWorldRotation(BeforeTransform.Rotator() + InterpRotator);
	//Don't interpolate scale (I don't currently have smooth scale changes). Just pick whichever is closer to the target timestamp.
	Hitbox->SetWorldScale3D(SnapshotFraction <= 0.5f ? BeforeTransform.GetScale3D() : AfterTransform.GetScale3D());
	UKismetSystemLibrary::DrawDebugBox(Hitbox, Hitbox->GetComponentLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Blue, Hitbox->GetComponentRotation(), 5.0f, 2);
	UKismetSystemLibrary::DrawDebugBox(Hitbox, OriginalTransform.GetLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Green, OriginalTransform.Rotator(), 5.0f, 2);
	return OriginalTransform;
}

#pragma endregion 
#pragma region Trace Validation

bool UAbilityFunctionLibrary::PredictLineTrace(AActor* Shooter, float const TraceLength,
	TEnumAsByte<ETraceTypeQuery> const TraceChannel, TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID,
	FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	if (!IsValid(Shooter) || !Shooter->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) || TraceLength <= 0.0f)
	{
		return false;
	}
	APawn* ShooterAsPawn = Cast<APawn>(Shooter);
	if (!IsValid(ShooterAsPawn) || !ShooterAsPawn->IsLocallyControlled())
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
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, Result, true);

	OutTargetSet.SetID = TargetSetID;
	if (IsValid(Result.GetActor()))
	{
		OutTargetSet.Targets.Add(Result.GetActor());
	}
	
	return Result.bBlockingHit;
}

bool UAbilityFunctionLibrary::ValidateLineTraceTarget(AActor* Shooter, FAbilityOrigin const& Origin, AActor* Target,
	float const TraceLength, TEnumAsByte<ETraceTypeQuery> const TraceChannel, TArray<AActor*> const& ActorsToIgnore)
{
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || !IsValid(Target) || Shooter == Target
		|| ActorsToIgnore.Contains(Target) || TraceLength <= 0.0f)
	{
		return false;
	}
	APawn* ShooterAsPawn = Cast<APawn>(Shooter);
	if (IsValid(ShooterAsPawn) && ShooterAsPawn->IsLocallyControlled())
	{
		//If the prediction was done locally, we don't need to validate.
		return true;
	}
	//TODO: Validate aim location and origin (if using separate origin).
	bool bDidHit = false;
	float const Ping = USaiyoraCombatLibrary::GetActorPing(Shooter);
	//Get target's hitboxes, rewind them, and store their actual transforms for un-rewinding.
	TMap<UHitbox*, FTransform> ReturnTransforms;
	//If listen server (ping 0), skip rewinding.
	if (Ping > 0.0f)
	{
		TArray<UHitbox*> HitboxComponents;
		Target->GetComponents<UHitbox>(HitboxComponents);
		for (UHitbox* Hitbox : HitboxComponents)
		{
			ReturnTransforms.Add(Hitbox, RewindHitbox(Hitbox, USaiyoraCombatLibrary::GetActorPing(Shooter)));
		}
	}
	//Trace from the camera for a very large distance to find what we were aiming at.
	//TODO: Math to find distance and angle from origin point so that we can calculate the max distance we can trace before we outrange the origin.
	FVector const CamTraceEnd = Origin.AimLocation + (CamTraceLength * Origin.AimDirection.GetSafeNormal());
	FHitResult Result;
	UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.AimLocation, CamTraceEnd,
		TraceChannel, false, ActorsToIgnore, EDrawDebugTrace::ForDuration, Result, false, FLinearColor::Blue);
	if (IsValid(Result.GetActor()) && Result.GetActor() == Target)
	{
		//If the aim trace was successful (we were aiming at the target), check that a trace from the aim origin isn't blocked and is within range.
		FVector OriginTraceEnd = Origin.Origin + (TraceLength * (Result.ImpactPoint - Origin.Origin).GetSafeNormal());
		FHitResult OriginResult;
		UKismetSystemLibrary::LineTraceSingle(Shooter, Origin.Origin, OriginTraceEnd, TraceChannel, false, ActorsToIgnore,
			EDrawDebugTrace::ForDuration, OriginResult, false, FLinearColor::Green);
		if (IsValid(OriginResult.GetActor()) && OriginResult.GetActor() == Target && FVector::DotProduct(Origin.AimDirection, (OriginResult.ImpactPoint - Origin.Origin)) > 0.0f)
		{
			bDidHit = true;
		}
	}
	//Return all rewound hitboxes to their actual transforms.
	for (TTuple<UHitbox*, FTransform> const& ReturnTransform : ReturnTransforms)
	{
		ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
	}
	return bDidHit;
}

#pragma endregion