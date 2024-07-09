#pragma once
#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "NPCEnums.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "NPCStructs.generated.h"

class UNPCAbilityComponent;
class UNPCAbility;
class UCombatAbility;
class AAIController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatBehaviorNotification, const ENPCCombatBehavior, PreviousStatus, const ENPCCombatBehavior, NewStatus);

//A combat choice is just the ability we want to cast and the requirements to cast that ability.
USTRUCT()
struct FNPCCombatChoice
{
	GENERATED_BODY()

public:

	void Init(UNPCAbilityComponent* AbilityComponent);
	
	TSubclassOf<UNPCAbility> GetAbilityClass() const { return AbilityClass; }
	bool IsChoiceValid() const;

	FString DEBUG_GetDisplayName() const { return DEBUG_ChoiceName; }

private:

	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/SaiyoraV4.NPCChoiceRequirement", ExcludeBaseStruct))
	TArray<FInstancedStruct> Requirements;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UNPCAbility> AbilityClass;
	
	UPROPERTY()
	UNPCAbilityComponent* OwningComponentRef = nullptr;
	bool bInitialized = false;

	UPROPERTY(EditAnywhere)
	FString DEBUG_ChoiceName = "";
};

USTRUCT(BlueprintType)
struct FRotationLock
{
	GENERATED_BODY()

	FORCEINLINE bool operator==(const FRotationLock& Other) const { return Other.LockID == LockID; }

	static FRotationLock GenerateRotationLock()
	{
		static int LockCounter = 0;
		return FRotationLock(LockCounter++);
	}

	FRotationLock() {}
	
private:

	int LockID = -1;
	FRotationLock(const int InID) : LockID(InID) {}
};

#pragma region Patrol

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolStateNotification, AActor*, PatrollingActor, const ENPCPatrolSubstate, PatrolSubstate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPatrolLocationNotification, AActor*, PatrollingActor, const FVector&, Location);

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

#pragma endregion
#pragma region Tokens

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

#pragma endregion
#pragma region Query Params

USTRUCT()
struct FNPCQueryParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName ParamName;

	virtual void AddParam(FEnvQueryRequest& Request) const {}

	virtual ~FNPCQueryParam() {}
};

USTRUCT()
struct FNPCFloatParam : public FNPCQueryParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Value = 0.0f;

	virtual void AddParam(FEnvQueryRequest& Request) const override
	{
		Request.SetFloatParam(ParamName, Value);
	}
};

USTRUCT()
struct FNPCIntParam : public FNPCQueryParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 Value = 0;

	virtual void AddParam(FEnvQueryRequest& Request) const override
	{
		Request.SetIntParam(ParamName, Value);
	}
};

USTRUCT()
struct FNPCBoolParam : public FNPCQueryParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	bool bValue = false;

	virtual void AddParam(FEnvQueryRequest& Request) const override
	{
		Request.SetBoolParam(ParamName, bValue);
	}
};

#pragma endregion