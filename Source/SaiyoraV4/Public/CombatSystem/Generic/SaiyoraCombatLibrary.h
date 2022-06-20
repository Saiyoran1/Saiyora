#pragma once
#include "CombatEnums.h"
#include "CombatStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraCombatLibrary.generated.h"

class UBuff;
class UBuffFunction;

UCLASS()
class SAIYORAV4_API USaiyoraCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	//Net

	UFUNCTION(BlueprintPure, Category = "Time")
	static float GetActorPing(const AActor* Actor);

	//Modifier

	UFUNCTION(BlueprintPure, Category = "Modifier", meta = (DefaultToSelf = "Source", HidePin = "Source", NativeMakeFunc))
	static FCombatModifier MakeCombatModifier(UBuff* Source, const EModifierType ModifierType, const float ModifierValue, const bool bStackable);
	UFUNCTION(BlueprintPure, Category = "Modifier", meta = (DefaultToSelf = "Source", HidePin = "Source", NativeMakeFunc))
	static FCombatModifier MakeBuffFunctionCombatModifier(const UBuffFunction* Source, const EModifierType ModifierType, const float ModifierValue, const bool bStackable);
};


