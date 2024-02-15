#pragma once
#include "CoreMinimal.h"
#include "CombatStructs.h"
#include "ThreatEnums.h"
#include "ThreatStructs.generated.h"

class UBuff;
class UThreatHandler;

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
	ESaiyoraPlane AppliedByPlane = ESaiyoraPlane::Both;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	ESaiyoraPlane AppliedToPlane = ESaiyoraPlane::Both;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	EThreatType ThreatType = EThreatType::None;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	float Threat = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	bool bSuccess = false;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	bool bInitialThreat = false;
};

USTRUCT(BlueprintType)
struct FThreatTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	UThreatHandler* TargetThreat = nullptr;
	UPROPERTY(BlueprintReadOnly)
	float Threat = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	TArray<UBuff*> Fixates;
	UPROPERTY(BlueprintReadOnly)
	TArray<UBuff*> Blinds;
	UPROPERTY(BlueprintReadOnly)
	bool Faded = false;

	FThreatTarget() {}
	FThreatTarget(UThreatHandler* NewTarget);

	FORCEINLINE bool operator<(const FThreatTarget& Other) const { return LessThan(Other); }
	bool LessThan(const FThreatTarget& Other) const;
};

USTRUCT()
struct FMisdirect
{
	GENERATED_BODY()

	UPROPERTY()
	UBuff* Source = nullptr;
	UPROPERTY()
	AActor* Target = nullptr;

	FMisdirect() {}
	FMisdirect(UBuff* SourceBuff, AActor* TargetActor) : Source(SourceBuff), Target(TargetActor) {}

	FORCEINLINE bool operator==(FMisdirect const& Other) const { return Other.Source == Source; }
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FThreatRestriction, FThreatEvent const&, ThreatEvent);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FThreatModCondition, FThreatEvent const&, ThreatEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatStatusNotification, UThreatHandler*, Handler, const bool, NewCombatStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTargetNotification, AActor*, PreviousTarget, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFadeNotification, AActor*, Target, const bool, Faded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVanishNotification, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActorCombatNotification, AActor*, Actor, const bool, NewCombatStatus);

//Threat caused by damage and healing can have a separate base threat value, an optional modifier from the source, and the option to ignore modifiers and restrictions.
USTRUCT(BlueprintType)
struct FThreatFromDamage
{
	GENERATED_BODY()

	UPROPERTY()
	bool GeneratesThreat = true;
	UPROPERTY()
	bool SeparateBaseThreat = false;
	UPROPERTY()
	float BaseThreat = 0.0f;
	UPROPERTY()
	bool IgnoreModifiers = false;
	UPROPERTY()
	bool IgnoreRestrictions = false;
	UPROPERTY()
	FThreatModCondition SourceModifier;
};