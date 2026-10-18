#pragma once
#include "BuffStructs.h"
#include "BuffFunctionality.generated.h"

class UCombatAbility;
class UBuffHandler;

#pragma region UBuffFunction

//Base class for objects that represent some functionality a buff wants to perform that can be reused by multiple buffs.
UCLASS(Abstract, EditInlineNew, CollapseCategories)
class SAIYORAV4_API UBuffFunction : public UObject
{
	GENERATED_BODY()

public:

	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintPure)
	UBuff* GetOwningBuff() const { return OwningBuff; }
	
	//Set up refs and establish a connection to the owning buff, before OnApply.
	void InitializeBuffFunction(UBuff* BuffRef);
	
	//Pure virtual function called during initial application for child classes to use
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) {}
	//Pure virtual function called any time the buff's stacks or duration are updated for child classes to use
	virtual void OnChange(const FBuffApplyEvent& ApplyEvent) {}
	//Pure virtual function called when the buff is removed for child classes to use
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) {}
	//Virtual function for child classes to determine specific restrictions on buff application.
	//Useful for things like "don't apply a DoT to a target that can't take damage."
	virtual bool CheckBuffFunctionRestricted(const FBuffApplyEvent& ApplyEvent, EBuffApplyFailReason& OutFailReason) const { return false; }

protected:

	//Sets a buff function to automatically show the DelegateFunctionName property in editor,
	//and populate the list with functions from the parent buff that fit the signature of the provided delegate property.
	void SetDelegateProperty(const FName PropertyName);

private:

	UPROPERTY()
	UBuff* OwningBuff = nullptr;

	//Many buff functions use some kind of bound function (modifiers, restrictions). This lets you choose the appropriate function in the editor.
	//These function names are functions from the owning buff that match the signature the buff function subclass requires.
	//Ex. For a buff function that adds a Health Event Modifier (damage buff, healing nerf, etc), these would be all the functions of the owning buff that match the signature of FHealthEventModCondition.
	UPROPERTY(EditAnywhere, meta = (GetOptions = "GetDelegateFunctionNames", EditCondition = "bBindToDelegate", EditConditionHides))
	FName DelegateFunctionName = NAME_None;

	//Determines whether this buff function is going to bind a delegate callback at setup time.
	//Shows the DelegateFunctionName field and populates its options with functions from the parent buff that match the appropriate signature.
	UPROPERTY()
	bool bBindToDelegate = false;
	//The name of the delegate variable (FBuffRestriction, FHealthEventModCondition, etc.) that we want to use for this buff function.
	FName DelegatePropertyName = NAME_None;
	//Gets the actual property from DelegatePropertyName.
	const FDelegateProperty* GetDelegateProperty() const;
	//Options function that populates the dropdown for DelegateFunctionName.
	UFUNCTION()
	TArray<FName> GetDelegateFunctionNames() const;
};

#pragma endregion
#pragma region UBuffRestrictionFunction

//Buff function for adding a conditional restriction to incoming or outgoing buffs
UCLASS()
class UBuffRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

public:

	UBuffRestrictionFunction();
	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

private:

	//Whether this restriction is on incoming or outgoing buffs
	UPROPERTY(EditAnywhere)
	ECombatEventDirection Direction = ECombatEventDirection::Incoming;

	UPROPERTY(Transient)
	FBuffRestriction Restriction;
	UPROPERTY()
	UBuffHandler* TargetComponent = nullptr;
};

#pragma endregion