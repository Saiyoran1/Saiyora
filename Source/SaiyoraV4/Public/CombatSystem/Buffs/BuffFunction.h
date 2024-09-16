#pragma once
#include "BuffStructs.h"
#include "BuffFunction.generated.h"

class UCombatAbility;
class UBuffHandler;

#pragma region UBuffFunction

//Base class for objects that represent some functionality a buff wants to perform that can be reused by multiple buffs.
UCLASS(Abstract)
class SAIYORAV4_API UBuffFunction : public UObject
{
	GENERATED_BODY()

public:

	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintPure, Category = "Buff")
	UBuff* GetOwningBuff() const { return OwningBuff; }

	//Static creation function called by all derived factory functions to create the instance and call InitializeBuffFunction.
	static UBuffFunction* InstantiateBuffFunction(UBuff* Buff, const TSubclassOf<UBuffFunction> FunctionClass);
	//Called by InstantiateBuffFunction to set up refs and establish a connection to the owning buff.
	void InitializeBuffFunction(UBuff* BuffRef);
	//Pure virtual function called during init for child classes to setup functionality
	virtual void SetupBuffFunction() {}
	//Pure virtual function called during initial application for child classes to use
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) {}
	//Pure virtual function called any time the buff's stacks change for child classes to use
	virtual void OnStack(const FBuffApplyEvent& ApplyEvent) {}
	//Pure virtual function called any time the buff's duration is updated for child classes to use
	virtual void OnRefresh(const FBuffApplyEvent& ApplyEvent) {}
	//Pure virtual function called when the buff is removed for child classes to use
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) {}
	//Pure virtual function called after removal to cleanup refs and bindings in child classes
	virtual void CleanupBuffFunction() {}

private:

	UPROPERTY()
	UBuff* OwningBuff = nullptr;
	bool bBuffFunctionInitialized = false;
};

#pragma endregion
#pragma region UBuffRestrictionFunction

//Buff function for adding a conditional restriction to incoming or outgoing buffs
UCLASS()
class UBuffRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	//Factory function to create and instantiate a BuffRestriction buff function
	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void BuffRestriction(UBuff* Buff, const ECombatEventDirection RestrictionDirection, const FBuffRestriction& Restriction);

private:

	//Whether this restriction is on incoming or outgoing buffs
	ECombatEventDirection Direction = ECombatEventDirection::Incoming;
	//The actual restriction function to apply
	FBuffRestriction Restrict;
	UPROPERTY()
	UBuffHandler* TargetComponent = nullptr;

	//Called during setup to copy variables into the created buff function instance.
	void SetRestrictionVars(const ECombatEventDirection RestrictionDirection, const FBuffRestriction& Restriction);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;
};

#pragma endregion

USTRUCT()
struct FBuffFunction
{
	GENERATED_BODY()

public:

	void Init(UBuff* Buff) { OwningBuff = Buff; }
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) {}
	virtual void OnChange(const FBuffApplyEvent& ChangeEvent) {}
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) {}

	virtual ~FBuffFunction() {}

protected:

	UBuff* GetOwningBuff() const { return OwningBuff; }

private:

	UPROPERTY()
	UBuff* OwningBuff = nullptr;
};

UCLASS()
class UBuffFunctionLibrary : public UObject
{
	GENERATED_BODY()

public:
	
#pragma region Complex Ability Modifiers

	UFUNCTION()
	FCombatModifier TestComplexAbilityMod(UCombatAbility* Ability);

#pragma endregion 
};