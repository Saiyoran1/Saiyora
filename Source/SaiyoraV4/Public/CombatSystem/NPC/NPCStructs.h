#pragma once
#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "NPCEnums.h"
#include "Engine/TargetPoint.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "NPCStructs.generated.h"

class UNPCAbilityComponent;
class UNPCAbility;
class UCombatAbility;
class AAIController;

UENUM(BlueprintType)
enum class EPatrolSubstate : uint8
{
	None,
	MovingToPoint,
	WaitingAtPoint,
	PatrolFinished
};

USTRUCT(BlueprintType)
struct FPatrolPoint
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FVector Location = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WaitTime = 0.0f;

	FPatrolPoint() {}
	FPatrolPoint(const FVector& InLoc) : Location(InLoc) {}
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FAbilityTokenCallback, const bool, bTokensAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityTokenNotification, const bool, bTokensAvailable);

USTRUCT()
struct FNPCAbilityToken
{
	GENERATED_BODY()

	bool bAvailable = true;
	FTimerHandle CooldownHandle;
	UPROPERTY()
	UNPCAbility* OwningInstance = nullptr;
};

USTRUCT()
struct FNPCAbilityTokens
{
	GENERATED_BODY()

	TArray<FNPCAbilityToken> Tokens;
	int32 AvailableCount = 0;
	FAbilityTokenNotification OnTokenAvailabilityChanged;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolStateNotification, AActor*, PatrollingActor, const EPatrolSubstate, PatrolSubstate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolLocationNotification, AActor*, PatrollingActor, const FVector&, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatBehaviorNotification, const ENPCCombatBehavior, PreviousStatus, const ENPCCombatBehavior, NewStatus);

//Experimenting with dodging behavior trees and just using a priority system for both movement AND abilities.
//A combat choice can be an ability we want to cast, which includes whether we would need to move into a specific spot before casting and a movement behavior to use during the cast,
//or it can just be a movement the NPC wants to perform. Each choice has some requirements that must be met to perform this choice.
//Choices need to update their owning NPCAbilityComponent with whether they are valid or not, then the component can decide to override the current choice or not with newly available choices.
USTRUCT()
struct FNPCCombatChoice
{
	GENERATED_BODY()

public:

	void Init(UNPCAbilityComponent* AbilityComponent, const int Index);
	void Execute() {/*TODO*/}
	void Abort() {/*TODO*/}
	void UpdateRequirementMet(const int RequirementIdx, const bool bRequirementMet);

	bool RequiresPreMove() const { return bPreCastMove; }
	UEnvQuery* GetPreMoveQuery(TArray<FInstancedStruct>& OutParams) const { OutParams = PreCastQueryParams; return PreCastQuery; }
	
	bool IsHighPriority() const { return bHighPriority; }
	bool CanAbortDuringCast() const { return bCastCanBeInterrupted; }
	bool IsChoiceValid() const { return bValid; }

private:

	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCChoiceRequirement"))
	TArray<FInstancedStruct> Requirements;
	UPROPERTY(EditAnywhere)
	bool bHighPriority = false;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UNPCAbility> AbilityClass;
	UPROPERTY(EditAnywhere)
	bool bCastCanBeInterrupted = false;
	
	UPROPERTY(EditAnywhere)
	bool bPreCastMove = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bPreCastMove"))
	TObjectPtr<UEnvQuery> PreCastQuery;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bPreCastMove", BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
	TArray<FInstancedStruct> PreCastQueryParams;
	
	UPROPERTY(EditAnywhere)
	bool bDuringCastMove = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bDuringCastMove"))
	TObjectPtr<UEnvQuery> DuringCastQuery;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bDuringCastMove", BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
	TArray<FInstancedStruct> DuringCastQueryParams;

	bool bValid = false;
	TMap<int, bool> RequirementsMap;
	int Priority = -1;
	UPROPERTY()
	UNPCAbilityComponent* OwningComponentRef = nullptr;
	bool bInitialized = false;
};

//A struct that is intended to be inherited from to be used as FInstancedStructs inside FNPCCombatChoice.
//These requirements are event-based, and must be set up in c++. They basically constantly update their owning choice with whether they are met.
USTRUCT()
struct FNPCChoiceRequirement
{
	GENERATED_BODY()

public:

	bool IsMet() const { return bIsMet; }
	void Init(FNPCCombatChoice* Choice, AActor* NPC, const int Idx);
	virtual void SetupRequirement() {}
	virtual ~FNPCChoiceRequirement() {}

protected:

	void UpdateRequirementMet(const bool bMet);
	AActor* GetOwningNPC() const { return OwningNPC; }
	FNPCCombatChoice GetOwningChoice() const { return *OwningChoice; }

private:

	bool bIsMet = false;
	int RequirementIndex = -1;
	UPROPERTY()
	AActor* OwningNPC = nullptr;
	FNPCCombatChoice* OwningChoice = nullptr;
};