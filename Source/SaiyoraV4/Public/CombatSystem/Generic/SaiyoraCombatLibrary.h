#pragma once
#include "CombatStructs.h"
#include "InstancedStruct.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraCombatLibrary.generated.h"

class UBuff;
class UBuffFunction;
class ASaiyoraPlayerCharacter;

UCLASS()
class SAIYORAV4_API USaiyoraCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	//Net

	UFUNCTION(BlueprintPure, Category = "Time")
	static float GetActorPing(const AActor* Actor);
	UFUNCTION(BlueprintPure, Category = "Player", meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static ASaiyoraPlayerCharacter* GetLocalSaiyoraPlayer(const UObject* WorldContext);

	//Attachment

	UFUNCTION(BlueprintCallable, Category = "Attachment")
	static void AttachCombatActorToComponent(AActor* Target, USceneComponent* Parent, const FName SocketName, const EAttachmentRule LocationRule,
		const EAttachmentRule RotationRule, const EAttachmentRule ScaleRule, const bool bWeldSimulatedBodies);

	//Modifier

	UFUNCTION(BlueprintPure, Category = "Modifier")
	static bool IsModifierValid(const FCombatModifierHandle& Handle) { return Handle.IsValid(); }
	UFUNCTION(BlueprintPure, Category = "Modifier")
	static FCombatModifierHandle GetInvalidModifier() { return FCombatModifierHandle::Invalid; }

#pragma region Parameter Make Functions

	/*
	 * These are helper functions for creating FInstancedStructs containing the various different combat param struct types.
	 * In Blueprints this takes multiple nodes including at least one impure node by default, and isn't super straightforward.
	 * There are also functions here for getting a param of type by name from an FInstancedStruct array.
	 */

public:
	
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedCombatStringParam(const FString& Name, const FString& Value) { return FInstancedStruct::Make(FCombatParameterString(Name, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceCombatStringParam(const FCombatParameterString& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FCombatParameterString GetStringParamFromInstance(const FInstancedStruct& Instance)
	{
		if (Instance.GetScriptStruct() == FCombatParameterString::StaticStruct())
		{
			return Instance.Get<FCombatParameterString>();
		}
		return FCombatParameterString(FString("BadName"), FString("BadValue"));
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeCombatParameterIntInstanced(const FString& Name, const int32 Value) { return FInstancedStruct::Make(FCombatParameterInt(Name, Value)); }

#pragma endregion 
};


