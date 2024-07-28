#include "CombatNetSubsystem.h"
#include "CombatDebugOptions.h"
#include "Hitbox.h"
#include "PredictableProjectile.h"
#include "SaiyoraGameInstance.h"
#include "SaiyoraPlayerCharacter.h"

#pragma region Hitbox Rewinding

void UCombatNetSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const USaiyoraGameInstance* GameInstance = Cast<USaiyoraGameInstance>(InWorld.GetGameInstance());
	if (IsValid(GameInstance))
	{
		DebugOptions = GameInstance->CombatDebugOptions;
	}
}

void UCombatNetSubsystem::RegisterNewHitbox(UHitbox* Hitbox)
{
    if (Snapshots.Contains(Hitbox))
    {
        return;
    }
    Snapshots.Add(Hitbox);
    if (Snapshots.Num() == 1)
    {
        GetWorld()->GetTimerManager().SetTimer(SnapshotHandle, this, &UCombatNetSubsystem::CreateSnapshot, SnapshotInterval, true);
    }
}

void UCombatNetSubsystem::CreateSnapshot()
{
	const float Timestamp = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
    for (TTuple<UHitbox*, FRewindRecord>& Snapshot : Snapshots)
    {
        if (IsValid(Snapshot.Key))
        {
            Snapshot.Value.Transforms.Add(TTuple<float, FTransform>(Timestamp, Snapshot.Key->GetComponentTransform()));
            //Everything else in here is just making sure we aren't saving snapshots significantly older than the max lag compensation.
            int32 LowestAcceptableIndex = -1;
            for (int i = 0; i < Snapshot.Value.Transforms.Num(); i++)
            {
                if (Snapshot.Value.Transforms[i].Key < Timestamp - MaxLagCompensation)
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

FTransform UCombatNetSubsystem::RewindHitbox(UHitbox* Hitbox, const float Timestamp)
{
	const float CurrentTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	const float RewindTimestamp = FMath::Clamp(Timestamp, CurrentTime - MaxLagCompensation, CurrentTime);
    const FTransform OriginalTransform = Hitbox->GetComponentTransform();
    //Zero rewind time means just use the current transform.
    if (RewindTimestamp == CurrentTime)
    {
    	return OriginalTransform;
    }
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
    	if (RewindTimestamp <= Record->Transforms[i].Key)
    	{
    		AfterIndex = i;
    		break;
    	}
    }
    //If the very first snapshot is after the timestamp, immediately apply max lag compensation (rewinding to the oldest snapshot).
    if (AfterIndex == 0)
    {
    	Hitbox->SetWorldTransform(Record->Transforms[0].Value);
    	if (IsValid(DebugOptions) && DebugOptions->bDrawRewindHitboxes)
    	{
    		DebugOptions->DrawRewindHitbox(Hitbox, OriginalTransform);
    	}
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
    	AfterTimestamp = CurrentTime;
    	AfterTransform = Hitbox->GetComponentTransform();
    }
    //Find out what fraction of the way from the before timestamp to the after timestamp our target timestamp is.
    const float SnapshotGap = AfterTimestamp - BeforeTimestamp;
    const float SnapshotFraction = (RewindTimestamp - BeforeTimestamp) / SnapshotGap;
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
	if (IsValid(DebugOptions) && DebugOptions->bDrawRewindHitboxes)
	{
		DebugOptions->DrawRewindHitbox(Hitbox, OriginalTransform);
	}
    return OriginalTransform;
}

#pragma endregion
#pragma region Projectiles

int32 UCombatNetSubsystem::GetNewProjectileID(ASaiyoraPlayerCharacter* Player, const FPredictedTick& Tick)
{
	if (!IsValid(Player))
	{
		return -1;
	}
	//Get the projectile info for this player.
	FPlayerProjectileInfo& PlayerProjectileInfo = PlayerProjectiles.FindOrAdd(Player);
	//Get the player's projectile info for the given tick.
	FPredictedTickProjectiles& PredictedTickInfo = PlayerProjectileInfo.TickProjectiles.FindOrAdd(Tick);
	//Increment the ID counter for this tick.
	return PredictedTickInfo.IDCounter++;
}

void UCombatNetSubsystem::RegisterClientProjectile(ASaiyoraPlayerCharacter* Player, APredictableProjectile* Projectile)
{
	if (!IsValid(Player) || Player->GetLocalRole() != ROLE_AutonomousProxy || !IsValid(Projectile))
	{
		return;
	}
	//Get the projectile info for the given player.
	FPlayerProjectileInfo& PlayerProjectileInfo = PlayerProjectiles.FindOrAdd(Player);
	//Get the player's projectile info for this predicted tick.
	FPredictedTickProjectiles& ProjectileIDs = PlayerProjectileInfo.TickProjectiles.FindOrAdd(Projectile->GetSourceInfo().SourceTick);
	//Add the projectile and its ID to the projectile map for this tick.
	ProjectileIDs.Projectiles.Add(Projectile->GetSourceInfo().ID, Projectile);
}

void UCombatNetSubsystem::ReplaceProjectile(APredictableProjectile* AuthProjectile)
{
	if (!IsValid(AuthProjectile) || AuthProjectile->IsFake())
	{
		return;
	}
	//Get the projectile info for the projectile's owner.
	FPlayerProjectileInfo* PlayerProjectileInfo = PlayerProjectiles.Find(AuthProjectile->GetSourceInfo().Owner);
	if (!PlayerProjectileInfo)
	{
		return;
	}
	//Get the info for the predicted tick this projectile was spawned during.
	FPredictedTickProjectiles* TickInfo = PlayerProjectileInfo->TickProjectiles.Find(AuthProjectile->GetSourceInfo().SourceTick);
	if (!TickInfo)
	{
		return;
	}
	//Get the client predicted projectile matching the auth projectile's ID.
	APredictableProjectile* ClientProjectile = TickInfo->Projectiles.FindRef(AuthProjectile->GetSourceInfo().ID);
	//If we found a client projectile, replace it (destroying it) with the server projectile.
	//Update the server projectile to be invisible if the client projectile was predicted to be destroyed already.
	if (IsValid(ClientProjectile))
	{
		const bool bWasLocallyDestroyed = ClientProjectile->Replace();
		AuthProjectile->UpdateLocallyDestroyed(bWasLocallyDestroyed);
	}
	//Remove the info for this tick/ID combo since it has already been replaced.
	TickInfo->Projectiles.Remove(AuthProjectile->GetSourceInfo().ID);
	//If there's no more predicted projectiles for this predicted tick, we can go ahead and remove the info struct for this tick.
	if (TickInfo->Projectiles.Num() == 0)
	{
		PlayerProjectileInfo->TickProjectiles.Remove(AuthProjectile->GetSourceInfo().SourceTick);
	}
}

#pragma endregion 