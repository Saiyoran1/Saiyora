#pragma once
#include "CoreMinimal.h"
#include "SaiyoraStructs.h"
#include "ThreatEnums.h"
#include "ThreatStructs.generated.h"

class UCombatComponent;

USTRUCT(BlueprintType)
struct FThreatEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	UCombatComponent* AppliedTo = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	UCombatComponent* AppliedBy = nullptr;
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
	UCombatComponent* Target = nullptr;
	float Threat = 0.0f;
	UPROPERTY()
	TArray<UBuff*> Fixates;
	UPROPERTY()
	TArray<UBuff*> Blinds;
	bool Faded = false;

	FThreatTarget();
	FThreatTarget(UCombatComponent* ThreatTarget, float const InitialThreat, bool const bFaded = false, UBuff* InitialFixate = nullptr, UBuff* InitialBlind = nullptr);

	FORCEINLINE bool operator<(FThreatTarget const& Other) const { return LessThan(Other); }
	bool LessThan(FThreatTarget const& Other) const;
};

USTRUCT()
struct FMisdirect
{
	GENERATED_BODY()

	UPROPERTY()
	UBuff* Source;
	UPROPERTY()
	UCombatComponent* Target;

	FMisdirect();
	FMisdirect(UBuff* SourceBuff, UCombatComponent* TargetComponent);

	FORCEINLINE bool operator==(FMisdirect const& Other) const { return Other.Source == Source; }
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FThreatRestriction, FThreatEvent const&, ThreatEvent);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FCombatModifier, FThreatModCondition, FThreatEvent const&, ThreatEvent);

DECLARE_DYNAMIC_DELEGATE_OneParam(FCombatStatusCallback, bool const, NewCombatStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCombatStatusNotification, bool const, NewCombatStatus);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FTargetCallback, UCombatComponent*, PreviousTarget, UCombatComponent*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTargetNotification, UCombatComponent*, PreviousTarget, UCombatComponent*, NewTarget);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FFadeCallback, UCombatComponent*, Target, bool const, Faded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFadeNotification, UCombatComponent*, Target, bool const, Faded);

DECLARE_DYNAMIC_DELEGATE_OneParam(FVanishCallback, UCombatComponent*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVanishNotification, UCombatComponent*, Target);

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