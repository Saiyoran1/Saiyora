#include "BuffFunctionality.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

#pragma region UBuffFunction

UWorld* UBuffFunction::GetWorld() const
{
    return GetOwningBuff()->GetWorld();
}

UBuffFunction* UBuffFunction::InstantiateBuffFunction(UBuff* Buff, const TSubclassOf<UBuffFunction> FunctionClass)
{
    return nullptr;
}

void UBuffFunction::InitializeBuffFunction(UBuff* BuffRef)
{
    OwningBuff = BuffRef;
    SetupBuffFunction();
}

#pragma endregion
#pragma region UBuffRestrictionFunction

void UBuffRestrictionFunction::SetupBuffFunction()
{
    TargetComponent = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwningBuff()->GetAppliedTo());
    Restriction.BindUFunction(GetOwningBuff(), RestrictionFunctionName);
}

void UBuffRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
    if (!IsValid(TargetComponent) || !Restriction.IsBound())
    {
        return;
    }
    switch (Direction)
    {
        case ECombatEventDirection::Outgoing :
            TargetComponent->AddOutgoingBuffRestriction(Restriction);
            break;
        case ECombatEventDirection::Incoming :
            TargetComponent->AddIncomingBuffRestriction(Restriction);
            break;
        default :
            break;
    }
}

void UBuffRestrictionFunction::OnRemove(const FBuffRemoveEvent& RemoveEvent)
{
    if (!IsValid(TargetComponent) || !Restriction.IsBound())
    {
        return;
    }
    switch (Direction)
    {
    case ECombatEventDirection::Outgoing :
        TargetComponent->RemoveOutgoingBuffRestriction(Restriction);
        break;
    case ECombatEventDirection::Incoming :
        TargetComponent->RemoveIncomingBuffRestriction(Restriction);
        break;
    default :
        break;
    }
}

TArray<FName> UBuffRestrictionFunction::GetBuffRestrictionFunctionNames() const
{
    return USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), FindFunction("ExampleRestrictionFunction"));
}

#pragma endregion 
