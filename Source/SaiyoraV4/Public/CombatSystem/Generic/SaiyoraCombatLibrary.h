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

	//Gets the target actor's ping, if it is a player character. Returns 0 for all other actors.
	UFUNCTION(BlueprintPure, Category = "Time")
	static float GetActorPing(const AActor* Actor);
	//Queries the GameState's player array and gets the local player, cast to ASaiyoraPlayerCharacter.
	UFUNCTION(BlueprintPure, Category = "Player", meta = (HidePin = "WorldContext", DefaultToSelf = "WorldContext"))
	static ASaiyoraPlayerCharacter* GetLocalSaiyoraPlayer(const UObject* WorldContext);

	//Attachment

	//Wrapper around AttachToComponent that also updates plane-based visuals via the parent's CombatStatusComponent.
	UFUNCTION(BlueprintCallable, Category = "Attachment")
	static void AttachCombatActorToComponent(AActor* Target, USceneComponent* Parent, const FName SocketName, const EAttachmentRule LocationRule,
		const EAttachmentRule RotationRule, const EAttachmentRule ScaleRule, const bool bWeldSimulatedBodies);

#pragma region Combat Parameter Functions

	/*
	 * These are helper functions for creating FInstancedStructs containing the various different combat param struct types.
	 * In Blueprints this takes multiple nodes including at least one impure node by default, and isn't super straightforward.
	 * There are also functions here for getting a param of type by name from an FInstancedStruct array.
	 */

public:

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedBoolParam(const FString& Name, const bool Value) { return FInstancedStruct::Make(FCombatParameterBool(Name, Value)); }
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
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetBoolParam(const TArray<FInstancedStruct>& Params, const FString& ParamName, bool& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterBool* BoolParam = Param.GetPtr<FCombatParameterBool>();
			if (BoolParam && BoolParam->ParamName.Equals(ParamName, ESearchCase::IgnoreCase))
			{
				ParamValue = BoolParam->BoolParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedIntParam(const FString& Name, const int32 Value) { return FInstancedStruct::Make(FCombatParameterInt(Name, Value)); }
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
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetIntParam(const TArray<FInstancedStruct>& Params, const FString& ParamName, int32& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterInt* IntParam = Param.GetPtr<FCombatParameterInt>();
			if (IntParam && IntParam->ParamName.Equals(ParamName, ESearchCase::IgnoreCase))
			{
				ParamValue = IntParam->IntParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedFloatParam(const FString& Name, const float Value) { return FInstancedStruct::Make(FCombatParameterFloat(Name, Value)); }
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
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetFloatParam(const TArray<FInstancedStruct>& Params, const FString& ParamName, float& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterFloat* FloatParam = Param.GetPtr<FCombatParameterFloat>();
			if (FloatParam && FloatParam->ParamName.Equals(ParamName, ESearchCase::IgnoreCase))
			{
				ParamValue = FloatParam->FloatParam;
				return true;
			}
		}
		return false;
	}
	
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedStringParam(const FString& Name, const FString& Value) { return FInstancedStruct::Make(FCombatParameterString(Name, Value)); }
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
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetStringParam(const TArray<FInstancedStruct>& Params, const FString& ParamName, FString& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterString* StringParam = Param.GetPtr<FCombatParameterString>();
			if (StringParam && StringParam->ParamName.Equals(ParamName, ESearchCase::IgnoreCase))
			{
				ParamValue = StringParam->StringParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedVectorParam(const FString& Name, const FVector& Value) { return FInstancedStruct::Make(FCombatParameterVector(Name, Value)); }
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
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetVectorParam(const TArray<FInstancedStruct>& Params, const FString& ParamName, FVector& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterVector* VectorParam = Param.GetPtr<FCombatParameterVector>();
			if (VectorParam && VectorParam->ParamName.Equals(ParamName, ESearchCase::IgnoreCase))
			{
				ParamValue = VectorParam->VectorParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedRotatorParam(const FString& Name, const FRotator& Value) { return FInstancedStruct::Make(FCombatParameterRotator(Name, Value)); }
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
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetRotatorParam(const TArray<FInstancedStruct>& Params, const FString& ParamName, FRotator& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterRotator* RotatorParam = Param.GetPtr<FCombatParameterRotator>();
			if (RotatorParam && RotatorParam->ParamName.Equals(ParamName, ESearchCase::IgnoreCase))
			{
				ParamValue = RotatorParam->RotatorParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedObjectParam(const FString& Name, UObject* Value) { return FInstancedStruct::Make(FCombatParameterObject(Name, Value)); }
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
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetObjectParam(const TArray<FInstancedStruct>& Params, const FString& ParamName, UObject*& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterObject* ObjectParam = Param.GetPtr<FCombatParameterObject>();
			if (ObjectParam && ObjectParam->ParamName.Equals(ParamName, ESearchCase::IgnoreCase))
			{
				ParamValue = ObjectParam->ObjectParam;
				return true;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	static FInstancedStruct MakeInstancedClassParam(const FString& Name, const TSubclassOf<UObject> Value) { return FInstancedStruct::Make(FCombatParameterClass(Name, Value)); }
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
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool GetClassParam(const TArray<FInstancedStruct>& Params, const FString& ParamName, TSubclassOf<UObject>& ParamValue)
	{
		for (const FInstancedStruct& Param : Params)
		{
			const FCombatParameterClass* ClassParam = Param.GetPtr<FCombatParameterClass>();
			if (ClassParam && ClassParam->ParamName.Equals(ParamName, ESearchCase::IgnoreCase))
			{
				ParamValue = ClassParam->ClassParam;
				return true;
			}
		}
		return false;
	}

#pragma endregion 
};


