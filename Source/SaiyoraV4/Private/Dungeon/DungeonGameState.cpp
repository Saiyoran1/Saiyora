#include "Dungeon/DungeonGameState.h"

#include "AncientSpecialization.h"
#include "DungeonPostProcess.h"
#include "ModernSpecialization.h"
#include "SaiyoraPlayerCharacter.h"
#include "UnrealNetwork.h"

void ADungeonGameState::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		const FDungeonProgress PreviousProgress = DungeonProgress;
		DungeonProgress.DungeonPhase = EDungeonPhase::WaitingToStart;
		DungeonProgress.PhaseStartTime = GetServerWorldTimeSeconds();
		for (const TTuple<FGameplayTag, FString>& BossInfo : BossRequirements)
		{
			DungeonProgress.BossesKilled.Add(FBossKillStatus(BossInfo.Key, BossInfo.Value));
		}
		OnRep_DungeonProgress(PreviousProgress);
	}
	GetWorld()->SpawnActor(ADungeonPostProcess::StaticClass());
}

void ADungeonGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADungeonGameState, DungeonProgress);
	DOREPLIFETIME(ADungeonGameState, ReadyPlayers);
}

#pragma region Dungeon Status

const float ADungeonGameState::MINIMUMTIMELIMIT = 1.0f;

bool ADungeonGameState::AreDungeonRequirementsMet()
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

void ADungeonGameState::OnRep_DungeonProgress(const FDungeonProgress& PreviousProgress)
{
	if (PreviousProgress.DungeonPhase != DungeonProgress.DungeonPhase)
	{
		OnDungeonPhaseChanged.Broadcast(PreviousProgress.DungeonPhase, DungeonProgress.DungeonPhase);
	}
	if (PreviousProgress.KillCount != DungeonProgress.KillCount)
	{
		OnKillCountChanged.Broadcast(DungeonProgress.KillCount, KillCountRequirement);
	}
	if (PreviousProgress.DeathCount != DungeonProgress.DeathCount)
	{
		OnDeathCountChanged.Broadcast(DungeonProgress.DeathCount, GetCurrentDeathPenaltyTime());
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
	if (!PreviousProgress.bDepleted && DungeonProgress.bDepleted)
	{
		OnDungeonDepleted.Broadcast();
	}
	if (PreviousProgress.CompletionTime != DungeonProgress.CompletionTime && DungeonProgress.CompletionTime >= 0.0f)
	{
		OnDungeonCompleted.Broadcast(DungeonProgress.CompletionTime);
	}
}

void ADungeonGameState::TryStartCountdown()
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::WaitingToStart)
	{
		return;
	}
	FText FailureMessage;
	if (!CheckDungeonCanStart(FailureMessage))
	{
		//If the dungeon can't start, notify all players of why.
		TArray<ASaiyoraPlayerCharacter*> Players;
		GetActivePlayers(Players);
		for (ASaiyoraPlayerCharacter* Player : Players)
		{
			Player->DisplayErrorMessage(FailureMessage, 10.0f);
		}
		return;
	}
	const FDungeonProgress PreviousProgress = DungeonProgress;
	DungeonProgress.DungeonPhase = EDungeonPhase::Countdown;
	DungeonProgress.PhaseStartTime = GetServerWorldTimeSeconds();
	OnRep_DungeonProgress(PreviousProgress);
	if (CountdownLength > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADungeonGameState::EndCountdown, CountdownLength, false);
	}
	else
	{
		//Skip the countdown phase entirely.
		EndCountdown();
	}
}

bool ADungeonGameState::CheckDungeonCanStart(FText& OutFailMessage) const
{
	TArray<ASaiyoraPlayerCharacter*> Players;
	GetActivePlayers(Players);
	TArray<const ASaiyoraPlayerCharacter*> PlayersMissingSpecs;
	//If any players haven't selected both an ancient and modern spec, don't start the dungeon.
	for (const ASaiyoraPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player->GetAncientSpecialization()) || !IsValid(Player->GetModernSpecialization()))
		{
			PlayersMissingSpecs.Add(Player);
		}
	}
	if (PlayersMissingSpecs.Num() > 0)
	{
		//If multiple players are missing specs, build a string that names all of them.
		if (PlayersMissingSpecs.Num() > 1)
		{
			FString LastPlayerName;
			FString PlayerNames;
			for (const ASaiyoraPlayerCharacter* Player : PlayersMissingSpecs)
			{
				if (!LastPlayerName.IsEmpty())
				{
					if (!PlayerNames.IsEmpty())
					{
						PlayerNames += ", ";
					}
					PlayerNames += LastPlayerName;
				}
				LastPlayerName = Player->GetName();
			}
			if (PlayersMissingSpecs.Num() > 2)
			{
				PlayerNames += ",";
			}
			OutFailMessage = FText::FromString(FString::Printf(
				TEXT("Cannot start the dungeon because players %s and %s haven't selected specializations yet!"), *PlayerNames, *LastPlayerName));
		}
		//Otherwise, just a string for the one player.
		else
		{
			OutFailMessage = FText::FromString(FString::Printf(
				TEXT("Cannot start the dungeon because player %s hasn't selected specializations yet!"), *PlayersMissingSpecs[0]->GetName()));
		}
		return false;
	}
	return true;
}

void ADungeonGameState::EndCountdown()
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::Countdown)
	{
		return;
	}
	const FDungeonProgress PreviousProgress = DungeonProgress;
	DungeonProgress.DungeonPhase = EDungeonPhase::InProgress;
	DungeonProgress.PhaseStartTime = GetServerWorldTimeSeconds();
	if (AreDungeonRequirementsMet())
	{
		//Instantly complete dungeon if all requirements are met.
		CompleteDungeon();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADungeonGameState::DepleteDungeonFromTimer, FMath::Max(MINIMUMTIMELIMIT, TimeLimit), false);
	}
	OnRep_DungeonProgress(PreviousProgress);
}

void ADungeonGameState::CompleteDungeon()
{
	DungeonProgress.CompletionTime = GetElapsedDungeonTime();
	DungeonProgress.DungeonPhase = EDungeonPhase::Completed;
	DungeonProgress.PhaseStartTime = GetServerWorldTimeSeconds();
	GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
}

void ADungeonGameState::DepleteDungeonFromTimer()
{
	if (!HasAuthority() || DungeonProgress.DungeonPhase != EDungeonPhase::InProgress)
	{
		return;
	}
	const FDungeonProgress PreviousProgress = DungeonProgress;
	DungeonProgress.bDepleted = true;
	OnRep_DungeonProgress(PreviousProgress);
}

bool ADungeonGameState::IsBossKilled(const FGameplayTag BossTag) const
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

float ADungeonGameState::GetElapsedDungeonTime() const
{
	if (DungeonProgress.DungeonPhase == EDungeonPhase::InProgress)
	{
		return (GetServerWorldTimeSeconds() - DungeonProgress.PhaseStartTime) + (DeathPenaltySeconds * DungeonProgress.DeathCount);
	}
	if (DungeonProgress.DungeonPhase == EDungeonPhase::Completed)
	{
		return DungeonProgress.CompletionTime;
	}	
	return 0.0f;
}

void ADungeonGameState::ReportPlayerDeath()
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
			GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ADungeonGameState::DepleteDungeonFromTimer, TimeRemaining - DeathPenaltySeconds, false);
		}
	}
	OnRep_DungeonProgress(PreviousProgress);
}

void ADungeonGameState::ReportTrashDeath(const int32 KillCount)
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

void ADungeonGameState::ReportBossDeath(const FGameplayTag BossTag)
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

void ADungeonGameState::UpdatePlayerReadyStatus(ASaiyoraPlayerCharacter* Player, const bool bReady)
{
	if (!HasAuthority() || !IsValid(Player) || DungeonProgress.DungeonPhase != EDungeonPhase::WaitingToStart)
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
		TArray<ASaiyoraPlayerCharacter*> Players;
		GetActivePlayers(Players);
		if (ReadyPlayers.Num() == Players.Num())
		{
			TryStartCountdown();
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

void ADungeonGameState::OnRep_ReadyPlayers(const TArray<ASaiyoraPlayerCharacter*>& PreviousReadyPlayers)
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