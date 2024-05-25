#pragma once
#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "NPCEnums.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "NPCStructs.generated.h"

class UNPCAbilityComponent;
class UNPCAbility;
class UCombatAbility;
class AAIController;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolStateNotification, AActor*, PatrollingActor, const ENPCPatrolSubstate, PatrolSubstate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolLocationNotification, AActor*, PatrollingActor, const FVector&, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatBehaviorNotification, const ENPCCombatBehavior, PreviousStatus, const ENPCCombatBehavior, NewStatus);

//Experimenting with dodging behavior trees and just using a priority system for both movement AND abilities.
//A combat choice can be an ability we want to cast, which includes whether we would need to move into a specific spot before casting and a movement behavior to use during the cast,
//or it can just be a movement the NPC wants to perform. Each choice has some requirements that must be met to perform this choice.
USTRUCT()
struct FNPCCombatChoice
{
	GENERATED_BODY()

public:

	void Init(UNPCAbilityComponent* AbilityComponent);

	bool HasPreCastQuery() const { return bPreCastQuery; }
	UEnvQuery* GetPreMoveQuery(TArray<FAIDynamicParam>& OutParams) const { OutParams = PreCastQueryParams; return PreCastQuery; }
	bool ShouldRepeatPreCastQuery() const { return bRepeatPreCastQuery; }

	/*bool HasDuringCastQuery() const { return bDuringCastQuery; }
	UEnvQuery* GetDuringCastQuery(TArray<FAIDynamicParam>& OutParams) const { OutParams = DuringCastQueryParams; return DuringCastQuery; }
	bool ShouldRepeatDuringCastQuery() const { return bRepeatDuringCastQuery; }*/

	bool HasAbility() const;
	TSubclassOf<UNPCAbility> GetAbilityClass() const { return AbilityClass; }
	
	bool IsChoiceValid() const;

	FString DEBUG_GetDisplayName() const { return DEBUG_ChoiceName; }

private:

	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCChoiceRequirement"))
	TArray<FInstancedStruct> Requirements;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UNPCAbility> AbilityClass;
	
	UPROPERTY(EditAnywhere)
	bool bPreCastQuery = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bPreCastQuery"))
	TObjectPtr<UEnvQuery> PreCastQuery;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bPreCastQuery"))
	TArray<FAIDynamicParam> PreCastQueryParams;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bPreCastQuery"))
	bool bRepeatPreCastQuery = false;
	
	/*UPROPERTY(EditAnywhere)
	bool bDuringCastQuery = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bDuringCastQuery"))
	TObjectPtr<UEnvQuery> DuringCastQuery;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bDuringCastQuery"))
	TArray<FAIDynamicParam> DuringCastQueryParams;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bDuringCastQuery"))
	bool bRepeatDuringCastQuery = false;*/
	
	UPROPERTY()
	UNPCAbilityComponent* OwningComponentRef = nullptr;
	bool bInitialized = false;

	UPROPERTY(EditAnywhere)
	FString DEBUG_ChoiceName = "";
};

//A struct that is intended to be inherited from to be used as FInstancedStructs inside FNPCCombatChoice.
//These requirements are event-based, and must be set up in c++. They basically constantly update their owning choice with whether they are met.
USTRUCT()
struct FNPCChoiceRequirement
{
	GENERATED_BODY()

public:
	
	void Init(FNPCCombatChoice* Choice, AActor* NPC);
	virtual bool IsMet() const { return false; }
	
	virtual ~FNPCChoiceRequirement() {}

	FString DEBUG_GetReqName() const { return DEBUG_RequirementName; }

protected:
	
	AActor* GetOwningNPC() const { return OwningNPC; }
	FNPCCombatChoice GetOwningChoice() const { return *OwningChoice; }

	FString DEBUG_RequirementName = "";

private:

	virtual void SetupRequirement() {}
	
	bool bIsMet = false;
	UPROPERTY()
	AActor* OwningNPC = nullptr;
	FNPCCombatChoice* OwningChoice = nullptr;
};

//A struct that is intended to be inherited from to be used as FInstancedStructs for NPCs to use.
//Using these as context for combat choice requirements is the intended use case but they can probably be used for more.
USTRUCT()
struct FNPCTargetContext
{
	GENERATED_BODY()

public:

	virtual AActor* GetBestTarget(const AActor* Querier) const { return nullptr; }
	virtual ~FNPCTargetContext() {}
	
private:
	
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