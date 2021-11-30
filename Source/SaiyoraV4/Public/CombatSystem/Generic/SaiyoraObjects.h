#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "SaiyoraStructs.h"
#include "BuffStructs.h"
#include "SaiyoraObjects.generated.h"

/*UCLASS()
class SAIYORAV4_API UModifiableIntValue : public UObject
{
	GENERATED_BODY()
public:
	void Init(int32 const Base, bool const bModdable = false, bool const bLowCapped = false, int32 const Min = 0, bool const bHighCapped = false, int32 const Max = 0);
	void SetRecalculationFunction(FIntValueRecalculation const& NewCalculation);
	void RecalculateValue(bool const bChangedPredictionID = false);
	void AddModifier(FCombatModifier const& Modifier, UBuff* Source);
	void RemoveModifier(UBuff* Source);
	int32 GetValue() const { return Value; }
	bool IsModifiable() const { return bModifiable; }
	int32 GetLastPredictionID() const { return LastPredictionID; }
private:
	int32 Value = 0;
	int32 BaseValue = 0;
	bool bModifiable = false;
	bool bHasMin = false;
	int32 Minimum = 0;
	bool bHasMax = false;
	int32 Maximum = 0;
	TMap<UBuff*, FCombatModifier> Modifiers;
	int32 LastPredictionID = 0;
	FIntValueRecalculation CustomRecalculation;
	virtual void OnUpdated(int32 const PreviousValue, int32 const PredictionID) {}
};

UCLASS()
class UModifiableFloatValue : public UObject
{
	GENERATED_BODY()
public:
	void Init(float const Base, bool const bModdable = false, bool const bLowCapped = false, float const Min = 0.0f, bool const bHighCapped = false, float const Max = 0.0f);
	void SetRecalculationFunction(FFloatValueRecalculation const& NewCalculation);
	void RecalculateValue(bool const bChangedPredictionID = false);
	void AddModifier(FCombatModifier const& Modifier, UBuff* Source);
	void RemoveModifier(UBuff* Source);
	float GetValue() const { return Value; }
	bool IsModifiable() const { return bModifiable; }
	int32 GetLastPredictionID() const { return LastPredictionID; }
private:
	float Value = 0.0f;
	float BaseValue = 0.0f;
	bool bModifiable = false;
	bool bHasMin = false;
	float Minimum = 0.0f;
	bool bHasMax = false;
	float Maximum = 0.0f;
	TMap<UBuff*, FCombatModifier> Modifiers;
	int32 LastPredictionID = 0;
	FFloatValueRecalculation CustomRecalculation;
	virtual void OnValueChanged(float const PreviousValue, int32 const PredictionID) {}
};*/