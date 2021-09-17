#pragma once
#include "CoreMinimal.h"
#include "SaiyoraStructs.h"
#include "ThreatEnums.h"
#include "ThreatStructs.generated.h"

USTRUCT(BlueprintType)
struct FThreatEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	AActor* AppliedTo = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	AActor* AppliedBy = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	UObject* Source = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	bool AppliedXPlane = false;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::None;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::None;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	EThreatType ThreatType = EThreatType::None;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	float Threat = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	bool bSuccess = false;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	bool bInitialThreat = false;
};

USTRUCT()
struct FThreatTarget
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* Target = nullptr;
	float Threat = 0.0f;
	int32 Fixates = 0;
	int32 Blinds = 0;

	FThreatTarget();
	FThreatTarget(AActor* ThreatTarget, float const InitialThreat, int32 const InitialFixates = 0, int32 const InitialBlinds = 0);

	FORCEINLINE bool operator<(FThreatTarget const& Other) const { return LessThan(Other); }
	bool LessThan(FThreatTarget const& Other) const;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FThreatCondition, FThreatEvent const&, ThreatEvent);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FThreatModCondition, FThreatEvent const&, ThreatEvent);

DECLARE_DYNAMIC_DELEGATE_OneParam(FCombatStatusCallback, bool const, NewCombatStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCombatStatusNotification, bool const, NewCombatStatus);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FTargetCallback, AActor*, PreviousTarget, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTargetNotification, AActor*, PreviousTarget, AActor*, NewTarget);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FFadeCallback, AActor*, Actor, bool const, Faded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFadeNotification, AActor*, Actor, bool const, Faded);

DECLARE_DYNAMIC_DELEGATE_OneParam(FVanishCallback, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVanishNotification, AActor*, Actor);

//Threat caused by damage and healing can have a separate base threat value, an optional modifier from the source, and the option to ignore modifiers and restrictions.
USTRUCT(BlueprintType)
struct FThreatFromDamage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Threat")
	bool GeneratesThreat = false;
	UPROPERTY(BlueprintReadWrite, Category = "Threat")
	bool SeparateBaseThreat = false;
	UPROPERTY(BlueprintReadWrite, Category = "Threat")
	float BaseThreat = 0.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Threat")
	bool IgnoreModifiers = false;
	UPROPERTY(BlueprintReadWrite, Category = "Threat")
	bool IgnoreRestrictions = false;
	UPROPERTY(BlueprintReadWrite, Category = "Threat")
	FThreatModCondition SourceModifier;
};