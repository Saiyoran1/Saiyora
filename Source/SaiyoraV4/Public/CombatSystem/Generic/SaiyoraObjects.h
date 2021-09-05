// Fill out your copyright notice in the Description page of Project Settings.

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
	int32 AddModifier(FCombatModifier const& Modifier);
	void RemoveModifier(int32 const ModifierID);
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
	TMap<int32, FCombatModifier> Modifiers;
	FIntValueRecalculation CustomRecalculation;
	FIntValueNotification OnValueChanged;
	FBuffEventCallback BuffStackCallback;
	UFUNCTION()
	void CheckForModifierSourceStack(FBuffApplyEvent const& Event);
};

UCLASS()
class UModifiableFloatValue : public UObject
{
	GENERATED_BODY()
public:
	void Init(float const Base, bool const bModdable = false, bool const bLowCapped = false, float const Min = 0.0f, bool const bHighCapped = false, float const Max = 0.0f);
	void SetRecalculationFunction(FFloatValueRecalculation const& NewCalculation);
	void RecalculateValue();
	void SubscribeToValueChanged(FFloatValueCallback const& Callback);
	void UnsubscribeFromValueChanged(FFloatValueCallback const& Callback);
	int32 AddModifier(FCombatModifier const& Modifier);
	void RemoveModifier(int32 const ModifierID);
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
	TMap<int32, FCombatModifier> Modifiers;
	FFloatValueRecalculation CustomRecalculation;
	FFloatValueNotification OnValueChanged;
	FBuffEventCallback BuffStackCallback;
	UFUNCTION()
	void CheckForModifierSourceStack(FBuffApplyEvent const& Event);
};