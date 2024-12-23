#include "StatBuffFunctions.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"

#pragma region Stat Modifiers

void UStatModifierFunction::SetupBuffFunction()
{
	TargetHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwningBuff()->GetAppliedTo());
	for (TTuple<FGameplayTag, FCombatModifier>& Mod : StatMods)
	{
		Mod.Value.BuffSource = GetOwningBuff();
	}
}

void UStatModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		for (const TTuple<FGameplayTag, FCombatModifier>& Mod : StatMods)
		{
			StatModHandles.Add(Mod.Key, TargetHandler->AddStatModifier(Mod.Key, Mod.Value));
		}
	}
}

void UStatModifierFunction::OnChange(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler) && ApplyEvent.PreviousStacks != ApplyEvent.NewStacks)
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
	if (IsValid(TargetHandler))
	{
		for (const TTuple<FGameplayTag, FCombatModifierHandle>& ModHandle : StatModHandles)
		{
			TargetHandler->RemoveStatModifier(ModHandle.Key, ModHandle.Value);
		}
	}
}

#pragma endregion