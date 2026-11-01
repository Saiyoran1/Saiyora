#include "CombatDebugOptions.h"
#include "AbilityStructs.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatAbility.h"
#include "EngineUtils.h"
#include "Hitbox.h"
#include "NPCStructs.h"
#include "NPCAbility.h"
#include "PredictableProjectile.h"
#include "RotationBehaviors.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraPlayerCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSaiyoraBuffs, Log, All);

#pragma region Console Commands
#if WITH_EDITOR

static FAutoConsoleCommandWithWorld KillSelf
(
	TEXT("Combat.KillSelf"),
	TEXT("Kills the local player instantly."),
	FConsoleCommandWithWorldDelegate::CreateLambda(
		[](UWorld* World)
		{
			if (!World)
			{
				return;
			}
			for (TActorIterator<ASaiyoraPlayerCharacter> It(World); It; ++It)
			{
				ASaiyoraPlayerCharacter* Player = *It;
				if (IsValid(Player) && Player->IsLocallyControlled())
				{
					Player->DEBUG_RequestKillSelf();
					break;
				}
			}
		})
	);

#endif
#pragma endregion 

void UCombatDebugOptions::LogAbilityEvent(const AActor* Actor, const FAbilityEvent& Event)
{
	FString LogString = FString::Printf(TEXT("%s: Ability %s cast result was %s. "),
		IsValid(Actor) ? *Actor->GetName() : TEXT("null"), IsValid(Event.Ability) ? *Event.Ability->GetName() : TEXT("null"), *UEnum::GetDisplayValueAsText(Event.ActionTaken).ToString());
	
	if (Event.FailReasons.Num() > 0)
	{
		LogString += FString::Printf(TEXT("Fail reasons: "));
		for (int i = 0; i < Event.FailReasons.Num(); i++)
		{
			LogString += UEnum::GetDisplayValueAsText(Event.FailReasons[i]).ToString();
			if (i < Event.FailReasons.Num() - 1)
			{
				LogString += TEXT(", ");
			}
			else
			{
				LogString += TEXT(". ");
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("%s"), *LogString);
}

void UCombatDebugOptions::DisplayTokenInfo(const TMap<TSubclassOf<UNPCAbility>, FNPCAbilityTokens>& Tokens)
{
	for (const TTuple<TSubclassOf<UNPCAbility>, FNPCAbilityTokens>& TokenInfo : Tokens)
	{
		const FString AbilityName = TokenInfo.Key->GetName();
		int Available = 0;
		int Reserved = 0;
		int InUse = 0;
		int OnCooldown = 0;
		for (const FNPCAbilityToken& Token : TokenInfo.Value.Tokens)
		{
			switch (Token.State)
			{
			case ENPCAbilityTokenState::Available :
				Available++;
				break;
			case ENPCAbilityTokenState::Reserved :
				Reserved++;
				break;
			case ENPCAbilityTokenState::InUse :
				InUse++;
				break;
			case ENPCAbilityTokenState::Cooldown :
				OnCooldown++;
				break;
			default :
				break;
			}
		}
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.0f, FColor::Cyan, FString::Printf(
			L"%s: Available: %i, Reserved: %i, In Use: %i, On Cooldown: %i", *AbilityName, Available, Reserved, InUse, OnCooldown));
	}
}

void UCombatDebugOptions::DisplayClaimedLocations(const TMap<AActor*, FVector>& Locations)
{
	for (const TTuple<AActor*, FVector>& Tuple : Locations)
	{
		if (IsValid(Tuple.Key))
		{
			DrawDebugSphere(Tuple.Key->GetWorld(), Tuple.Value + FVector::UpVector * 50.0f, 50.0f, 32, FColor::Cyan, false, -1);
			DrawDebugCircle(Tuple.Key->GetWorld(), Tuple.Value + FVector::UpVector * 50.0f, 200.0f, 32, FColor::Cyan, false, -1,
				0, 2, FVector(0, 1, 0),FVector(1, 0, 0), false);
			DrawDebugLine(Tuple.Key->GetWorld(), Tuple.Value, Tuple.Key->GetActorLocation() + FVector::UpVector * 50.0f, FColor::Cyan, false, -1);
		}
	}
}

void UCombatDebugOptions::DrawRotationBehavior(const AActor* Actor, const FRotationDebugInfo& DebugInfo)
{
	if (!IsValid(Actor))
	{
		return;
	}
	const FVector StartLoc = Actor->GetActorLocation() + FVector::UpVector * 50.0f;
	DrawDebugDirectionalArrow(Actor->GetWorld(), StartLoc, StartLoc + DebugInfo.OriginalRotation.Vector() * 100.0f, 100.0f, FColor::Red);
	if (DebugInfo.bClampingRotation)
	{
		DrawDebugDirectionalArrow(Actor->GetWorld(), StartLoc, StartLoc + DebugInfo.UnclampedRotation.Vector() * 100.0f, 100.0f, FColor::Yellow);
	}
	DrawDebugDirectionalArrow(Actor->GetWorld(), StartLoc, StartLoc + DebugInfo.FinalRotation.Vector() * 100.0f, 100.0f, FColor::Green);
}

void UCombatDebugOptions::DrawRewindHitbox(const UHitbox* Hitbox, const FTransform& PreviousTransform)
{
	if (!IsValid(Hitbox))
	{
		return;
	}
	DrawDebugBox(Hitbox->GetWorld(), Hitbox->GetComponentLocation(), Hitbox->GetScaledBoxExtent(), Hitbox->GetComponentQuat(), FColor::Blue, false, 5.0f, 0, 2);
	DrawDebugBox(Hitbox->GetWorld(), PreviousTransform.GetLocation(), Hitbox->GetUnscaledBoxExtent() * PreviousTransform.GetScale3D(), PreviousTransform.GetRotation(), FColor::Green, false, 5.0f, 0, 2);
}

void UCombatDebugOptions::DrawHiddenProjectile(const APredictableProjectile* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}
	DrawDebugSphere(Projectile->GetWorld(), Projectile->GetRootComponent()->Bounds.Origin, Projectile->GetRootComponent()->Bounds.SphereRadius, 32, FColor::Green);
}

#pragma region Buffs
#if !UE_BUILD_SHIPPING

void UCombatDebugOptions::DEBUG_LogBuffApplyEvent(const UBuffHandler* AppliedToHandler, const FBuffApplyEvent& ApplyEvent) const
{
	//Log the apply event itself.
	FString EventString;
	ApplyEvent.DEBUG_ToString(EventString);
	UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s BuffApplyEvent: %s"),
		USaiyoraCombatLibrary::BoolToServerOrClientString(AppliedToHandler->GetOwnerRole() == ROLE_Authority), *EventString);
	//Optionally dump the entire state of both involved buff handlers.
	if (BuffLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		//Log the state of the handler the buff was applied to.
		TArray<FString> IncomingDump;
		AppliedToHandler->DEBUG_DumpState(ECombatEventDirection::Incoming, IncomingDump);
		for (const FString& DumpString : IncomingDump)
		{
			UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
		}
		//If valid, log the state of the handler the buff was applied by.
		if (IsValid(ApplyEvent.AppliedBy) && ApplyEvent.AppliedBy->Implements<USaiyoraCombatInterface>())
		{
			const UBuffHandler* GeneratorBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(ApplyEvent.AppliedBy);
			if (IsValid(GeneratorBuff))
			{
				TArray<FString> OutgoingDump;
				GeneratorBuff->DEBUG_DumpState(ECombatEventDirection::Outgoing,OutgoingDump);
				for (const FString& DumpString : OutgoingDump)
				{
					UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
				}
			}
		}
	}
}

void UCombatDebugOptions::DEBUG_LogBuffApplyFailure(const UBuffHandler* AppliedToHandler, const FBuffApplyEvent& ApplyEvent) const
{
	//Log the apply event itself.
	FString EventString;
	ApplyEvent.DEBUG_ToString(EventString);
	UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s FAILED BuffApplyEvent: %s"),
		USaiyoraCombatLibrary::BoolToServerOrClientString(AppliedToHandler->GetOwnerRole() == ROLE_Authority), *EventString);
	//Optionally dump the entire state of both involved buff handlers.
	if (BuffLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		//Log the state of the handler the buff was attempting to be applied to.
		TArray<FString> IncomingDump;
		AppliedToHandler->DEBUG_DumpState(ECombatEventDirection::Incoming, IncomingDump);
		for (const FString& DumpString : IncomingDump)
		{
			UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
		}
		//If valid, log the state of the handler the buff was applied by.
		if (IsValid(ApplyEvent.AppliedBy) && ApplyEvent.AppliedBy->Implements<USaiyoraCombatInterface>())
		{
			const UBuffHandler* GeneratorBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(ApplyEvent.AppliedBy);
			if (IsValid(GeneratorBuff))
			{
				TArray<FString> OutgoingDump;
				GeneratorBuff->DEBUG_DumpState(ECombatEventDirection::Outgoing,OutgoingDump);
				for (const FString& DumpString : OutgoingDump)
				{
					UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
				}
			}
		}
	}
}

void UCombatDebugOptions::DEBUG_LogBuffRemoveEvent(const UBuffHandler* AppliedToHandler, const FBuffRemoveEvent& RemoveEvent) const
{
	//Log the remove event itself.
	FString EventString;
	RemoveEvent.DEBUG_ToString(EventString);
	UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s BuffRemoveEvent: %s"),
		USaiyoraCombatLibrary::BoolToServerOrClientString(AppliedToHandler->GetOwnerRole() == ROLE_Authority), *EventString);
	//Optionally dump the entire state of both involved buff handlers.
	if (BuffLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		//Log the state of the handler the buff was applied to.
		TArray<FString> IncomingDump;
		AppliedToHandler->DEBUG_DumpState(ECombatEventDirection::Incoming, IncomingDump);
		for (const FString& DumpString : IncomingDump)
		{
			UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
		}
		//If valid, log the state of the handler the buff was applied by.
		if (IsValid(RemoveEvent.AppliedBy) && RemoveEvent.AppliedBy->Implements<USaiyoraCombatInterface>())
		{
			const UBuffHandler* GeneratorBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(RemoveEvent.AppliedBy);
			if (IsValid(GeneratorBuff))
			{
				TArray<FString> OutgoingDump;
				GeneratorBuff->DEBUG_DumpState(ECombatEventDirection::Outgoing, OutgoingDump);
				for (const FString& DumpString : OutgoingDump)
				{
					UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
				}
			}
		}
	}
}

void UCombatDebugOptions::DEBUG_LogBuffRestrictionAdded(const UBuffHandler* AppliedToHandler, const ECombatEventDirection Direction, const FBuffRestriction& Restriction) const
{
	UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s Buff Restriction Added to handler for %s: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Restriction));
	//Optionally dump the whole restrictions list on the handler.
	if (BuffRestrictionLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpRestrictions(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogBuffRestrictionAddFailed(const UBuffHandler* AppliedToHandler, const ECombatEventDirection Direction,
	const FBuffRestriction& Restriction, const FString& FailReason) const
{
	UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s Buff Restriction FAILED to Add to handler for %s: %s, FailReason: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Restriction),
		*FailReason);
	//Optionally dump the whole restrictions list on the handler.
	if (BuffRestrictionLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpRestrictions(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogBuffRestrictionRemoved(const UBuffHandler* AppliedToHandler, const ECombatEventDirection Direction, const FBuffRestriction& Restriction) const
{
	UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s Buff Restriction Removed from handler for %s: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Restriction));
	//Optionally dump the whole restrictions list on the handler.
	if (BuffRestrictionLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpRestrictions(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogBuffRestrictionRemoveFailed(const UBuffHandler* AppliedToHandler, const ECombatEventDirection Direction,
	const FBuffRestriction& Restriction, const FString& FailReason) const
{
	UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s Buff Restriction FAILED to Remove from handler for %s: %s, FailReason: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Restriction),
		*FailReason);
	//Optionally dump the whole restrictions list on the handler.
	if (BuffRestrictionLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpRestrictions(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s"), *DumpString);
		}
	}
}

#endif
#pragma endregion 