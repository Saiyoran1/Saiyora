#include "SaiyoraGameState.h"
#include "Hitbox.h"
#include "PredictableProjectile.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"

float const ASaiyoraGameState::SnapshotInterval = .03f;
float const ASaiyoraGameState::MaxLagCompensation = .3f;

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

void ASaiyoraGameState::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		const FDungeonProgress PreviousProgress = DungeonProgress;
		DungeonProgress.DungeonPhase = EDungeonPhase::WaitingToStart;
		DungeonProgress.PhaseStartTime = WorldTime;
		for (const TTuple<FGameplayTag, FString>& BossInfo : BossRequirements)
		{
			DungeonProgress.BossesKilled.Add(FBossKillStatus(BossInfo.Key, BossInfo.Value));
		}
		OnRep_DungeonProgress(PreviousProgress);
	}
}

void ASaiyoraGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASaiyoraGameState, DungeonProgress);
	DOREPLIFETIME(ASaiyoraGameState, ReadyPlayers);
}

#pragma region Time

float ASaiyoraGameState::GetServerWorldTimeSeconds() const
{
	return WorldTime;
}

void ASaiyoraGameState::UpdateClientWorldTime(float const ServerTime)
{
	WorldTime = ServerTime;
}

#pragma endregion 
#pragma region Snapshotting

void ASaiyoraGameState::RegisterNewHitbox(UHitbox* Hitbox)
{
    if (Snapshots.Contains(Hitbox))
    {
        return;
    }
    Snapshots.Add(Hitbox);
    if (Snapshots.Num() == 1)
    {
        GetWorld()->GetTimerManager().SetTimer(SnapshotHandle, this, &ASaiyoraGameState::CreateSnapshot, SnapshotInterval, true);
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
                if (Snapshot.Value.Transforms[i].Key < GetServerWorldTimeSeconds() - MaxLagCompensation)
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

FTransform ASaiyoraGameState::RewindHitbox(UHitbox* Hitbox, float const Ping)
{
    //Clamp rewinding between 0 (current time) and max lag compensation.
    float const RewindTime = FMath::Clamp(Ping, 0.0f, MaxLagCompensation);
    FTransform const OriginalTransform = Hitbox->GetComponentTransform();
    //Zero rewind time means just use the current transform.
    if (RewindTime == 0.0f)
    {
    	return OriginalTransform;
    }
    float const Timestamp = GetServerWorldTimeSeconds() - RewindTime;
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
#pragma region Dungeon Status

const float ASaiyoraGameState::MINIMUMTIMELIMIT = 1.0f;

bool ASaiyoraGameState::AreDungeonRequirementsMet()
{
	if (DungeonProgress.DungeonPhase == EDungeonPhase::Completed)
	{
		return true;
	}
	if (DungeonProgress.DungeonPhase != EDungeonPhase::InProgress)
	{
		return false;
	}
	if (DungeonProgress.KillCount < KillCountRequirement)
	{
		return false;
	}
	for (const TTuple<FGameplayTag, FString> BossInfo : BossRequirements)
	{
		if (!IsBossKilled(BossInfo.Key))
		{
			return false;
		}
	}
	return true;
}

void ASaiyoraGameState::OnRep_DungeonProgress(const FDungeonProgress& PreviousProgress)
{
	if (PreviousProgress.DungeonPhase != DungeonProgress.DungeonPhase)
	{
		OnDungeonPhaseChanged.Broadcast(PreviousProgress.DungeonPhase, DungeonProgress.DungeonPhase);
	}
	if (PreviousProgress.KillCount != DungeonProgress.KillCount)
	{
		OnKillCountChanged.Broadcast(DungeonProgress.KillCount);
	}
	if (PreviousProgress.DeathCount != DungeonProgress.DeathCount)
	{
		OnDeathCountChanged.Broadcast(DungeonProgress.DeathCount);
	}
	TArray<FBossKillStatus> PreviousBosses = PreviousProgress.BossesKilled;
	for (const FBossKillStatus& BossKillStatus : DungeonProgress.BossesKilled)
	{
		if (BossKillStatus.bKilled)
		{
			for (int i = 0; i < PreviousBosses.Num(); i++)
			{
				if (PreviousBosses[i].BossTag == BossKillStatus.BossTag)
				{
					if (!PreviousBosses[i].bKilled)
					{
						OnBossKilled.Broadcast(BossKillStatus.BossTag, BossKillStatus.DisplayName);
					}
					PreviousBosses.RemoveAt(i);
					break;
				}
			}
		}
	}
	if (PreviousProgress.bDepleted != DungeonProgress.bDepleted && DungeonProgress.bDepleted)
	{
		OnDungeonDepleted.Broadcast();
	}
	if (PreviousProgress.CompletionTime != DungeonProgress.CompletionTime && DungeonProgress.CompletionTime >= 0.0f)
	{
		OnDungeonCompleted.Broadcast(DungeonProgress.CompletionTime);
	}
}

void ASaiyoraGameState::StartCountdown()
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::WaitingToStart)
	{
		return;
	}
	const FDungeonProgress PreviousProgress = DungeonProgress;
	DungeonProgress.DungeonPhase = EDungeonPhase::Countdown;
	DungeonProgress.PhaseStartTime = WorldTime;
	OnRep_DungeonProgress(PreviousProgress);
	if (CountdownLength > 0.0f)
	{
		FTimerDelegate CountdownDelegate;
		CountdownDelegate.BindUObject(this, &ASaiyoraGameState::EndCountdown);
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, CountdownDelegate, CountdownLength, false);
	}
	else
	{
		//Skip the countdown phase entirely.
		EndCountdown();
	}
}

void ASaiyoraGameState::EndCountdown()
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::Countdown)
	{
		return;
	}
	const FDungeonProgress PreviousProgress = DungeonProgress;
	DungeonProgress.DungeonPhase = EDungeonPhase::InProgress;
	DungeonProgress.PhaseStartTime = WorldTime;
	if (AreDungeonRequirementsMet())
	{
		//Instantly complete dungeon if all requirements are met.
		CompleteDungeon();
	}
	else
	{
		FTimerDelegate DepletionDelegate;
		DepletionDelegate.BindUObject(this, &ASaiyoraGameState::DepleteDungeonFromTimer);
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, DepletionDelegate, FMath::Max(MINIMUMTIMELIMIT, TimeLimit), false);
	}
	OnRep_DungeonProgress(PreviousProgress);
}

void ASaiyoraGameState::CompleteDungeon()
{
	DungeonProgress.CompletionTime = GetElapsedDungeonTime();
	DungeonProgress.DungeonPhase = EDungeonPhase::Completed;
	DungeonProgress.PhaseStartTime = WorldTime;
	GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
}

void ASaiyoraGameState::DepleteDungeonFromTimer()
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::InProgress)
	{
		return;
	}
	const FDungeonProgress PreviousProgress = DungeonProgress;
	DungeonProgress.bDepleted = true;
	OnRep_DungeonProgress(PreviousProgress);
}

bool ASaiyoraGameState::IsBossKilled(const FGameplayTag BossTag) const
{
	if (!BossTag.IsValid() || !BossTag.MatchesTag(FSaiyoraCombatTags::Get().DungeonBoss) || BossTag.MatchesTagExact(FSaiyoraCombatTags::Get().DungeonBoss))
	{
		return false;
	}
	for (const FBossKillStatus& KillStatus : DungeonProgress.BossesKilled)
	{
		if (KillStatus.BossTag.MatchesTagExact(BossTag))
		{
			return KillStatus.bKilled;
		}
	}
	return false;
}

float ASaiyoraGameState::GetElapsedDungeonTime() const
{
	if (DungeonProgress.DungeonPhase == EDungeonPhase::InProgress)
	{
		return (WorldTime - DungeonProgress.PhaseStartTime) + (DeathPenaltySeconds * DungeonProgress.DeathCount);
	}
	if (DungeonProgress.DungeonPhase == EDungeonPhase::Completed)
	{
		return DungeonProgress.CompletionTime;
	}	
	return 0.0f;
}

void ASaiyoraGameState::ReportPlayerDeath()
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::InProgress)
	{
		return;
	}
	const FDungeonProgress PreviousProgress = DungeonProgress;
	DungeonProgress.DeathCount++;
	if (!DungeonProgress.bDepleted)
	{
		const float TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(PhaseTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		if (TimeRemaining <= DeathPenaltySeconds)
		{
			DungeonProgress.bDepleted = true;
		}
		else
		{
			FTimerDelegate DepletionDelegate;
			DepletionDelegate.BindUObject(this, &ASaiyoraGameState::DepleteDungeonFromTimer);
			GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, DepletionDelegate, TimeRemaining - DeathPenaltySeconds, false);
		}
	}
	OnRep_DungeonProgress(PreviousProgress);
}

void ASaiyoraGameState::ReportTrashDeath(const int32 KillCount)
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::InProgress || KillCount <= 0)
	{
		return;
	}
	const FDungeonProgress PreviousProgress = DungeonProgress;
	DungeonProgress.KillCount += KillCount;
	if (AreDungeonRequirementsMet())
	{
		CompleteDungeon();
	}
	OnRep_DungeonProgress(PreviousProgress);
}

void ASaiyoraGameState::ReportBossDeath(const FGameplayTag BossTag)
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::InProgress ||
		!BossTag.IsValid() || !BossTag.MatchesTag(FSaiyoraCombatTags::Get().DungeonBoss) || BossTag.MatchesTagExact(FSaiyoraCombatTags::Get().DungeonBoss))
	{
		return;
	}
	for (FBossKillStatus& KillStatus : DungeonProgress.BossesKilled)
	{
		if (KillStatus.BossTag.MatchesTagExact(BossTag))
		{
			if (!KillStatus.bKilled)
			{
				const FDungeonProgress PreviousProgress = DungeonProgress;
				KillStatus.bKilled = true;
				if (AreDungeonRequirementsMet())
				{
					CompleteDungeon();
				}
				OnRep_DungeonProgress(PreviousProgress);
			}
			break;
		}
	}
}

#pragma endregion 
#pragma region Player Handling

void ASaiyoraGameState::InitPlayer(ASaiyoraPlayerCharacter* Player)
{
	if (!Player || GroupPlayers.Contains(Player))
	{
		return;
	}
	GroupPlayers.Add(Player);
	OnPlayerAdded.Broadcast(Player);
}

void ASaiyoraGameState::UpdatePlayerReadyStatus(ASaiyoraPlayerCharacter* Player, const bool bReady)
{
	if (!HasAuthority() || !Player || DungeonProgress.DungeonPhase != EDungeonPhase::WaitingToStart)
	{
		return;
	}
	const TArray<ASaiyoraPlayerCharacter*> PreviousReadyPlayers = ReadyPlayers;
	if (bReady)
	{
		if (ReadyPlayers.Contains(Player))
		{
			return;
		}
		ReadyPlayers.Add(Player);
		OnRep_ReadyPlayers(PreviousReadyPlayers);
		if (ReadyPlayers.Num() == GroupPlayers.Num())
		{
			StartCountdown();
		}
	}
	else
	{
		if (ReadyPlayers.Remove(Player) > 0)
		{
			OnRep_ReadyPlayers(PreviousReadyPlayers);
		}
	}
}

void ASaiyoraGameState::OnRep_ReadyPlayers(const TArray<ASaiyoraPlayerCharacter*>& PreviousReadyPlayers)
{
	TArray<ASaiyoraPlayerCharacter*> UncheckedPlayers = PreviousReadyPlayers;
	for (ASaiyoraPlayerCharacter* Player : ReadyPlayers)
	{
		if (PreviousReadyPlayers.Contains(Player))
		{
			UncheckedPlayers.Remove(Player);
		}
		else
		{
			OnPlayerReadyChanged.Broadcast(Player, true);
		}
	}
	for (const ASaiyoraPlayerCharacter* Player : UncheckedPlayers)
	{
		OnPlayerReadyChanged.Broadcast(Player, false);
	}
}

#pragma endregion 