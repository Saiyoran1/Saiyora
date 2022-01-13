#include "BuffFunction.h"
#include "Buff.h"
#include "DamageHandler.h"
#include "PlaneComponent.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"

#pragma region Core Buff Function

UBuffFunction* UBuffFunction::InstantiateBuffFunction(UBuff* Buff, TSubclassOf<UBuffFunction> const FunctionClass)
{
    if (!IsValid(Buff) || !IsValid(FunctionClass))
    {
        return nullptr;
    }
    UBuffFunction* NewFunction = NewObject<UBuffFunction>(Buff, FunctionClass);
    NewFunction->InitializeBuffFunction(Buff);
    return NewFunction;
}

UWorld* UBuffFunction::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
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

void UCustomBuffFunction::SetupCustomBuffFunction(UBuff* Buff, TSubclassOf<UCustomBuffFunction> const FunctionClass)
{
    InstantiateBuffFunction(Buff, FunctionClass);
}

void UCustomBuffFunction::SetupCustomBuffFunctions(UBuff* Buff,
    TArray<TSubclassOf<UCustomBuffFunction>> const& FunctionClasses)
{
    for (TSubclassOf<UCustomBuffFunction> const& FunctionClass : FunctionClasses)
    {
        InstantiateBuffFunction(Buff, FunctionClass);
    }
}

#pragma endregion
#pragma region Buff Restriction

void UBuffRestrictionFunction::BuffRestriction(UBuff* Buff, EBuffRestrictionType const RestrictionType, FBuffRestriction const& Restriction)
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
    NewBuffRestrictionFunction->SetRestrictionVars(RestrictionType, Restriction);
}

void UBuffRestrictionFunction::SetRestrictionVars(EBuffRestrictionType const RestrictionType, FBuffRestriction const& Restriction)
{
    if (GetOwningBuff()->GetAppliedTo()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        TargetComponent = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwningBuff()->GetAppliedTo());
        RestrictType = RestrictionType;
        Restrict = Restriction;
    }
}

void UBuffRestrictionFunction::OnApply(FBuffApplyEvent const& ApplyEvent)
{
    if (IsValid(TargetComponent))
    {
        switch (RestrictType)
        {
            case EBuffRestrictionType::Outgoing :
                TargetComponent->AddOutgoingBuffRestriction(GetOwningBuff(), Restrict);
                break;
            case EBuffRestrictionType::Incoming :
                TargetComponent->AddIncomingBuffRestriction(GetOwningBuff(), Restrict);
                break;
            default :
                break;
        }
    }
}

void UBuffRestrictionFunction::OnRemove(FBuffRemoveEvent const& RemoveEvent)
{
    if (IsValid(TargetComponent))
    {
        switch (RestrictType)
        {
        case EBuffRestrictionType::Outgoing :
            TargetComponent->RemoveOutgoingBuffRestriction(GetOwningBuff());
            break;
        case EBuffRestrictionType::Incoming :
            TargetComponent->RemoveIncomingBuffRestriction(GetOwningBuff());
            break;
        default :
            break;
        }
    }
}

#pragma endregion 