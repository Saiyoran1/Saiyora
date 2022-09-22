#pragma once
#include "CombatEnums.h"
#include "CombatStructs.h"
#include "Engine/EngineTypes.h"
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

	UFUNCTION(BlueprintPure, Category = "Modifier", meta = (NativeMakeFunc))
	static FCombatModifier MakeCombatModifier(const EModifierType ModifierType, const float ModifierValue);
	UFUNCTION(BlueprintPure, Category = "Modifier", meta = (DefaultToSelf = "Source", HidePin = "Source", NativeMakeFunc))
	static FCombatModifier MakeBuffCombatModifier(UBuff* Source, const EModifierType ModifierType, const float ModifierValue, const bool bStackable);
	UFUNCTION(BlueprintPure, Category = "Modifier", meta = (DefaultToSelf = "Source", HidePin = "Source", NativeMakeFunc))
	static FCombatModifier MakeBuffFunctionCombatModifier(const UBuffFunction* Source, const EModifierType ModifierType, const float ModifierValue, const bool bStackable);

	//Attachment

	UFUNCTION(BlueprintCallable, Category = "Attachment")
	static void AttachCombatActorToComponent(AActor* Target, USceneComponent* Parent, const FName SocketName, const EAttachmentRule LocationRule,
		const EAttachmentRule RotationRule, const EAttachmentRule ScaleRule, const bool bWeldSimulatedBodies);
};


