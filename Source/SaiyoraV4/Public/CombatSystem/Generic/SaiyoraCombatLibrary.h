#pragma once
#include "CombatStructs.h"
#include "InstancedStruct.h"
#include "NPCEnums.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraCombatLibrary.generated.h"

class ASaiyoraPlayerCharacter;

UCLASS()
class SAIYORAV4_API USaiyoraCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	//Gets the target actor's ping, if it is a player character. Returns 0 for all other actors.
	UFUNCTION(BlueprintPure, Category = "Time")
	static float GetActorPing(const AActor* Actor);
	
	//Queries the GameState's player array and gets the local player, cast to ASaiyoraPlayerCharacter.
	UFUNCTION(BlueprintPure, Category = "Player", meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static ASaiyoraPlayerCharacter* GetLocalSaiyoraPlayer(const UObject* WorldContext);
	
	//Wrapper around AttachToComponent that also updates plane-based visuals via the parent's CombatStatusComponent.
	UFUNCTION(BlueprintCallable, Category = "Attachment")
	static void AttachCombatActorToComponent(AActor* Target, USceneComponent* Parent, const FName SocketName, const EAttachmentRule LocationRule,
		const EAttachmentRule RotationRule, const EAttachmentRule ScaleRule, const bool bWeldSimulatedBodies);

	//Helper to check whether a certain NPC behavior should be treated as valid for certain actions such as taking damage or generating/receiving threat.
	static bool IsActiveNPCBehavior(const ENPCCombatBehavior Behavior) { return Behavior == ENPCCombatBehavior::Patrolling || Behavior == ENPCCombatBehavior::Combat; }

	static FString GetCombatantName(const AActor* Combatant);

	//Helper for bitmask enums to convert the enum value to a bit.
	template <typename T>
	FORCEINLINE static int32 EnumValueToBit(const T Enum)
	{
		return 1 << static_cast<uint8>(Enum);
	}
	//Helper to get an untranslated string from an enum value.
	template <typename T>
	FORCEINLINE static FString EnumValueToString(const T Enum)
	{
		return StaticEnum<T>()->GetNameStringByValue(static_cast<int64>(Enum));
	}

#pragma region Editor Helpers

public:

	//Get a list of names of functions on the provided object that match the signature of the provided example function.
	static TArray<FName> GetMatchingFunctionNames(const UObject* Object, const UFunction* ExampleFunction);
	//Check if two function signatures are the same. Useful for populating lists of modifier/restriction functions with appropriate function names.
	static bool IsSignatureTheSame(const UFunction* Function, const UFunction* Example);

#pragma endregion 
#pragma region Log Helpers

public:

	//Helper that just converts a bool to a string for easier logging
	static const TCHAR* BoolToServerOrClientString(const bool bServer) { return bServer ? TEXT("SERVER") : TEXT("CLIENT"); }
	//Logs a delegate formatted as { Object: %s, Function: %s }
	static FString DelegateToLogString(const FScriptDelegate& Delegate);

#pragma endregion 
#pragma region Combat Parameter Functions

	/*
	 * These are helper functions for creating FInstancedStructs containing the various different combat param struct types.
	 * In Blueprints this takes multiple nodes including at least one impure node by default, and isn't super straightforward.
	 * There are also functions here for getting a param of type by tag from an FInstancedStruct array.
	 */

public:

	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static FInstancedStruct MakeInstancedBoolParam(const FGameplayTag Tag, const bool Value) { return FInstancedStruct::Make(FCombatParameterBool(Tag, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceBoolParam(const FCombatParameterBool& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetBoolParamFromInstance(const FInstancedStruct& Instance, FCombatParameterBool& OutParam)
	{
		if (Instance.GetScriptStruct() == FCombatParameterBool::StaticStruct())
		{
			OutParam = Instance.Get<FCombatParameterBool>();
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static bool GetBoolParam(const TArray<FInstancedStruct>& Params, const FGameplayTag Tag, bool& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterBool* BoolParam = Param.GetPtr<FCombatParameterBool>();
			if (BoolParam && BoolParam->ParamTag.MatchesTagExact(Tag))
			{
				ParamValue = BoolParam->BoolParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static FInstancedStruct MakeInstancedIntParam(const FGameplayTag Tag, const int32 Value) { return FInstancedStruct::Make(FCombatParameterInt(Tag, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceIntParam(const FCombatParameterInt& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetIntParamFromInstance(const FInstancedStruct& Instance, FCombatParameterInt& OutParam)
	{
		if (Instance.GetScriptStruct() == FCombatParameterInt::StaticStruct())
		{
			OutParam = Instance.Get<FCombatParameterInt>();
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static bool GetIntParam(const TArray<FInstancedStruct>& Params, const FGameplayTag Tag, int32& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterInt* IntParam = Param.GetPtr<FCombatParameterInt>();
			if (IntParam && IntParam->ParamTag.MatchesTagExact(Tag))
			{
				ParamValue = IntParam->IntParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static FInstancedStruct MakeInstancedFloatParam(const FGameplayTag Tag, const float Value) { return FInstancedStruct::Make(FCombatParameterFloat(Tag, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceFloatParam(const FCombatParameterFloat& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetFloatParamFromInstance(const FInstancedStruct& Instance, FCombatParameterFloat& OutParam)
	{
		if (Instance.GetScriptStruct() == FCombatParameterFloat::StaticStruct())
		{
			OutParam = Instance.Get<FCombatParameterFloat>();
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static bool GetFloatParam(const TArray<FInstancedStruct>& Params, const FGameplayTag Tag, float& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterFloat* FloatParam = Param.GetPtr<FCombatParameterFloat>();
			if (FloatParam && FloatParam->ParamTag.MatchesTagExact(Tag))
			{
				ParamValue = FloatParam->FloatParam;
				return true;
			}
		}
		return false;
	}
	
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static FInstancedStruct MakeInstancedStringParam(const FGameplayTag Tag, const FString& Value) { return FInstancedStruct::Make(FCombatParameterString(Tag, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceStringParam(const FCombatParameterString& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetStringParamFromInstance(const FInstancedStruct& Instance, FCombatParameterString& OutParam)
	{
		if (Instance.GetScriptStruct() == FCombatParameterString::StaticStruct())
		{
			OutParam = Instance.Get<FCombatParameterString>();
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static bool GetStringParam(const TArray<FInstancedStruct>& Params, const FGameplayTag Tag, FString& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterString* StringParam = Param.GetPtr<FCombatParameterString>();
			if (StringParam && StringParam->ParamTag.MatchesTagExact(Tag))
			{
				ParamValue = StringParam->StringParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static FInstancedStruct MakeInstancedVectorParam(const FGameplayTag Tag, const FVector& Value) { return FInstancedStruct::Make(FCombatParameterVector(Tag, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceVectorParam(const FCombatParameterVector& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetVectorParamFromInstance(const FInstancedStruct& Instance, FCombatParameterVector& OutParam)
	{
		if (Instance.GetScriptStruct() == FCombatParameterVector::StaticStruct())
		{
			OutParam = Instance.Get<FCombatParameterVector>();
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static bool GetVectorParam(const TArray<FInstancedStruct>& Params, const FGameplayTag Tag, FVector& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterVector* VectorParam = Param.GetPtr<FCombatParameterVector>();
			if (VectorParam && VectorParam->ParamTag.MatchesTagExact(Tag))
			{
				ParamValue = VectorParam->VectorParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static FInstancedStruct MakeInstancedRotatorParam(const FGameplayTag Tag, const FRotator& Value) { return FInstancedStruct::Make(FCombatParameterRotator(Tag, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceRotatorParam(const FCombatParameterRotator& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetRotatorParamFromInstance(const FInstancedStruct& Instance, FCombatParameterRotator& OutParam)
	{
		if (Instance.GetScriptStruct() == FCombatParameterRotator::StaticStruct())
		{
			OutParam = Instance.Get<FCombatParameterRotator>();
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static bool GetRotatorParam(const TArray<FInstancedStruct>& Params, const FGameplayTag Tag, FRotator& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterRotator* RotatorParam = Param.GetPtr<FCombatParameterRotator>();
			if (RotatorParam && RotatorParam->ParamTag.MatchesTagExact(Tag))
			{
				ParamValue = RotatorParam->RotatorParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static FInstancedStruct MakeInstancedObjectParam(const FGameplayTag Tag, UObject* Value) { return FInstancedStruct::Make(FCombatParameterObject(Tag, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceObjectParam(const FCombatParameterObject& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetObjectParamFromInstance(const FInstancedStruct& Instance, FCombatParameterObject& OutParam)
	{
		if (Instance.GetScriptStruct() == FCombatParameterObject::StaticStruct())
		{
			OutParam = Instance.Get<FCombatParameterObject>();
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static bool GetObjectParam(const TArray<FInstancedStruct>& Params, const FGameplayTag Tag, UObject*& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterObject* ObjectParam = Param.GetPtr<FCombatParameterObject>();
			if (ObjectParam && ObjectParam->ParamTag.MatchesTagExact(Tag))
			{
				ParamValue = ObjectParam->ObjectParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static FInstancedStruct MakeInstancedClassParam(const FGameplayTag Tag, const TSubclassOf<UObject> Value) { return FInstancedStruct::Make(FCombatParameterClass(Tag, Value)); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct InstanceClassParam(const FCombatParameterClass& Param) { return FInstancedStruct::Make(Param); }
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetClassParamFromInstance(const FInstancedStruct& Instance, FCombatParameterClass& OutParam)
	{
		if (Instance.GetScriptStruct() == FCombatParameterClass::StaticStruct())
		{
			OutParam = Instance.Get<FCombatParameterClass>();
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintPure, Category = "Combat", meta = (GameplayTagFilter = "Param"))
	static bool GetClassParam(const TArray<FInstancedStruct>& Params, const FGameplayTag Tag, TSubclassOf<UObject>& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterClass* ClassParam = Param.GetPtr<FCombatParameterClass>();
			if (ClassParam && ClassParam->ParamTag.MatchesTagExact(Tag))
			{
				ParamValue = ClassParam->ClassParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FString CombatParamToString(const FInstancedStruct& Param)
	{
		const UScriptStruct* Struct = Param.GetScriptStruct();
		if (!Struct)
		{
			return "None";
		}
		FString ValueString;
		Struct->ExportText(ValueString, Param.GetMemory(), nullptr, nullptr, PPF_None, nullptr);
		return FString::Printf(L"Type: %s, Value: %s", *Struct->GetName(), *ValueString);
	}

#pragma endregion
#pragma region Unique Event IDs

public:

	static int GetUniqueCombatEventID() { return UniqueCombatEventID++; }
	
private:
	
	static int UniqueCombatEventID;

#pragma endregion 
};


