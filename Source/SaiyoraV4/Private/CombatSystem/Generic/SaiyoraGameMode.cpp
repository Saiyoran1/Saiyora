#include "SaiyoraGameMode.h"
#include "Hitbox.h"
#include "SaiyoraCombatLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetSystemLibrary.h"

const float ASaiyoraGameMode::MaxLagCompensation = 0.3f;
const float ASaiyoraGameMode::SnapshotInterval = 0.03f;

#pragma region Setup

void ASaiyoraGameMode::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(SnapshotHandle, this, &ASaiyoraGameMode::CreateSnapshot, SnapshotInterval, true);
}

#pragma endregion
#pragma region Snapshotting

void ASaiyoraGameMode::RegisterNewHitbox(UHitbox* Hitbox)
{
	if (Snapshots.Contains(Hitbox))
	{
		return;
	}
	Snapshots.Add(Hitbox);
}

void ASaiyoraGameMode::CreateSnapshot()
{
	for (TTuple<UHitbox*, FRewindRecord>& Snapshot : Snapshots)
	{
		if (IsValid(Snapshot.Key))
		{
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

FTransform ASaiyoraGameMode::RewindHitbox(UHitbox* Hitbox, float const Ping)
{
	//Clamp rewinding between 0 (current time) and max lag compensation.
	float const RewindTime = FMath::Clamp(Ping, 0.0f, MaxLagCompensation);
	FTransform const OriginalTransform = Hitbox->GetComponentTransform();
	//Zero rewind time means just use the current transform.
	if (RewindTime == 0.0f)
	{
		return OriginalTransform;
	}
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
		UKismetSystemLibrary::DrawDebugBox(this, Hitbox->GetComponentLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Blue, Hitbox->GetComponentRotation(), 5.0f, 2);
		UKismetSystemLibrary::DrawDebugBox(this, OriginalTransform.GetLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Green, OriginalTransform.Rotator(), 5.0f, 2);
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
	UKismetSystemLibrary::DrawDebugBox(this, Hitbox->GetComponentLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Blue, Hitbox->GetComponentRotation(), 5.0f, 2);
	UKismetSystemLibrary::DrawDebugBox(this, OriginalTransform.GetLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Green, OriginalTransform.Rotator(), 5.0f, 2);
	return OriginalTransform;
}

#pragma endregion 
#pragma region Trace Validation

bool ASaiyoraGameMode::ValidateLineTraceTarget(AActor* Shooter, FAbilityTarget const& Target, float const TraceLength,
	TEnumAsByte<ETraceTypeQuery> const TraceChannel, TArray<AActor*> const ActorsToIgnore)
{
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || !IsValid(Target.HitTarget) || Shooter == Target.HitTarget
		|| ActorsToIgnore.Contains(Target.HitTarget) || TraceLength <= 0.0f)
	{
		return false;
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
		Target.HitTarget->GetComponents<UHitbox>(HitboxComponents);
	
		for (UHitbox* Hitbox : HitboxComponents)
		{
			ReturnTransforms.Add(Hitbox, RewindHitbox(Hitbox, USaiyoraCombatLibrary::GetActorPing(Shooter)));
		}
	}
	//Trace against the rewound hitboxes.
	FVector const TraceEnd = Target.AimLocation + (TraceLength * Target.AimDirection.GetSafeNormal());
	FHitResult Result;
	UKismetSystemLibrary::LineTraceSingle(Shooter, Target.bUseSeparateOrigin ? Target.Origin : Target.AimLocation, TraceEnd,
		TraceChannel, false, ActorsToIgnore, EDrawDebugTrace::ForDuration, Result, false, FLinearColor::Blue);
	if (IsValid(Result.GetActor()) && Result.GetActor() == Target.HitTarget)
	{
		bDidHit = true;
	}
	//Return all rewound hitboxes to their actual transforms.
	for (TTuple<UHitbox*, FTransform> const& ReturnTransform : ReturnTransforms)
	{
		ReturnTransform.Key->SetWorldTransform(ReturnTransform.Value);
	}
	return bDidHit;
}

bool ASaiyoraGameMode::ValidateMultiLineTraceTarget(AActor* Shooter, FAbilityTarget const& Target,
	float const TraceLength, TEnumAsByte<ETraceTypeQuery> const TraceChannel, TArray<AActor*> const ActorsToIgnore)
{
	if (!IsValid(Shooter) || Shooter->GetLocalRole() != ROLE_Authority || !IsValid(Target.HitTarget) || Shooter == Target.HitTarget
		|| ActorsToIgnore.Contains(Target.HitTarget) || TraceLength <= 0.0f)
	{
		return false;
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
		Target.HitTarget->GetComponents<UHitbox>(HitboxComponents);
	
		for (UHitbox* Hitbox : HitboxComponents)
		{
			ReturnTransforms.Add(Hitbox, RewindHitbox(Hitbox, USaiyoraCombatLibrary::GetActorPing(Shooter)));
		}
	}
	//Trace against the rewound hitboxes.
	FVector const TraceEnd = Target.AimLocation + (TraceLength * Target.AimDirection.GetSafeNormal());
	//For multi-trace, if any of the hit targets are our target actor, the hit should be valid.
	TArray<FHitResult> Results;
	UKismetSystemLibrary::LineTraceMulti(Shooter, Target.bUseSeparateOrigin ? Target.Origin : Target.AimLocation, TraceEnd,
		TraceChannel, false,ActorsToIgnore, EDrawDebugTrace::ForDuration, Results, false, FLinearColor::Blue);
	for (FHitResult const& Hit : Results)
	{
		if (IsValid(Hit.GetActor()) && Hit.GetActor() == Target.HitTarget)
		{
			bDidHit = true;
			break;
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