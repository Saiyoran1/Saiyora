// Fill out your copyright notice in the Description page of Project Settings.

#include "Buff.h"
#include "BuffFunction.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BuffHandler.h"
#include "SaiyoraBuffLibrary.h"
#include "SaiyoraCombatLibrary.h"

const float UBuff::MinimumBuffDuration = 0.1f;
const FGameplayTag UBuff::GenericBuffFunctionTag = FGameplayTag::RequestGameplayTag(FName(TEXT("BuffFunction")));

TMap<FGameplayTag, FGameplayTagContainer> UBuff::GetServerFunctionTags() const
{
    if (HasAnyFlags(RF_ClassDefaultObject) || !GetAppliedTo()->HasAuthority())
    {
        TMap<FGameplayTag, FGameplayTagContainer> DefaultTags;
        for (TSubclassOf<UBuffFunction> const& FunctionClass : ServerFunctionClasses)
        {
            if (FunctionClass)
            {
                for (TTuple<FGameplayTag, FGameplayTagContainer> const& FunctionPair : FunctionClass.GetDefaultObject()->GetBuffFunctionTags())
                {
                    if (FunctionPair.Key.MatchesTag(GenericBuffFunctionTag))
                    {
                        DefaultTags.FindOrAdd(FunctionPair.Key).AppendTags(FunctionPair.Value);
                    }       
                }
            }
        }
        return DefaultTags;
    }
    return FunctionTags;
}

TMap<FGameplayTag, FGameplayTagContainer> UBuff::GetClientFunctionTags() const
{
    if (HasAnyFlags(RF_ClassDefaultObject) || GetAppliedTo()->HasAuthority())
    {
        TMap<FGameplayTag, FGameplayTagContainer> DefaultTags;
        for (TSubclassOf<UBuffFunction> const& FunctionClass : ClientFunctionClasses)
        {
            if (FunctionClass)
            {
                for (TTuple<FGameplayTag, FGameplayTagContainer> const& FunctionPair : FunctionClass.GetDefaultObject()->GetBuffFunctionTags())
                {
                    if (FunctionPair.Key.MatchesTag(GenericBuffFunctionTag))
                    {
                        DefaultTags.FindOrAdd(FunctionPair.Key).AppendTags(FunctionPair.Value);
                    }       
                }
            }
        }
        return DefaultTags;
    }
    return FunctionTags;
}

void UBuff::InitializeBuff(FBuffApplyEvent& ApplicationEvent)
{  
    SetInitialStacks(ApplicationEvent.StackOverrideType, ApplicationEvent.OverrideStacks);
    SetInitialDuration(ApplicationEvent.RefreshOverrideType, ApplicationEvent.OverrideDuration);
    
    ApplicationEvent.Result.ActionTaken = EBuffApplyAction::NewBuff;
    ApplicationEvent.Result.AffectedBuff = this;
    ApplicationEvent.Result.PreviousStacks = 0;
    ApplicationEvent.Result.PreviousDuration = 0.0f;
    ApplicationEvent.Result.NewStacks = CurrentStacks;
    ApplicationEvent.Result.NewDuration = GetRemainingTime();
    ApplicationEvent.Result.NewApplyTime = USaiyoraCombatLibrary::GetWorldTime(this);

    CreationEvent = ApplicationEvent;

    for (TSubclassOf<UBuffFunction> const& FunctionClass : ServerFunctionClasses)
    {
        if (FunctionClass)
        {
            UBuffFunction* NewFunction = Functions.Add_GetRef(NewObject<UBuffFunction>(this, FunctionClass));
            for (TTuple<FGameplayTag, FGameplayTagContainer> const& FunctionPair : NewFunction->GetBuffFunctionTags())
            {
                if (FunctionPair.Key.MatchesTag(GenericBuffFunctionTag))
                {
                    FunctionTags.FindOrAdd(FunctionPair.Key).AppendTags(FunctionPair.Value);
                }
            }
            NewFunction->SetupBuffFunction(this);
        }   
    }

    Status = EBuffStatus::Active;
    
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
    ApplicationEvent.Result.NewApplyTime = USaiyoraCombatLibrary::GetWorldTime(this);

    switch (ApplicationEvent.Result.ActionTaken)
    {
        case EBuffApplyAction::Failed:
            break;
        case EBuffApplyAction::Stacked:
            NotifyFunctionsOfStack(ApplicationEvent);
            break;
        case EBuffApplyAction::Refreshed:
            NotifyFunctionsOfRefresh(ApplicationEvent);
            break;
        case EBuffApplyAction::StackedAndRefreshed:
            NotifyFunctionsOfStack(ApplicationEvent);
            NotifyFunctionsOfRefresh(ApplicationEvent);
            break;
        default:
            break;
    }

    if (ApplicationEvent.Result.ActionTaken != EBuffApplyAction::Failed)
    {
        LastApplyEvent = ApplicationEvent;
    }
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
    LastApplyTime = USaiyoraCombatLibrary::GetWorldTime(this);
    
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
    }

    Status = EBuffStatus::Removed;

    RemovingEvent = RemoveEvent;
}

void UBuff::UpdateDurationNoOverride()
{
    float const TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle);
    float const NewDuration = FMath::Clamp((TimeRemaining + DefaultDurationPerApply), MinimumBuffDuration, MaximumDuration);
    LastApplyTime = USaiyoraCombatLibrary::GetWorldTime(this);
    ExpireTime = LastApplyTime + NewDuration;
    ResetExpireTimer(NewDuration);
}

void UBuff::UpdateDurationWithOverride(float const OverrideDuration)
{
    float const NewDuration = FMath::Clamp(OverrideDuration, MinimumBuffDuration, MaximumDuration);
    LastApplyTime = USaiyoraCombatLibrary::GetWorldTime(this);
    ExpireTime = LastApplyTime + NewDuration;
    ResetExpireTimer(NewDuration);
}

void UBuff::UpdateDurationWithAdditiveOverride(float const OverrideDuration)
{
    float const TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle);
    float const NewDuration = FMath::Clamp((TimeRemaining + OverrideDuration), MinimumBuffDuration, MaximumDuration);
    LastApplyTime = USaiyoraCombatLibrary::GetWorldTime(this);
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

    for (TSubclassOf<UBuffFunction> const& FunctionClass : ClientFunctionClasses)
    {
        if (FunctionClass)
        {
            UBuffFunction* NewFunction = Functions.Add_GetRef(NewObject<UBuffFunction>(this, FunctionClass));
            for (TTuple<FGameplayTag, FGameplayTagContainer> const& FunctionPair : NewFunction->GetBuffFunctionTags())
            {
                if (FunctionPair.Key.MatchesTag(GenericBuffFunctionTag))
                {
                    FunctionTags.FindOrAdd(FunctionPair.Key).AppendTags(FunctionPair.Value);
                }
            }
            NewFunction->SetupBuffFunction(this);
        }   
    }
    
    Status = EBuffStatus::Active;

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
        break;
    case EBuffApplyAction::Refreshed:
        NotifyFunctionsOfRefresh(LastApplyEvent);
        break;
    case EBuffApplyAction::StackedAndRefreshed:
        NotifyFunctionsOfStack(LastApplyEvent);
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
    }

    Status = EBuffStatus::Removed;

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
    UBuffHandler* BuffContainer = ReplicatedEvent.AppliedTo->FindComponentByClass<UBuffHandler>();
    if (!BuffContainer)
    {
        return;
    }
    BuffContainer->NotifyOfReplicatedIncomingBuffApply(ReplicatedEvent);

    UBuffHandler* BuffGenerator = ReplicatedEvent.AppliedBy->FindComponentByClass<UBuffHandler>();
    if (BuffGenerator)
    {
        BuffGenerator->NotifyOfReplicatedOutgoingBuffApply(ReplicatedEvent);
    }
}

void UBuff::HandleBuffRemoveEventReplication(FBuffRemoveEvent const& ReplicatedEvent)
{
    UBuffHandler* BuffContainer = ReplicatedEvent.RemovedFrom->FindComponentByClass<UBuffHandler>();
    if (!BuffContainer)
    {
        return;
    }
    BuffContainer->NotifyOfReplicatedIncomingBuffRemove(ReplicatedEvent);

    UBuffHandler* BuffGenerator = ReplicatedEvent.AppliedBy->FindComponentByClass<UBuffHandler>();
    if (BuffGenerator)
    {
        BuffGenerator->NotifyOfReplicatedOutgoingBuffRemove(ReplicatedEvent);
    }
}

#pragma endregion 

