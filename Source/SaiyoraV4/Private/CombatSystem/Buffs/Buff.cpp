#include "Buff.h"
#include "BuffFunction.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"
#include "GameFramework/GameStateBase.h"

const float UBuff::MinimumBuffDuration = 0.1f;

#pragma region Initialization

UWorld* UBuff::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

void UBuff::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UBuff, CreationEvent);
    DOREPLIFETIME(UBuff, LastApplyEvent);
    DOREPLIFETIME(UBuff, RemovalReason);
}

void UBuff::InitializeBuff(FBuffApplyEvent& Event, UBuffHandler* NewHandler, EBuffApplicationOverrideType const StackOverrideType,
        int32 const OverrideStacks, EBuffApplicationOverrideType const RefreshOverrideType, float const OverrideDuration)
{
    if (Status != EBuffStatus::Spawning || !IsValid(NewHandler))
    {
        return;
    }
    GameStateRef = GetWorld()->GetGameState();
    Handler = NewHandler;
    Event.ActionTaken = EBuffApplyAction::NewBuff;
    Event.PreviousStacks = 0;
    switch (StackOverrideType)
    {
    case EBuffApplicationOverrideType::None :
        CurrentStacks = FMath::Clamp(DefaultInitialStacks, 1, MaximumStacks);
        break;
    case EBuffApplicationOverrideType::Normal :
        CurrentStacks = FMath::Clamp(OverrideStacks, 1, MaximumStacks);
        break;
    case EBuffApplicationOverrideType::Additive :
        CurrentStacks = FMath::Clamp(DefaultInitialStacks + OverrideStacks, 1, MaximumStacks);
        break;
    default :
        CurrentStacks = FMath::Clamp(DefaultInitialStacks, 1, MaximumStacks);
        break;
    }
    Event.NewStacks = CurrentStacks;
    LastApplyTime = GameStateRef->GetServerWorldTimeSeconds();
    Event.NewApplyTime = LastApplyTime;
    Event.PreviousDuration = 0.0f;
    if (bFiniteDuration)
    {
        switch (RefreshOverrideType)
        {
        case EBuffApplicationOverrideType::None :
            Event.NewDuration = FMath::Clamp(DefaultInitialDuration, MinimumBuffDuration, MaximumDuration);
            break;
        case EBuffApplicationOverrideType::Normal :
            Event.NewDuration = FMath::Clamp(OverrideDuration, MinimumBuffDuration, MaximumDuration);
            break;
        case EBuffApplicationOverrideType::Additive :
            Event.NewDuration = FMath::Clamp(DefaultInitialDuration + OverrideDuration, MinimumBuffDuration, MaximumDuration);
            break;
        default :
            Event.NewDuration = FMath::Clamp(DefaultInitialDuration, MinimumBuffDuration, MaximumDuration);
            break;
        }
        ExpireTime = LastApplyTime + Event.NewDuration;
        ResetExpireTimer();
    }
    else
    {
        Event.NewDuration = 0.0f;
        ExpireTime = 0.0f;
    }
    CreationEvent = Event;
    SetupCommonBuffFunctions();
    Status = EBuffStatus::Active;
    Handler->NotifyOfNewIncomingBuff(CreationEvent);
    for (UBuffFunction* Function : BuffFunctions)
    {
        if (IsValid(Function))
        {
            Function->OnApply(CreationEvent);
        }
    }
    OnApply(CreationEvent);
}

void UBuff::OnRep_CreationEvent()
{
    if (Status != EBuffStatus::Spawning || CreationEvent.ActionTaken != EBuffApplyAction::NewBuff)
    {
        //The check for new buff is more to guard against replicating a default CreationEvent than anything else.
        return;
    }
    GameStateRef = GetWorld()->GetGameState();
    CurrentStacks = CreationEvent.NewStacks;
    LastApplyTime = CreationEvent.NewApplyTime;
    ExpireTime = CreationEvent.NewDuration + LastApplyTime;
    SetupCommonBuffFunctions();
    Status = EBuffStatus::Active;
    if (IsValid(CreationEvent.AppliedTo) && CreationEvent.AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        Handler = ISaiyoraCombatInterface::Execute_GetBuffHandler(CreationEvent.AppliedTo);
        if (IsValid(Handler))
        {
            Handler->NotifyOfNewIncomingBuff(CreationEvent);
        }
    }
    for (UBuffFunction* Function : BuffFunctions)
    {
        if (IsValid(Function))
        {
            Function->OnApply(CreationEvent);
        }
    }
    OnApply(CreationEvent);
    //Check if there is a valid LastApplyEvent that came in before or at the same time as CreationEvent (due to either variables coming in the wrong order, or net relevancy).
    if (LastApplyEvent.ActionTaken != EBuffApplyAction::NewBuff && LastApplyEvent.ActionTaken != EBuffApplyAction::Failed)
    {
        OnRep_LastApplyEvent();
    }
}

#pragma endregion 
#pragma region Application

void UBuff::ApplyEvent(FBuffApplyEvent& ApplicationEvent, EBuffApplicationOverrideType const StackOverrideType,
        int32 const OverrideStacks, EBuffApplicationOverrideType const RefreshOverrideType, float const OverrideDuration)
{
    ApplicationEvent.AffectedBuff = this;
    ApplicationEvent.PreviousStacks = GetCurrentStacks();
    bool bStacked = false;
    if (bStackable && CurrentStacks < MaximumStacks)
    {
        switch (StackOverrideType)
        {
            case EBuffApplicationOverrideType::None :
                CurrentStacks = FMath::Clamp(CurrentStacks + DefaultStacksPerApply, 1, MaximumStacks);
                break;
            case EBuffApplicationOverrideType::Normal :
                CurrentStacks = FMath::Clamp(OverrideStacks, 1, MaximumStacks);
                break;
            case EBuffApplicationOverrideType::Additive :
                CurrentStacks = FMath::Clamp(CurrentStacks + OverrideStacks, 1, MaximumStacks);
                break;
            default :
                CurrentStacks = FMath::Clamp(CurrentStacks + DefaultStacksPerApply, 1, MaximumStacks);
                break;
        }
        bStacked = true;
    }
    ApplicationEvent.NewStacks = CurrentStacks;
    ApplicationEvent.NewApplyTime = GameStateRef->GetServerWorldTimeSeconds();
    ApplicationEvent.PreviousDuration = GetRemainingTime();
    bool bRefreshed = false;
    if (bFiniteDuration && bRefreshable && GetRemainingTime() < MaximumDuration)
    {
        LastApplyTime = ApplicationEvent.NewApplyTime;
        switch (RefreshOverrideType)
        {
            case EBuffApplicationOverrideType::None :
                ExpireTime = LastApplyTime + FMath::Clamp(GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle) + DefaultDurationPerApply, MinimumBuffDuration, MaximumDuration);
                break;
            case EBuffApplicationOverrideType::Normal :
                ExpireTime = LastApplyTime + FMath::Clamp(OverrideDuration, MinimumBuffDuration, MaximumDuration);
                break;
            case EBuffApplicationOverrideType::Additive :
                ExpireTime = LastApplyTime + FMath::Clamp(GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle) + OverrideDuration, MinimumBuffDuration, MaximumDuration);
                break;
            default :
                ExpireTime = LastApplyTime + FMath::Clamp(GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle) + DefaultDurationPerApply, MinimumBuffDuration, MaximumDuration);
                break;
        }
        ResetExpireTimer();
        bRefreshed = true;
    }
    ApplicationEvent.NewDuration = GetRemainingTime();
    ApplicationEvent.ActionTaken = bStacked ? (bRefreshed ? EBuffApplyAction::StackedAndRefreshed : EBuffApplyAction::Stacked) 
        : (bRefreshed ? EBuffApplyAction::Refreshed : EBuffApplyAction::Failed);
    if (ApplicationEvent.ActionTaken == EBuffApplyAction::Failed)
    {
        return;
    }
    LastApplyEvent = ApplicationEvent;
    for (UBuffFunction* Function : BuffFunctions)
    {
        if (LastApplyEvent.ActionTaken == EBuffApplyAction::Stacked || LastApplyEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
        {
            Function->OnStack(LastApplyEvent);
        }
        if (LastApplyEvent.ActionTaken == EBuffApplyAction::Refreshed || LastApplyEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
        {
            Function->OnRefresh(LastApplyEvent);
        }
    }
    OnApply(LastApplyEvent);
    OnUpdated.Broadcast(ApplicationEvent);
}

void UBuff::OnRep_LastApplyEvent()
{
    if (Status != EBuffStatus::Active)
    {
        //Wait for CreationEvent replication to initialize the buff. This function will be called again if needed.
        return;
    }
    CurrentStacks = LastApplyEvent.NewStacks;
    LastApplyTime = LastApplyEvent.NewApplyTime;
    ExpireTime = LastApplyEvent.NewDuration + LastApplyTime;
    for (UBuffFunction* Function : BuffFunctions)
    {
        if (LastApplyEvent.ActionTaken == EBuffApplyAction::Stacked || LastApplyEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
        {
            Function->OnStack(LastApplyEvent);
        }
        if (LastApplyEvent.ActionTaken == EBuffApplyAction::Refreshed || LastApplyEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
        {
            Function->OnRefresh(LastApplyEvent);
        }
    }
    OnApply(LastApplyEvent);
    OnUpdated.Broadcast(LastApplyEvent);
}

#pragma endregion
#pragma region Expiration

void UBuff::ResetExpireTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(ExpireHandle);
    GetWorld()->GetTimerManager().SetTimer(ExpireHandle, this, &UBuff::CompleteExpireTimer, FMath::Max(MinimumBuffDuration, ExpireTime - LastApplyTime));
}

void UBuff::CompleteExpireTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(ExpireHandle);
    TerminateBuff(EBuffExpireReason::TimedOut);
}

FBuffRemoveEvent UBuff::TerminateBuff(EBuffExpireReason const TerminationReason)
{
    if (bFiniteDuration)
    {
        GetWorld()->GetTimerManager().ClearTimer(ExpireHandle);
    }
    RemovalReason = TerminationReason;
    FBuffRemoveEvent RemoveEvent;
    RemoveEvent.Result = true;
    RemoveEvent.RemovedBuff = this;
    RemoveEvent.RemovedFrom = CreationEvent.AppliedTo;
    RemoveEvent.AppliedBy = CreationEvent.AppliedBy;
    RemoveEvent.ExpireReason = RemovalReason;
    for (UBuffFunction* Function : BuffFunctions)
    {
        Function->OnRemove(RemoveEvent);
        Function->CleanupBuffFunction();
    }
    OnRemove(RemoveEvent);
    Status = EBuffStatus::Removed;
    Handler->NotifyOfIncomingBuffRemoval(RemoveEvent);
    OnRemoved.Broadcast(RemoveEvent);
    return RemoveEvent;
}

void UBuff::OnRep_RemovalReason()
{
    if (RemovalReason == EBuffExpireReason::Invalid)
    {
        //Guard against replicating the default RemovalReason.
        return;
    }
    TerminateBuff(RemovalReason);
}

#pragma endregion 
#pragma region Subscriptions

void UBuff::SubscribeToBuffUpdated(FBuffEventCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnUpdated.AddUnique(Callback);
    }
}

void UBuff::UnsubscribeFromBuffUpdated(FBuffEventCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnUpdated.Remove(Callback);
    }
}

void UBuff::SubscribeToBuffRemoved(FBuffRemoveCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnRemoved.AddUnique(Callback);
    }
}

void UBuff::UnsubscribeFromBuffRemoved(FBuffRemoveCallback const& Callback)
{
    if (Callback.IsBound())
    {
        OnRemoved.Remove(Callback);
    }
}

#pragma endregion