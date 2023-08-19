#pragma once
#include "CombatStructs.h"
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
	UFUNCTION(BlueprintPure, Category = "Player")
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
};


