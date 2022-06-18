#include "StatBuffFunctions.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"

#pragma region Stat Modifiers

void UStatModifierFunction::StatModifiers(UBuff* Buff, TMap<FGameplayTag, FCombatModifier> const& Modifiers)
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

void UStatModifierFunction::SetModifierVars(TMap<FGameplayTag, FCombatModifier> const& Modifiers)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwningBuff()->GetAppliedTo());
		for (TTuple<FGameplayTag, FCombatModifier> const& ModTuple : Modifiers)
		{
			if (ModTuple.Key.IsValid() && ModTuple.Key.MatchesTag(FSaiyoraCombatTags::Get().Stat) && !ModTuple.Key.MatchesTagExact(FSaiyoraCombatTags::Get().Stat) && ModTuple.Value.Type != EModifierType::Invalid)
			{
				StatMods.Add(ModTuple.Key, FCombatModifier(ModTuple.Value.Value, ModTuple.Value.Type, GetOwningBuff(), ModTuple.Value.bStackable));
			}
		}
	}
}

void UStatModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		for (TTuple<FGameplayTag, FCombatModifier> const& Mod : StatMods)
		{
			TargetHandler->AddStatModifier(Mod.Key, Mod.Value);
		}
	}
}

void UStatModifierFunction::OnStack(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		for (TTuple<FGameplayTag, FCombatModifier> const& Mod : StatMods)
		{
			if (Mod.Value.bStackable)
			{
				TargetHandler->AddStatModifier(Mod.Key, Mod.Value);
			}
		}
	}
}

void UStatModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		for (TTuple<FGameplayTag, FCombatModifier> const& Mod : StatMods)
		{
			TargetHandler->RemoveStatModifier(Mod.Key, GetOwningBuff());
		}
	}
}

#pragma endregion