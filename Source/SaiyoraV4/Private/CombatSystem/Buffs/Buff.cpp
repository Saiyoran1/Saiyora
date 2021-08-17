// Fill out your copyright notice in the Description page of Project Settings.

#include "Buff.h"
#include "BuffFunction.h"
#include "UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BuffHandler.h"
#include "SaiyoraBuffLibrary.h"
#include "SaiyoraCombatInterface.h"
#include "GameFramework/GameStateBase.h"

const float UBuff::MinimumBuffDuration = 0.1f;

void UBuff::GetBuffTags(FGameplayTagContainer& OutContainer) const
{
    OutContainer.AppendTags(BuffTags);
}

void UBuff::InitializeBuff(FBuffApplyEvent& ApplicationEvent, UBuffHandler* NewHandler)
{
    GameStateRef = GetWorld()->GetGameState();
    if (!IsValid(GameStateRef))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Game State Ref in New Buff!"));
    }
    if (!IsValid(NewHandler))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invaid BuffHandler ref in New Buff!"));
    }
    Handler = NewHandler;
    
    SetInitialStacks(ApplicationEvent.StackOverrideType, ApplicationEvent.OverrideStacks);
    SetInitialDuration(ApplicationEvent.RefreshOverrideType, ApplicationEvent.OverrideDuration);
    
    ApplicationEvent.Result.ActionTaken = EBuffApplyAction::NewBuff;
    ApplicationEvent.Result.AffectedBuff = this;
    ApplicationEvent.Result.PreviousStacks = 0;
    ApplicationEvent.Result.PreviousDuration = 0.0f;
    ApplicationEvent.Result.NewStacks = CurrentStacks;
    ApplicationEvent.Result.NewDuration = GetRemainingTime();
    ApplicationEvent.Result.NewApplyTime = GameStateRef->GetServerWorldTimeSeconds();

    CreationEvent = ApplicationEvent;
    Status = EBuffStatus::Active;
    
    BuffFunctionality();
    
    for (UBuffFunction* Function : Functions)
    {
        Function->OnApply(ApplicationEvent);
    }
}

void UBuff::ApplyEvent(FBuffApplyEvent& ApplicationEvent)
{
    ApplicationEvent.Result.AffectedBuff = this;
    ApplicationEvent.Result.PreviousDuration = GetRemainingTime();
    ApplicationEvent.Result.PreviousStacks = GetCurrentStacks();
    
    bool const Stacked =  StackBuff(ApplicationEvent.StackOverrideType, ApplicationEvent.OverrideStacks);
    bool const Refreshed = RefreshBuff(ApplicationEvent.RefreshOverrideType, ApplicationEvent.OverrideDuration);
    ApplicationEvent.Result.ActionTaken = Stacked ? (Refreshed ? EBuffApplyAction::StackedAndRefreshed : EBuffApplyAction::Stacked) 
    : (Refreshed ? EBuffApplyAction::Refreshed : EBuffApplyAction::Failed);
    
    ApplicationEvent.Result.NewDuration = GetRemainingTime();
    ApplicationEvent.Result.NewStacks = GetCurrentStacks();
    ApplicationEvent.Result.NewApplyTime = GameStateRef->GetServerWorldTimeSeconds();

    switch (ApplicationEvent.Result.ActionTaken)
    {
        case EBuffApplyAction::Failed:
            break;
        case EBuffApplyAction::Stacked:
            NotifyFunctionsOfStack(ApplicationEvent);
            OnStacked.Broadcast();
            break;
        case EBuffApplyAction::Refreshed:
            NotifyFunctionsOfRefresh(ApplicationEvent);
            break;
        case EBuffApplyAction::StackedAndRefreshed:
            NotifyFunctionsOfStack(ApplicationEvent);
            OnStacked.Broadcast();
            NotifyFunctionsOfRefresh(ApplicationEvent);
            break;
        default:
            break;
    }

    if (ApplicationEvent.Result.ActionTaken != EBuffApplyAction::Failed)
    {
        LastApplyEvent = ApplicationEvent;
        OnUpdated.Broadcast(ApplicationEvent);
    }
}

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

#pragma region StackingBuff

//Stacking

void UBuff::SetInitialStacks(EBuffApplicationOverrideType const OverrideType, int32 const OverrideStacks)
{
    int32 Stacks;
    switch (OverrideType)
    {
        case EBuffApplicationOverrideType::None :
            Stacks = DefaultInitialStacks;
            break;
        case EBuffApplicationOverrideType::Normal :
            Stacks = OverrideStacks;
            break;
        case EBuffApplicationOverrideType::Additive :
            Stacks = DefaultInitialStacks + OverrideStacks;
            break;
        default :
            Stacks = DefaultInitialStacks;
            break;
    }
    CurrentStacks = FMath::Clamp(Stacks, 1, MaximumStacks);
}

bool UBuff::StackBuff(EBuffApplicationOverrideType const OverrideType, int32 const OverrideStacks)
{
    if (!bStackable || CurrentStacks == MaximumStacks)
    {
        return false;
    }
    switch (OverrideType)
    {
        case EBuffApplicationOverrideType::None:
            UpdateStacksNoOverride();
            break;
        case EBuffApplicationOverrideType::Normal:
            UpdateStacksWithOverride(OverrideStacks);
            break;
        case EBuffApplicationOverrideType::Additive:
            UpdateStacksWithAdditiveOverride(OverrideStacks);
            break;
        default: return false;
    }
    return true;
}

void UBuff::UpdateStacksNoOverride()
{
    CurrentStacks = FMath::Clamp(CurrentStacks + DefaultStacksPerApply, 1, MaximumStacks);
}

void UBuff::UpdateStacksWithOverride(int32 const OverrideStacks)
{
    CurrentStacks = FMath::Clamp(OverrideStacks, 1, MaximumStacks);
}

void UBuff::UpdateStacksWithAdditiveOverride(int32 const OverrideStacks)
{
    CurrentStacks = FMath::Clamp(CurrentStacks + OverrideStacks, 1, MaximumStacks);
}

void UBuff::NotifyFunctionsOfStack(FBuffApplyEvent const & ApplyEvent)
{
    for (UBuffFunction* Function : Functions)
    {
        Function->OnStack(ApplyEvent);
    }
}

#pragma endregion

#pragma region RefreshingBuff

//Refreshing

void UBuff::ClearExpireTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(ExpireHandle);
}

void UBuff::ResetExpireTimer(float const TimerLength)
{
    ClearExpireTimer();
    GetWorld()->GetTimerManager().SetTimer(ExpireHandle, this, &UBuff::CompleteExpireTimer, TimerLength);
}

void UBuff::CompleteExpireTimer()
{
    ClearExpireTimer();
    USaiyoraBuffLibrary::RemoveBuff(this, EBuffExpireReason::TimedOut);
}

void UBuff::SetInitialDuration(EBuffApplicationOverrideType const OverrideType, float const OverrideDuration)
{
    LastApplyTime = GameStateRef->GetServerWorldTimeSeconds();
    
    if (!bFiniteDuration)
    {
        ExpireTime = 0.0f;
        ClearExpireTimer();
        return;
    }
    
    float Duration;
    
    switch (OverrideType)
    {
        case EBuffApplicationOverrideType::None :
            Duration = DefaultInitialDuration;
            break;
        case EBuffApplicationOverrideType::Normal :
            Duration = OverrideDuration;
            break;
        case EBuffApplicationOverrideType::Additive :
            Duration = DefaultInitialDuration + OverrideDuration;
            break;
        default :
            Duration = DefaultInitialDuration;
    }
    Duration = FMath::Clamp(Duration, MinimumBuffDuration, MaximumDuration);
    ExpireTime = LastApplyTime + Duration;
    ResetExpireTimer(Duration);
}

bool UBuff::RefreshBuff(EBuffApplicationOverrideType const OverrideType, float const OverrideDuration)
{
    if(!bRefreshable || GetRemainingTime() == MaximumDuration)
    {
        return false;
    }
    switch (OverrideType)
    {
    case EBuffApplicationOverrideType::None:
        UpdateDurationNoOverride();
        break;
    case EBuffApplicationOverrideType::Normal:
        UpdateDurationWithOverride(OverrideDuration);
        break;
    case EBuffApplicationOverrideType::Additive:
        UpdateDurationWithAdditiveOverride(OverrideDuration);
        break;
    default: 
        UpdateDurationNoOverride();
    }
    return true;
}

void UBuff::ExpireBuff(FBuffRemoveEvent const & RemoveEvent)
{
    if (bFiniteDuration)
    {
        ClearExpireTimer();
    }
    for (UBuffFunction* Function : Functions)
    {
        Function->OnRemove(RemoveEvent);
        Function->CleanupBuffFunction();
    }
    OnRemoved.Broadcast();
    Status = EBuffStatus::Removed;

    RemovingEvent = RemoveEvent;
}

void UBuff::RegisterBuffFunction(UBuffFunction* NewFunction)
{
    Functions.AddUnique(NewFunction);
}

void UBuff::UpdateDurationNoOverride()
{
    float const TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle);
    float const NewDuration = FMath::Clamp((TimeRemaining + DefaultDurationPerApply), MinimumBuffDuration, MaximumDuration);
    LastApplyTime = GameStateRef->GetServerWorldTimeSeconds();
    ExpireTime = LastApplyTime + NewDuration;
    ResetExpireTimer(NewDuration);
}

void UBuff::UpdateDurationWithOverride(float const OverrideDuration)
{
    float const NewDuration = FMath::Clamp(OverrideDuration, MinimumBuffDuration, MaximumDuration);
    LastApplyTime = GameStateRef->GetServerWorldTimeSeconds();
    ExpireTime = LastApplyTime + NewDuration;
    ResetExpireTimer(NewDuration);
}

void UBuff::UpdateDurationWithAdditiveOverride(float const OverrideDuration)
{
    float const TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle);
    float const NewDuration = FMath::Clamp((TimeRemaining + OverrideDuration), MinimumBuffDuration, MaximumDuration);
    LastApplyTime = GameStateRef->GetServerWorldTimeSeconds();
    ExpireTime = LastApplyTime + NewDuration;
    ResetExpireTimer(NewDuration);
}

void UBuff::NotifyFunctionsOfRefresh(FBuffApplyEvent const & ApplyEvent)
{
    for (UBuffFunction* Function : Functions)
    {
        Function->OnRefresh(ApplyEvent);
    }
}

#pragma endregion

#pragma region Replication

void UBuff::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UBuff, CreationEvent);
    DOREPLIFETIME(UBuff, LastApplyEvent);
    DOREPLIFETIME(UBuff, RemovingEvent);
}

void UBuff::OnRep_CreationEvent()
{
    if (Status != EBuffStatus::Spawning || CreationEvent.Result.ActionTaken != EBuffApplyAction::NewBuff)
    {
        //Replication of a creation event after initialization has already happened.
        //An invalid creation event.

        UE_LOG(LogTemp, Warning, TEXT("Creation Event replication gone wrong."))
        return;
    }
    
    UpdateClientStateOnApply(CreationEvent);
    Status = EBuffStatus::Active;

    BuffFunctionality();

    for (UBuffFunction* Function : Functions)
    {
        Function->OnApply(CreationEvent);
    }

    HandleBuffApplyEventReplication(CreationEvent);

    //Check if there is a valid LastApplyEvent that came in before or at the same time as CreationEvent (due to either variables coming in the wrong order, or net relevancy).
    if (LastApplyEvent.Result.ActionTaken != EBuffApplyAction::NewBuff && LastApplyEvent.Result.ActionTaken != EBuffApplyAction::Failed)
    {
        OnRep_LastApplyEvent();
    }
}

void UBuff::OnRep_LastApplyEvent()
{
    if (Status != EBuffStatus::Active || !LastApplyEvent.Result.AffectedBuff)
    {
        //Wait for CreationEvent replication to initialize the buff. This function will be called again if needed.
        return;
    }
    
    UpdateClientStateOnApply(LastApplyEvent);
    
    switch (LastApplyEvent.Result.ActionTaken)
    {
    case EBuffApplyAction::Failed:
        return;
    case EBuffApplyAction::Stacked:
        NotifyFunctionsOfStack(LastApplyEvent);
        OnStacked.Broadcast();
        break;
    case EBuffApplyAction::Refreshed:
        NotifyFunctionsOfRefresh(LastApplyEvent);
        break;
    case EBuffApplyAction::StackedAndRefreshed:
        NotifyFunctionsOfStack(LastApplyEvent);
        OnStacked.Broadcast();
        NotifyFunctionsOfRefresh(LastApplyEvent);
        break;
    default:
        return;
    }
    
    HandleBuffApplyEventReplication(LastApplyEvent);
}

void UBuff::OnRep_RemovingEvent()
{
    if (!RemovingEvent.Result || !RemovingEvent.RemovedBuff)
    {
        //Invalid remove event.
        return;
    }

    for (UBuffFunction* Function : Functions)
    {
        Function->OnRemove(RemovingEvent);
        Function->CleanupBuffFunction();
    }

    Status = EBuffStatus::Removed;
    OnRemoved.Broadcast();
    HandleBuffRemoveEventReplication(RemovingEvent);
}

void UBuff::UpdateClientStateOnApply(FBuffApplyEvent const& ReplicatedEvent)
{
    CurrentStacks = ReplicatedEvent.Result.NewStacks;
    //No actual expire timer on clients, just Apply and Expire time for UI display and BuffFunction access.
    LastApplyTime = ReplicatedEvent.Result.NewApplyTime;
    ExpireTime = ReplicatedEvent.Result.NewDuration + LastApplyTime;
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

UWorld* UBuff::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}

#pragma endregion 

