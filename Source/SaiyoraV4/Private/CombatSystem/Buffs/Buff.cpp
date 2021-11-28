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
    DOREPLIFETIME(UBuff, RemovingEvent);
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
    AppliedBy = Event.AppliedBy;
    Source = Event.Source;
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
    //TODO: Creation Event refactoring?
    CreationEvent = Event;
    Status = EBuffStatus::Active;
    //TODO: Buff functionality refactoring.
    BuffFunctionality();
    
    for (UBuffFunction* Function : BuffFunctions)
    {
        Function->OnApply(Event);
    }
}

void UBuff::OnRep_CreationEvent()
{
    if (Status != EBuffStatus::Spawning || CreationEvent.ActionTaken != EBuffApplyAction::NewBuff)
    {
        //Replication of a creation event after initialization has already happened.
        //An invalid creation event.

        UE_LOG(LogTemp, Warning, TEXT("Creation Event replication gone wrong."))
        return;
    }
    
    UpdateClientStateOnApply(CreationEvent);
    Status = EBuffStatus::Active;

    BuffFunctionality();

    for (UBuffFunction* Function : BuffFunctions)
    {
        Function->OnApply(CreationEvent);
    }

    HandleBuffApplyEventReplication(CreationEvent);

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
    ApplicationEvent.PreviousDuration = GetRemainingTime();
    ApplicationEvent.PreviousStacks = GetCurrentStacks();
    ApplicationEvent.NewApplyTime = GameStateRef->GetServerWorldTimeSeconds();
    
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
    switch (ApplicationEvent.ActionTaken)
    {
        case EBuffApplyAction::Failed:
            break;
        case EBuffApplyAction::Stacked:
            for (UBuffFunction* Function : BuffFunctions)
            {
                if (IsValid(Function))
                {
                    Function->OnStack(ApplicationEvent);
                }
            }
            OnStacked.Broadcast();
            break;
        case EBuffApplyAction::Refreshed:
            for (UBuffFunction* Function : BuffFunctions)
            {
                if (IsValid(Function))
                {
                    Function->OnRefresh(ApplicationEvent);
                }
            }
            break;
        case EBuffApplyAction::StackedAndRefreshed:
            for (UBuffFunction* Function : BuffFunctions)
            {
                if (IsValid(Function))
                {
                    Function->OnStack(ApplicationEvent);
                    Function->OnRefresh(ApplicationEvent);
                }
            }
            OnStacked.Broadcast();
            break;
        default:
            break;
    }

    //TODO: Last apply event refactoring?
    if (ApplicationEvent.ActionTaken != EBuffApplyAction::Failed)
    {
        LastApplyEvent = ApplicationEvent;
        OnUpdated.Broadcast(ApplicationEvent);
    }
}

void UBuff::OnRep_LastApplyEvent()
{
    if (Status != EBuffStatus::Active || !IsValid(LastApplyEvent.AffectedBuff))
    {
        //Wait for CreationEvent replication to initialize the buff. This function will be called again if needed.
        return;
    }
    UpdateClientStateOnApply(LastApplyEvent);
    switch (LastApplyEvent.ActionTaken)
    {
    case EBuffApplyAction::Failed:
        break;
    case EBuffApplyAction::Stacked:
        for (UBuffFunction* Function : BuffFunctions)
        {
            if (IsValid(Function))
            {
                Function->OnStack(LastApplyEvent);
            }
        }
        OnStacked.Broadcast();
        break;
    case EBuffApplyAction::Refreshed:
        for (UBuffFunction* Function : BuffFunctions)
        {
            if (IsValid(Function))
            {
                Function->OnRefresh(LastApplyEvent);
            }
        }
        break;
    case EBuffApplyAction::StackedAndRefreshed:
        for (UBuffFunction* Function : BuffFunctions)
        {
            if (IsValid(Function))
            {
                Function->OnStack(LastApplyEvent);
                Function->OnRefresh(LastApplyEvent);
            }
        }
        OnStacked.Broadcast();
        break;
    default:
        break;
    }
    HandleBuffApplyEventReplication(LastApplyEvent);
}

void UBuff::UpdateClientStateOnApply(FBuffApplyEvent const& ReplicatedEvent)
{
    CurrentStacks = ReplicatedEvent.NewStacks;
    //No actual expire timer on clients, just Apply and Expire time for UI display and BuffFunction access.
    LastApplyTime = ReplicatedEvent.NewApplyTime;
    ExpireTime = ReplicatedEvent.NewDuration + LastApplyTime;
}

void UBuff::HandleBuffApplyEventReplication(FBuffApplyEvent const& ReplicatedEvent)
{
    if (!IsValid(ReplicatedEvent.AppliedTo) || !ReplicatedEvent.AppliedTo->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return;
    }
    UBuffHandler* BuffContainer = ISaiyoraCombatInterface::Execute_GetBuffHandler(ReplicatedEvent.AppliedTo);
    if (!IsValid(BuffContainer))
    {
        return;
    }
    Handler = BuffContainer;
    BuffContainer->NotifyOfReplicatedIncomingBuffApply(ReplicatedEvent);

    if (!IsValid(ReplicatedEvent.AppliedBy) || !ReplicatedEvent.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return;
    }
    UBuffHandler* BuffGenerator = ISaiyoraCombatInterface::Execute_GetBuffHandler(ReplicatedEvent.AppliedBy);
    if (IsValid(BuffGenerator))
    {
        BuffGenerator->NotifyOfReplicatedOutgoingBuffApply(ReplicatedEvent);
    }
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
    GetHandler()->RemoveBuff(this, EBuffExpireReason::TimedOut);
}

void UBuff::ExpireBuff(FBuffRemoveEvent const & RemoveEvent)
{
    if (bFiniteDuration)
    {
        GetWorld()->GetTimerManager().ClearTimer(ExpireHandle);
    }
    for (UBuffFunction* Function : BuffFunctions)
    {
        Function->OnRemove(RemoveEvent);
        Function->CleanupBuffFunction();
    }
    OnRemoved.Broadcast();
    Status = EBuffStatus::Removed;

    RemovingEvent = RemoveEvent;
}

void UBuff::OnRep_RemovingEvent()
{
    if (!RemovingEvent.Result || !RemovingEvent.RemovedBuff)
    {
        //Invalid remove event.
        return;
    }

    for (UBuffFunction* Function : BuffFunctions)
    {
        Function->OnRemove(RemovingEvent);
        Function->CleanupBuffFunction();
    }

    Status = EBuffStatus::Removed;
    OnRemoved.Broadcast();
    HandleBuffRemoveEventReplication(RemovingEvent);
}

void UBuff::HandleBuffRemoveEventReplication(FBuffRemoveEvent const& ReplicatedEvent)
{
    if (!IsValid(ReplicatedEvent.RemovedFrom) || !ReplicatedEvent.RemovedFrom->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return;
    }
    UBuffHandler* BuffContainer = ISaiyoraCombatInterface::Execute_GetBuffHandler(ReplicatedEvent.RemovedFrom);
    if (!IsValid(BuffContainer))
    {
        return;
    }
    BuffContainer->NotifyOfReplicatedIncomingBuffRemove(ReplicatedEvent);

    if (!IsValid(ReplicatedEvent.AppliedBy) || !ReplicatedEvent.AppliedBy->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
    {
        return;
    }
    UBuffHandler* BuffGenerator = ISaiyoraCombatInterface::Execute_GetBuffHandler(ReplicatedEvent.AppliedBy);
    if (IsValid(BuffGenerator))
    {
        BuffGenerator->NotifyOfReplicatedOutgoingBuffRemove(ReplicatedEvent);
    }
}

#pragma endregion 
#pragma region Subscriptions

void UBuff::SubscribeToBuffUpdated(FBuffEventCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnUpdated.AddUnique(Callback);
}

void UBuff::UnsubscribeFromBuffUpdated(FBuffEventCallback const& Callback)
{
    if (!Callback.IsBound())
    {
        return;
    }
    OnUpdated.Remove(Callback);
}

#pragma endregion