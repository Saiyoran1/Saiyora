#pragma once
#include "BuffStructs.h"
#include "BuffFunctionality.generated.h"

class UCombatAbility;
class UBuffHandler;

#pragma region UBuffFunction

//Base class for objects that represent some functionality a buff wants to perform that can be reused by multiple buffs.
UCLASS(Abstract, EditInlineNew)
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
	//Pure virtual function called any time the buff's stacks or duration are updated for child classes to use
	virtual void OnChange(const FBuffApplyEvent& ApplyEvent) {}
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

	virtual void SetupBuffFunction() override;
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	//Whether this restriction is on incoming or outgoing buffs
	UPROPERTY(EditAnywhere, Category = "Buff Restriction")
	ECombatEventDirection Direction = ECombatEventDirection::Incoming;
	UPROPERTY(EditAnywhere, Category = "Buff Restriction", meta = (GetOptions = "GetBuffRestrictionFunctionNames"))
	FName RestrictionFunctionName;
	
	FBuffRestriction Restriction;
	UPROPERTY()
	UBuffHandler* TargetComponent = nullptr;

	UFUNCTION()
	TArray<FName> GetBuffRestrictionFunctionNames() const;
	UFUNCTION()
	bool ExampleRestrictionFunction(const FBuffApplyEvent& Event) const { return false; }
};

#pragma endregion