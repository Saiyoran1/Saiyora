#include "StatBuffFunctions.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"

#pragma region Stat Modifiers

void UStatModifierFunction::StatModifiers(UBuff* Buff, const TMap<FGameplayTag, FCombatModifier>& Modifiers)
{
	if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UStatModifierFunction* NewStatModFunction = Cast<UStatModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewStatModFunction))
	{
		return;
	}
	NewStatModFunction->SetModifierVars(Modifiers);
}

void UStatModifierFunction::SetModifierVars(const TMap<FGameplayTag, FCombatModifier>& Modifiers)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwningBuff()->GetAppliedTo());
		for (const TTuple<FGameplayTag, FCombatModifier>& ModTuple : Modifiers)
		{
			if (ModTuple.Key.IsValid() && ModTuple.Key.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !ModTuple.Key.MatchesTagExact(FSaiyoraCombatTags::Get().Stat) && ModTuple.Value.Type != EModifierType::Invalid)
			{
				StatMods.Add(ModTuple.Key, FCombatModifier(ModTuple.Value.Value, ModTuple.Value.Type, GetOwningBuff(), ModTuple.Value.bStackable));
			}
		}
	}
}

void UStatModifierFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		for (const TTuple<FGameplayTag, FCombatModifier>& Mod : StatMods)
		{
			TargetHandler->AddStatModifier(Mod.Key, Mod.Value);
		}
	}
}

void UStatModifierFunction::OnStack(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		for (const TTuple<FGameplayTag, FCombatModifier>& Mod : StatMods)
		{
			if (Mod.Value.bStackable)
			{
				TargetHandler->AddStatModifier(Mod.Key, Mod.Value);
			}
		}
	}
}

void UStatModifierFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		for (const TTuple<FGameplayTag, FCombatModifier>& Mod : StatMods)
		{
			TargetHandler->RemoveStatModifier(Mod.Key, GetOwningBuff());
		}
	}
}

#pragma endregion