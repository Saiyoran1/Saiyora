#include "ResourceBuffFunctions.h"
#include "Buff.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Resource Delta Modifier

void UResourceDeltaModifierFunction::ResourceDeltaModifier(UBuff* Buff, const TSubclassOf<UResource> ResourceClass,
                                                           const FResourceDeltaModifier& Modifier)
{
	if (!IsValid(Buff) || !IsValid(ResourceClass) || !Modifier.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UResourceDeltaModifierFunction* NewResourceModifierFunction = Cast<UResourceDeltaModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewResourceModifierFunction))
	{
		return;
	}
	NewResourceModifierFunction->SetModifierVars(ResourceClass, Modifier);
}

void UResourceDeltaModifierFunction::SetModifierVars(const TSubclassOf<UResource> ResourceClass,
                                              const FResourceDeltaModifier& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		const UResourceHandler* TargetHandler = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwningBuff()->GetAppliedTo());
		if (IsValid(TargetHandler))
		{
			TargetResource = TargetHandler->FindActiveResource(ResourceClass);
			Mod = Modifier;
		}
	}
}

void UResourceDeltaModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetResource))
	{
		TargetResource->AddResourceDeltaModifier(Mod);
	}
}

void UResourceDeltaModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetResource))
	{
		TargetResource->RemoveResourceDeltaModifier(Mod);
	}
}

#pragma endregion