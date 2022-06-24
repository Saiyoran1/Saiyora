﻿#include "CoreClasses/SaiyoraGameState.h"
#include "Hitbox.h"
#include "PredictableProjectile.h"
#include "SaiyoraPlayerCharacter.h"
#include "Kismet/KismetSystemLibrary.h"

ASaiyoraGameState::ASaiyoraGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASaiyoraGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	WorldTime += DeltaSeconds;
}

void ASaiyoraGameState::InitPlayer(ASaiyoraPlayerCharacter* Player)
{
	if (!IsValid(Player) || ActivePlayers.Contains(Player))
	{
		return;
	}
	ActivePlayers.Add(Player);
	OnPlayerAdded.Broadcast(Player);
}

#pragma region Snapshotting

float const ASaiyoraGameState::SNAPSHOTINTERVAL = .03f;
float const ASaiyoraGameState::MAXLAGCOMPENSATION = .3f;

void ASaiyoraGameState::RegisterNewHitbox(UHitbox* Hitbox)
{
    if (Snapshots.Contains(Hitbox))
    {
        return;
    }
    Snapshots.Add(Hitbox);
    if (Snapshots.Num() == 1)
    {
        GetWorld()->GetTimerManager().SetTimer(SnapshotHandle, this, &ASaiyoraGameState::CreateSnapshot, SNAPSHOTINTERVAL, true);
    }
}

void ASaiyoraGameState::CreateSnapshot()
{
    for (TTuple<UHitbox*, FRewindRecord>& Snapshot : Snapshots)
    {
        if (IsValid(Snapshot.Key))
        {
            Snapshot.Value.Transforms.Add(TTuple<float, FTransform>(GetServerWorldTimeSeconds(), Snapshot.Key->GetComponentTransform()));
            //Everything else in here is just making sure we aren't saving snapshots significantly older than the max lag compensation.
            int32 LowestAcceptableIndex = -1;
            for (int i = 0; i < Snapshot.Value.Transforms.Num(); i++)
            {
                if (Snapshot.Value.Transforms[i].Key < GetServerWorldTimeSeconds() - MAXLAGCOMPENSATION)
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

FTransform ASaiyoraGameState::RewindHitbox(UHitbox* Hitbox, const float Ping)
{
    //Clamp rewinding between 0 (current time) and max lag compensation.
    const float RewindTime = FMath::Clamp(Ping, 0.0f, MAXLAGCOMPENSATION);
    const FTransform OriginalTransform = Hitbox->GetComponentTransform();
    //Zero rewind time means just use the current transform.
    if (RewindTime == 0.0f)
    {
    	return OriginalTransform;
    }
    const float Timestamp = GetServerWorldTimeSeconds() - RewindTime;
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
    float BeforeTimestamp;
    float AfterTimestamp;
    FTransform BeforeTransform;
    FTransform AfterTransform;
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
    	AfterTimestamp = GetServerWorldTimeSeconds();
    	AfterTransform = Hitbox->GetComponentTransform();
    }
    //Find out what fraction of the way from the before timestamp to the after timestamp our target timestamp is.
    const float SnapshotGap = AfterTimestamp - BeforeTimestamp;
    const float SnapshotFraction = (Timestamp - BeforeTimestamp) / SnapshotGap;
    //Interpolate location.
    const FVector LocDiff = AfterTransform.GetLocation() - BeforeTransform.GetLocation();
    const FVector InterpVector = LocDiff * SnapshotFraction;
    Hitbox->SetWorldLocation(BeforeTransform.GetLocation() + InterpVector);
    //Interpolate rotation.
    const FRotator RotDiff = AfterTransform.Rotator() - BeforeTransform.Rotator();
    const FRotator InterpRotator = RotDiff * SnapshotFraction;
    Hitbox->SetWorldRotation(BeforeTransform.Rotator() + InterpRotator);
    //Don't interpolate scale (I don't currently have smooth scale changes). Just pick whichever is closer to the target timestamp.
    Hitbox->SetWorldScale3D(SnapshotFraction <= 0.5f ? BeforeTransform.GetScale3D() : AfterTransform.GetScale3D());
    UKismetSystemLibrary::DrawDebugBox(Hitbox, Hitbox->GetComponentLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Blue, Hitbox->GetComponentRotation(), 5.0f, 2);
    UKismetSystemLibrary::DrawDebugBox(Hitbox, OriginalTransform.GetLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Green, OriginalTransform.Rotator(), 5.0f, 2);
    return OriginalTransform;
}

#pragma endregion
#pragma region Projectile Management

void ASaiyoraGameState::RegisterClientProjectile(APredictableProjectile* Projectile)
{
	if (!IsValid(Projectile) || Projectile->GetOwner()->GetLocalRole() != ROLE_AutonomousProxy || !Projectile->IsFake())
	{
		return;
	}
	FakeProjectiles.Add(Projectile->GetSourceInfo().SourceTick, Projectile);
}

void ASaiyoraGameState::ReplaceProjectile(APredictableProjectile* ServerProjectile)
{
	if (!IsValid(ServerProjectile) || ServerProjectile->IsFake())
	{
		return;
	}
	TArray<APredictableProjectile*> MatchingProjectiles;
	FakeProjectiles.MultiFind(ServerProjectile->GetSourceInfo().SourceTick, MatchingProjectiles);
	for (APredictableProjectile* Proj : MatchingProjectiles)
	{
		if (IsValid(Proj) && Proj->GetClass() == ServerProjectile->GetClass() && Proj->GetSourceInfo().ID == ServerProjectile->GetSourceInfo().ID)
		{
			bool const bLocallyDestroyed = Proj->Replace();
			ServerProjectile->UpdateLocallyDestroyed(bLocallyDestroyed);
			FakeProjectiles.RemoveSingle(ServerProjectile->GetSourceInfo().SourceTick, Proj);
			return;
		}
	}
}

#pragma endregion