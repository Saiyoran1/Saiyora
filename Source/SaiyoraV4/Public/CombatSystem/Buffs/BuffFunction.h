#pragma once
#include "BuffStructs.h"
#include "BuffFunction.generated.h"

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
#pragma region UCustomBuffFunction

//Subclass of UBuffFunction that exposes functionality to Blueprints
UCLASS(Abstract, Blueprintable)
class UCustomBuffFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	//Static blueprint-callable function to create/init a buff function of a given blueprint subclass
	UFUNCTION(BlueprintCallable, Category = "BuffFunctions")
	static void SetupCustomBuffFunction(UBuff* Buff, const TSubclassOf<UCustomBuffFunction> FunctionClass);
	//Static blueprint-callable function to create/init multiple buff functions of varying blueprint subclasses
	UFUNCTION(BlueprintCallable, Category = "BuffFunctions")
	static void SetupCustomBuffFunctions(UBuff* Buff, const TArray<TSubclassOf<UCustomBuffFunction>>& FunctionClasses);
	
	virtual void SetupBuffFunction() override { CustomSetup(); }
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override { CustomApply(ApplyEvent); }
	virtual void OnStack(const FBuffApplyEvent& ApplyEvent) override { CustomStack(ApplyEvent); }
	virtual void OnRefresh(const FBuffApplyEvent& ApplyEvent) override { CustomRefresh(ApplyEvent); }
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override { CustomRemove(RemoveEvent); }
	virtual void CleanupBuffFunction() override { CustomCleanup(); }

protected:

	//Called before OnApply to setup the buff function
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Setup"))
	void CustomSetup();
	//Called during application of the buff
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Apply"))
	void CustomApply(FBuffApplyEvent const& ApplyEvent);
	//Called when the buff's stacks change
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Stack"))
	void CustomStack(FBuffApplyEvent const& ApplyEvent);
	//Called when the buff's duration is updated
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Refresh"))
	void CustomRefresh(FBuffApplyEvent const& ApplyEvent);
	//Called when the buff is removed
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Remove"))
	void CustomRemove(FBuffRemoveEvent const& RemoveEvent);
	//Called after removal to cleanup any refs or bindings
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Cleanup"))
	void CustomCleanup();
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