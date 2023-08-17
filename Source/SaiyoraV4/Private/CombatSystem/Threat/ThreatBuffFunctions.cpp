#include "ThreatBuffFunctions.h"
#include "Buff.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

#pragma region Threat Modifier

void UThreatModifierFunction::ThreatModifier(UBuff* Buff, EThreatModifierType const ModifierType,
                                             FThreatModCondition const& Modifier)
{
	if (!IsValid(Buff) || ModifierType == EThreatModifierType::None || !Modifier.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UThreatModifierFunction* NewThreatModifierFunction = Cast<UThreatModifierFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewThreatModifierFunction))
	{
		return;
	}
	NewThreatModifierFunction->SetModifierVars(ModifierType, Modifier);
}

void UThreatModifierFunction::SetModifierVars(EThreatModifierType const ModifierType,
                                              FThreatModCondition const& Modifier)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
		ModType = ModifierType;
		Mod = Modifier;
	}
}

void UThreatModifierFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
		case EThreatModifierType::Incoming :
			TargetHandler->AddIncomingThreatModifier(Mod);
			break;
		case EThreatModifierType::Outgoing :
			TargetHandler->AddOutgoingThreatModifier(Mod);
			break;
		default :
			break;
		}
	}
}

void UThreatModifierFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (ModType)
		{
		case EThreatModifierType::Incoming :
			TargetHandler->RemoveIncomingThreatModifier(Mod);
			break;
		case EThreatModifierType::Outgoing :
			TargetHandler->RemoveOutgoingThreatModifier(Mod);
			break;
		default :
			break;
		}
	}
}

#pragma endregion
#pragma region Threat Restriction

void UThreatRestrictionFunction::ThreatRestriction(UBuff* Buff, EThreatModifierType const RestrictionType,
                                                   FThreatRestriction const& Restriction)
{
	if (!IsValid(Buff) || RestrictionType == EThreatModifierType::None || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UThreatRestrictionFunction* NewThreatRestrictionFunction = Cast<UThreatRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewThreatRestrictionFunction))
	{
		return;
	}
	NewThreatRestrictionFunction->SetRestrictionVars(RestrictionType, Restriction);
}

void UThreatRestrictionFunction::SetRestrictionVars(EThreatModifierType const RestrictionType,
                                                    FThreatRestriction const& Restriction)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
		RestrictType = RestrictionType;
		Restrict = Restriction;
	}
}

void UThreatRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{	
	if (IsValid(TargetHandler))
	{
		switch (RestrictType)
		{
		case EThreatModifierType::Incoming :
			TargetHandler->AddIncomingThreatRestriction(Restrict);
			break;
		case EThreatModifierType::Outgoing :
			TargetHandler->AddOutgoingThreatRestriction(Restrict);
			break;
		default :
			break;
		}
	}
}

void UThreatRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		switch (RestrictType)
		{
		case EThreatModifierType::Incoming :
			TargetHandler->RemoveIncomingThreatRestriction(Restrict);
			break;
		case EThreatModifierType::Outgoing :
			TargetHandler->RemoveOutgoingThreatRestriction(Restrict);
			break;
		default :
			break;
		}
	}
}

#pragma endregion
#pragma region Threat Actions

void UFadeFunction::Fade(UBuff* Buff)
{
	if (!IsValid(Buff) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UFadeFunction* NewFadeFunction = Cast<UFadeFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewFadeFunction))
	{
		return;
	}
	NewFadeFunction->SetFadeVars();
}

void UFadeFunction::SetFadeVars()
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
	}
}

void UFadeFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->AddFade(GetOwningBuff());
	}
}

void UFadeFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->RemoveFade(GetOwningBuff());
	}
}

void UFixateFunction::Fixate(UBuff* Buff, AActor* Target)
{
	if (!IsValid(Buff) || !IsValid(Target) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UFixateFunction* NewFixateFunction = Cast<UFixateFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewFixateFunction))
	{
		return;
	}
	NewFixateFunction->SetFixateVars(Target);
}

void UFixateFunction::SetFixateVars(AActor* Target)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
		FixateTarget = Target;
	}
}

void UFixateFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->AddFixate(FixateTarget, GetOwningBuff());
	}
}

void UFixateFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->RemoveFixate(FixateTarget, GetOwningBuff());
	}
}

void UBlindFunction::Blind(UBuff* Buff, AActor* Target)
{
	if (!IsValid(Buff) || !IsValid(Target) || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UBlindFunction* NewBlindFunction = Cast<UBlindFunction>(InstantiateBuffFunction(Buff, StaticClass()));
	if (!IsValid(NewBlindFunction))
	{
		return;
	}
	NewBlindFunction->SetBlindVars(Target);
}

void UBlindFunction::SetBlindVars(AActor* Target)
{
	if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		TargetHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwningBuff()->GetAppliedTo());
		BlindTarget = Target;
	}
}

void UBlindFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->AddBlind(BlindTarget, GetOwningBuff());
	}
}

void UBlindFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
	if (IsValid(TargetHandler))
	{
		TargetHandler->RemoveBlind(BlindTarget, GetOwningBuff());
	}
}

#pragma endregion
