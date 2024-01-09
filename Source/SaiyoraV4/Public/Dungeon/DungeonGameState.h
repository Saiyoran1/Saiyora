#pragma once
#include "AbilityStructs.h"
#include "SaiyoraPlayerController.h"
#include "CoreClasses/SaiyoraGameState.h"
#include "DungeonGameState.generated.h"

UENUM(BlueprintType)
enum class EDungeonPhase : uint8
{
	None,
	WaitingToStart,
	Countdown,
	InProgress,
	Completed,
};

USTRUCT()
struct FBossKillStatus
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag BossTag;
	UPROPERTY()
	FString DisplayName;
	UPROPERTY()
	bool bKilled = false;

	FBossKillStatus() {}
	FBossKillStatus(const FGameplayTag Tag, const FString& Name) { BossTag = Tag; DisplayName = Name; }
};

USTRUCT()
struct FDungeonProgress
{
	GENERATED_BODY()

	UPROPERTY()
	EDungeonPhase DungeonPhase = EDungeonPhase::None;
	UPROPERTY()
	float PhaseStartTime = -1.0f;
	UPROPERTY()
	TArray<FBossKillStatus> BossesKilled;
	UPROPERTY()
	int32 KillCount = 0;
	UPROPERTY()
	int32 DeathCount = 0;
	UPROPERTY()
	bool bDepleted = false;
	UPROPERTY()
	float CompletionTime = -1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDungeonPhaseChanged, const EDungeonPhase, OldPhase, const EDungeonPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKillCountChanged, const int32, KillCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathCountChanged, const int32, DeathCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossKilled, const FGameplayTag, BossTag, const FString&, BossName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDungeonDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDungeonCompleted, const float, CompletionTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerReadyChanged, const ASaiyoraPlayerCharacter*, Player, const bool, bReady);

UCLASS()
class SAIYORAV4_API ADungeonGameState : public ASaiyoraGameState
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Player Handling

public:
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void UpdatePlayerReadyStatus(ASaiyoraPlayerCharacter* Player, const bool bReady);
	UFUNCTION(BlueprintPure)
	void GetReadyPlayers(TArray<ASaiyoraPlayerCharacter*>& OutReadyPlayers) const { OutReadyPlayers = ReadyPlayers; }
	UPROPERTY(BlueprintAssignable)
	FOnPlayerReadyChanged OnPlayerReadyChanged;
	
private:
	
	UPROPERTY(ReplicatedUsing = OnRep_ReadyPlayers)
	TArray<ASaiyoraPlayerCharacter*> ReadyPlayers;
	UFUNCTION()
	void OnRep_ReadyPlayers(const TArray<ASaiyoraPlayerCharacter*>& PreviousReadyPlayers);

	//Dungeon State

public:

	UFUNCTION(BlueprintPure)
	FString GetDungeonName() const { return DungeonName; }
	UFUNCTION(BlueprintPure)
	EDungeonPhase GetDungeonPhase() const { return DungeonProgress.DungeonPhase; }
	UFUNCTION(BlueprintPure)
	float GetDungeonPhaseStartTime() const { return DungeonProgress.PhaseStartTime; }
	UFUNCTION(BlueprintPure)
	int32 GetCurrentKillCount() const { return DungeonProgress.KillCount; }
	UFUNCTION(BlueprintPure)
	int32 GetKillCountRequirement() const { return KillCountRequirement; }
	UFUNCTION(BlueprintPure)
	int32 GetCurrentDeathCount() const { return DungeonProgress.DeathCount; }
	UFUNCTION(BlueprintPure)
	float GetCurrentDeathPenaltyTime() const { return DeathPenaltySeconds * DungeonProgress.DeathCount; }
	UFUNCTION(BlueprintPure)
	void GetBossRequirements(TMap<FGameplayTag, FString>& OutBosses) const { OutBosses = BossRequirements; }
	UFUNCTION(BlueprintPure, meta = (GameplayTagFilter = "Boss"))
	bool IsBossKilled(const FGameplayTag BossTag) const;
	UFUNCTION(BlueprintPure)
	float GetDungeonTimeLimit() const { return TimeLimit; }
	UFUNCTION(BlueprintPure)
	float GetElapsedDungeonTime() const;
	UFUNCTION(BlueprintPure)
	bool IsDungeonDepleted() const { return DungeonProgress.bDepleted; }
	UFUNCTION(BlueprintPure)
	float GetDungeonCompletionTime() const { return DungeonProgress.CompletionTime; }
	UFUNCTION(BlueprintPure)
	float GetCountdownTimeRemaining() const { return DungeonProgress.DungeonPhase == EDungeonPhase::Countdown ? FMath::Max(0.0f, CountdownLength - (GetServerWorldTimeSeconds() - DungeonProgress.PhaseStartTime)) : 0.0f; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ReportPlayerDeath();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ReportTrashDeath(const int32 KillCount = 1);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (GameplayTagFilter = "Boss"))
	void ReportBossDeath(const FGameplayTag BossTag);
	
	UPROPERTY(BlueprintAssignable)
	FOnDungeonPhaseChanged OnDungeonPhaseChanged;
	UPROPERTY(BlueprintAssignable)
	FOnKillCountChanged OnKillCountChanged;
	UPROPERTY(BlueprintAssignable)
	FOnBossKilled OnBossKilled;
	UPROPERTY(BlueprintAssignable)
	FOnDeathCountChanged OnDeathCountChanged;
	UPROPERTY(BlueprintAssignable)
	FOnDungeonDepleted OnDungeonDepleted;
	UPROPERTY(BlueprintAssignable)
	FOnDungeonCompleted OnDungeonCompleted;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Dungeon Requirements", meta = (ClampMin = "1"))
	float TimeLimit = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon Requirements", meta = (Categories = "Boss"))
	TMap<FGameplayTag, FString> BossRequirements;
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon Requirements", meta = (ClampMin = "0"))
	int32 KillCountRequirement = 0;
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon Requirements", meta = (ClampMin = "0"))
	float DeathPenaltySeconds = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon Requirements", meta = (ClampMin = "0"))
	float CountdownLength = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Dungeon Information")
	FString DungeonName;
	
private:

	static const float MINIMUMTIMELIMIT;
	bool bInitialized = false;

	bool AreDungeonRequirementsMet();

	UPROPERTY(ReplicatedUsing = OnRep_DungeonProgress)
	FDungeonProgress DungeonProgress;
	UFUNCTION()
	void OnRep_DungeonProgress(const FDungeonProgress& PreviousProgress);
	FTimerHandle PhaseTimerHandle;
	void TryStartCountdown();
	bool CheckDungeonCanStart(FText& OutFailMessage) const;
	void EndCountdown();
	void CompleteDungeon();
	void DepleteDungeonFromTimer();
};