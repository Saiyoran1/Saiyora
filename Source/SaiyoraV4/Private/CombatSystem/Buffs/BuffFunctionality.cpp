#include "BuffFunctionality.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"

#pragma region UBuffFunction

UWorld* UBuffFunction::GetWorld() const
{
    return GetOwningBuff()->GetWorld();
}

void UBuffFunction::InitializeBuffFunction(UBuff* BuffRef)
{
    OwningBuff = BuffRef;
    if (bBindToDelegate)
    {
        if (const FDelegateProperty* DelegateProperty = GetDelegateProperty())
        {
            DelegateProperty->ContainerPtrToValuePtr<FScriptDelegate>(this)->BindUFunction(OwningBuff, DelegateFunctionName);
        }
    }
}

void UBuffFunction::SetDelegateProperty(const FName PropertyName)
{
    if (PropertyName.IsNone())
    {
        bBindToDelegate = false;
        return;
    }
    bBindToDelegate = true;
    DelegatePropertyName = PropertyName;
}

const FDelegateProperty* UBuffFunction::GetDelegateProperty() const
{
    return FindFProperty<FDelegateProperty>(GetClass(), DelegatePropertyName);
}

TArray<FName> UBuffFunction::GetDelegateFunctionNames() const
{
    TArray<FName> Names;
    Names.Add(NAME_None);
    if (!bBindToDelegate)
    {
        return Names;
    }
    const FDelegateProperty* CallbackProperty = GetDelegateProperty();
    if (!CallbackProperty)
    {
        return Names;
    }
    Names.Append(USaiyoraCombatLibrary::GetMatchingFunctionNames(GetOuter(), CallbackProperty->SignatureFunction));
    return Names;
}

#pragma endregion
#pragma region UBuffRestrictionFunction

UBuffRestrictionFunction::UBuffRestrictionFunction()
{
    SetDelegateProperty(GET_MEMBER_NAME_CHECKED(UBuffRestrictionFunction, Restriction));
}

void UBuffRestrictionFunction::OnApply(const FBuffApplyEvent& ApplyEvent)
{
    TargetComponent = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwningBuff()->GetAppliedTo());
    if (!IsValid(TargetComponent))
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
    if (!IsValid(TargetComponent))
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

#pragma endregion 
