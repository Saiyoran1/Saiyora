#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "SaiyoraStructs.h"
#include "BuffStructs.h"
#include "SaiyoraObjects.generated.h"

UCLASS()
class SAIYORAV4_API UModifiableIntValue : public UObject
{
	GENERATED_BODY()
public:
	void Init(int32 const Base, bool const bModdable = false, bool const bLowCapped = false, int32 const Min = 0, bool const bHighCapped = false, int32 const Max = 0);
	void SetRecalculationFunction(FIntValueRecalculation const& NewCalculation);
	void RecalculateValue();
	void SubscribeToValueChanged(FIntValueCallback const& Callback);
	void UnsubscribeFromValueChanged(FIntValueCallback const& Callback);
	void AddModifier(FCombatModifier const& Modifier, UBuff* Source);
	void RemoveModifier(UBuff* Source);
	int32 GetValue() const { return Value; }
	bool IsModifiable() const { return bModifiable; }
private:
	int32 Value = 0;
	int32 BaseValue = 0;
	bool bModifiable = false;
	bool bHasMin = false;
	int32 Minimum = 0;
	bool bHasMax = false;
	int32 Maximum = 0;
	TMap<UBuff*, FCombatModifier> Modifiers;
	FIntValueRecalculation CustomRecalculation;
	virtual void OnValueChanged(int32 const PreviousValue) {}
};

UCLASS()
class UModifiableFloatValue : public UObject
{
	GENERATED_BODY()
public:
	void Init(float const Base, bool const bModdable = false, bool const bLowCapped = false, float const Min = 0.0f, bool const bHighCapped = false, float const Max = 0.0f);
	void SetRecalculationFunction(FFloatValueRecalculation const& NewCalculation);
	void RecalculateValue();
	void AddModifier(FCombatModifier const& Modifier, UBuff* Source);
	void RemoveModifier(UBuff* Source);
	float GetValue() const { return Value; }
	bool IsModifiable() const { return bModifiable; }
private:
	float Value = 0.0f;
	float BaseValue = 0.0f;
	bool bModifiable = false;
	bool bHasMin = false;
	float Minimum = 0.0f;
	bool bHasMax = false;
	float Maximum = 0.0f;
	TMap<UBuff*, FCombatModifier> Modifiers;
	FFloatValueRecalculation CustomRecalculation;
	virtual void OnValueChanged(float const PreviousValue) {}
};