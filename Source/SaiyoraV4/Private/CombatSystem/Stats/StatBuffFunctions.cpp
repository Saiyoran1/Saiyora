#include "StatBuffFunctions.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"

#pragma region Stat Modifiers

void UStatModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwningBuff()->GetAppliedTo());
	if (!IsValid(TargetHandler))
	{
		return;
	}
	for (TTuple<FGameplayTag, FCombatModifier>& Mod : StatMods)
	{
		Mod.Value.BuffSource = GetOwningBuff();
		StatModHandles.Add(Mod.Key, TargetHandler->AddStatModifier(Mod.Key, Mod.Value));
	}
}

void UStatModifierFunction::OnChange(const FBuffApplyEvent& ApplyEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	if (ApplyEvent.PreviousStacks != ApplyEvent.NewStacks)
	{
		for (const TTuple<FGameplayTag, FCombatModifier>& Mod : StatMods)
		{
			if (Mod.Value.bStackable)
			{
				TargetHandler->UpdateStatModifier(Mod.Key, StatModHandles.FindRef(Mod.Key), Mod.Value);
			}
		}
	}
}

void UStatModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (!IsValid(TargetHandler))
	{
		return;
	}
	for (const TTuple<FGameplayTag, FCombatModifierHandle>& ModHandle : StatModHandles)
	{
		TargetHandler->RemoveStatModifier(ModHandle.Key, ModHandle.Value);
	}
}

#pragma endregion