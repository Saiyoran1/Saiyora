#include "ResourceBuffFunctions.h"
#include "Buff.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

#pragma region Resource Delta Modifier

void UResourceDeltaModifierFunction::SetupBuffFunction()
{
	Super::SetupBuffFunction();

	const UResourceHandler* TargetHandler = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwningBuff()->GetAppliedTo());
	if (IsValid(TargetHandler))
	{
		TargetResource = TargetHandler->FindActiveResource(ResourceClass);
		Modifier.BindUFunction(GetOwningBuff(), ModifierFunctionName);
	}
}

void UResourceDeltaModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetResource) && Modifier.IsBound())
	{
		TargetResource->AddResourceDeltaModifier(Modifier);
	}
}

void UResourceDeltaModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetResource) && Modifier.IsBound())
	{
		TargetResource->RemoveResourceDeltaModifier(Modifier);
	}
}

TArray<FName> UResourceDeltaModifierFunction::GetResourceDeltaModifierFunctionNames() const
{
	return USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), FindFunction("ExampleModifierFunction"));
}

#pragma endregion
