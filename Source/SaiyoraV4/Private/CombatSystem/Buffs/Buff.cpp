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

void UBuff::InitializeBuff(FBuffApplyEvent& Event, UBuffHandler* NewHandler, const bool bIgnoreRestrictions, const EBuffApplicationOverrideType StackOverrideType,
        const int32 OverrideStacks, const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration)
{
    if (Status != EBuffStatus::Spawning || !IsValid(NewHandler))
    {
        return;
    }
    GameStateRef = GetWorld()->GetGameState<AGameState>();
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
    LastRefreshTime = GameStateRef->GetServerWorldTimeSeconds();
    Event.NewApplyTime = LastRefreshTime;
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
        ExpireTime = LastRefreshTime + Event.NewDuration;
        ResetExpireTimer();
    }
    else
    {
        Event.NewDuration = 0.0f;
        ExpireTime = 0.0f;
    }
    CreationEvent = Event;
    bIgnoringRestrictions = bIgnoreRestrictions;
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
    GameStateRef = GetWorld()->GetGameState<AGameState>();
    CurrentStacks = CreationEvent.NewStacks;
    LastRefreshTime = CreationEvent.NewApplyTime;
    ExpireTime = CreationEvent.NewDuration + LastRefreshTime;
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

void UBuff::ApplyEvent(FBuffApplyEvent& ApplicationEvent, const EBuffApplicationOverrideType StackOverrideType,
        const int32 OverrideStacks, const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration)
{
    ApplicationEvent.AffectedBuff = this;
    ApplicationEvent.PreviousStacks = GetCurrentStacks();
    bool bStacked = false;
    if ((bStackable && CurrentStacks < MaximumStacks) || StackOverrideType != EBuffApplicationOverrideType::None)
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
    if (bFiniteDuration && ((bRefreshable && GetRemainingTime() < MaximumDuration) || RefreshOverrideType != EBuffApplicationOverrideType::None))
    {
        LastRefreshTime = ApplicationEvent.NewApplyTime;
        switch (RefreshOverrideType)
        {
            case EBuffApplicationOverrideType::None :
                ExpireTime = LastRefreshTime + FMath::Clamp(GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle) + DefaultDurationPerApply, MinimumBuffDuration, MaximumDuration);
                break;
            case EBuffApplicationOverrideType::Normal :
                ExpireTime = LastRefreshTime + FMath::Clamp(OverrideDuration, MinimumBuffDuration, MaximumDuration);
                break;
            case EBuffApplicationOverrideType::Additive :
                ExpireTime = LastRefreshTime + FMath::Clamp(GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle) + OverrideDuration, MinimumBuffDuration, MaximumDuration);
                break;
            default :
                ExpireTime = LastRefreshTime + FMath::Clamp(GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle) + DefaultDurationPerApply, MinimumBuffDuration, MaximumDuration);
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
        ApplicationEvent.FailReasons.AddUnique(EBuffApplyFailReason::NoStackRefreshOrDuplicate);
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
    if (LastApplyEvent.ActionTaken == EBuffApplyAction::Refreshed || LastApplyEvent.ActionTaken == EBuffApplyAction::StackedAndRefreshed)
    {
        LastRefreshTime = LastApplyEvent.NewApplyTime;
        ExpireTime = LastApplyEvent.NewDuration + LastRefreshTime;
    }
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
    GetWorld()->GetTimerManager().SetTimer(ExpireHandle, this, &UBuff::CompleteExpireTimer, FMath::Max(MinimumBuffDuration, ExpireTime - LastRefreshTime));
}

void UBuff::CompleteExpireTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(ExpireHandle);
    TerminateBuff(EBuffExpireReason::TimedOut);
}

FBuffRemoveEvent UBuff::TerminateBuff(const EBuffExpireReason TerminationReason)
{
    if (bIgnoringRestrictions && TerminationReason == EBuffExpireReason::Dispel)
    {
        return FBuffRemoveEvent();
    }
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