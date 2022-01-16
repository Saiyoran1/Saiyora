#include "ResourceBuffFunctions.h"
#include "Buff.h"
#include "ResourceHandler.h"
#include "SaiyoraCombatInterface.h"

#pragma region Resource Delta Modifier

void UResourceDeltaModifierFunction::ResourceDeltaModifier(UBuff* Buff, TSubclassOf<UResource> const ResourceClass,
                                                           FResourceDeltaModifier const& Modifier)
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

void UResourceDeltaModifierFunction::SetModifierVars(TSubclassOf<UResource> const ResourceClass,
                                              FResourceDeltaModifier const& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UResourceHandler* TargetHandler = ISaiyoraCombatInterface::Execute_GetResourceHandler(GetOwningBuff()->GetAppliedTo());
		if (IsValid(TargetHandler))
		{
			TargetResource = TargetHandler->FindActiveResource(ResourceClass);
			Mod = Modifier;
		}
	}
}

void UResourceDeltaModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetResource))
	{
		TargetResource->AddResourceDeltaModifier(GetOwningBuff(), Mod);
	}
}

void UResourceDeltaModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetResource))
	{
		TargetResource->RemoveResourceDeltaModifier(GetOwningBuff());
	}
}

#pragma endregion