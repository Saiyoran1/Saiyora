#include "BuffFunction.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"

#pragma region UBuffFunction

UWorld* UBuffFunction::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

UBuffFunction* UBuffFunction::InstantiateBuffFunction(UBuff* Buff, const TSubclassOf<UBuffFunction> FunctionClass)
{
    if (!IsValid(Buff) || !IsValid(FunctionClass))
    {
        return nullptr;
    }
    UBuffFunction* NewFunction = NewObject<UBuffFunction>(Buff, FunctionClass);
    NewFunction->InitializeBuffFunction(Buff);
    return NewFunction;
}

void UBuffFunction::InitializeBuffFunction(UBuff* BuffRef)
{
    if (!bBuffFunctionInitialized)
    {
        if (!IsValid(BuffRef))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid owning buff provided to buff function."));
            return;
        }
        OwningBuff = BuffRef;
        OwningBuff->RegisterBuffFunction(this);
        bBuffFunctionInitialized = true;
        SetupBuffFunction();
    }
}

#pragma endregion
#pragma region UCustomBuffFunction

void UCustomBuffFunction::SetupCustomBuffFunction(UBuff* Buff, const TSubclassOf<UCustomBuffFunction> FunctionClass)
{
    InstantiateBuffFunction(Buff, FunctionClass);
}

void UCustomBuffFunction::SetupCustomBuffFunctions(UBuff* Buff,
    const TArray<TSubclassOf<UCustomBuffFunction>>& FunctionClasses)
{
    for (const TSubclassOf<UCustomBuffFunction>& FunctionClass : FunctionClasses)
    {
        InstantiateBuffFunction(Buff, FunctionClass);
    }
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