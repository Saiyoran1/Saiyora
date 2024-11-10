#include "BuffFunctionality.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"

#pragma region UBuffFunction

UWorld* UBuffFunction::GetWorld() const
{
    return nullptr;
}

UBuffFunction* UBuffFunction::InstantiateBuffFunction(UBuff* Buff, const TSubclassOf<UBuffFunction> FunctionClass)
{
    return nullptr;
}

void UBuffFunction::InitializeBuffFunction(UBuff* BuffRef)
{
   
}

#pragma endregion
#pragma region UBuffRestrictionFunction

void UBuffRestrictionFunction::BuffRestriction(UBuff* Buff, const ECombatEventDirection RestrictionDirection, const FBuffRestriction& Restriction)
{
    if (!IsValid(Buff) || !Restriction.IsBound() || Buff->GetAppliedTo()->GetLocalRole() != ROLE_Authority)
    {
        return;
    }
    UBuffRestrictionFunction* NewBuffRestrictionFunction = Cast<UBuffRestrictionFunction>(InstantiateBuffFunction(Buff, StaticClass()));
    if (!IsValid(NewBuffRestrictionFunction))
    {
        return;
    }
    NewBuffRestrictionFunction->SetRestrictionVars(RestrictionDirection, Restriction);
}

void UBuffRestrictionFunction::SetRestrictionVars(const ECombatEventDirection RestrictionDirection, const FBuffRestriction& Restriction)
{
    if (GetOwningBuff()->GetAppliedTo()->Implements<USaiyoraCombatInterface>())
    {
        TargetComponent = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwningBuff()->GetAppliedTo());
        Direction = RestrictionDirection;
        Restrict = Restriction;
    }
}

void UBuffRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
    if (IsValid(TargetComponent))
    {
        switch (Direction)
        {
            case ECombatEventDirection::Outgoing :
                TargetComponent->AddOutgoingBuffRestriction(Restrict);
                break;
            case ECombatEventDirection::Incoming :
                TargetComponent->AddIncomingBuffRestriction(Restrict);
                break;
            default :
                break;
        }
    }
}

void UBuffRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
    if (IsValid(TargetComponent))
    {
        switch (Direction)
        {
        case ECombatEventDirection::Outgoing :
            TargetComponent->RemoveOutgoingBuffRestriction(Restrict);
            break;
        case ECombatEventDirection::Incoming :
            TargetComponent->RemoveIncomingBuffRestriction(Restrict);
            break;
        default :
            break;
        }
    }
}

#pragma endregion 