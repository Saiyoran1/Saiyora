#include "CombatDebugOptions.h"
#include "AbilityStructs.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CombatAbility.h"
#include "CombatLogComponent.h"
#include "DamageHandler.h"
#include "EngineUtils.h"
#include "Hitbox.h"
#include "NPCStructs.h"
#include "NPCAbility.h"
#include "PredictableProjectile.h"
#include "RotationBehaviors.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraGameState.h"
#include "SaiyoraPlayerCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSaiyoraBuffs, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogSaiyoraHealth, Log, All);

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

#pragma region Macros
#if !UE_BUILD_SHIPPING

#pragma region Buffs

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

void UCombatDebugOptions::DEBUG_LogBuffRemoveFailure(const UBuffHandler* AppliedToHandler, const FBuffRemoveEvent& RemoveEvent) const
{
	//Log the remove event itself.
	FString EventString;
	RemoveEvent.DEBUG_ToString(EventString);
	UE_LOG(LogSaiyoraBuffs, Log, TEXT("%s FAILED BuffRemoveEvent: %s"),
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

#pragma endregion
#pragma region Health Events

void UCombatDebugOptions::DEBUG_LogHealthEvent(const UDamageHandler* AppliedToHandler, const FHealthEvent& Event) const
{
	//Log the event itself.
	FString EventString;
	Event.DEBUG_ToString(EventString);
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s HealthEvent: %s"),
		USaiyoraCombatLibrary::BoolToServerOrClientString(AppliedToHandler->GetOwnerRole() == ROLE_Authority), *EventString);
	//Optionally dump the entire state of both involved damage handlers.
	if (HealthEventLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		//Log the state of the handler the health event was applied to.
		TArray<FString> IncomingDump;
		AppliedToHandler->DEBUG_DumpState(ECombatEventDirection::Incoming, IncomingDump);
		for (const FString& DumpString : IncomingDump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
		//If valid, log the state of the handler the event was applied by.
		if (IsValid(Event.Info.AppliedBy) && Event.Info.AppliedBy->Implements<USaiyoraCombatInterface>())
		{
			const UDamageHandler* GeneratorHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Event.Info.AppliedBy);
			if (IsValid(GeneratorHandler))
			{
				TArray<FString> OutgoingDump;
				GeneratorHandler->DEBUG_DumpState(ECombatEventDirection::Outgoing,OutgoingDump);
				for (const FString& DumpString : OutgoingDump)
				{
					UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
				}
			}
		}
	}
}

void UCombatDebugOptions::DEBUG_LogHealthEventFailure(const UDamageHandler* AppliedToHandler, const FHealthEvent& Event) const
{
	//Log the event itself.
	FString EventString;
	Event.DEBUG_ToString(EventString);
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s FAILED HealthEvent: %s"),
		USaiyoraCombatLibrary::BoolToServerOrClientString(AppliedToHandler->GetOwnerRole() == ROLE_Authority), *EventString);
	//Optionally dump the entire state of both involved damage handlers.
	if (HealthEventLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		//Log the state of the handler the event was attempting to be applied to.
		TArray<FString> IncomingDump;
		AppliedToHandler->DEBUG_DumpState(ECombatEventDirection::Incoming, IncomingDump);
		for (const FString& DumpString : IncomingDump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
		//If valid, log the state of the handler the event was applied by.
		if (IsValid(Event.Info.AppliedBy) && Event.Info.AppliedBy->Implements<USaiyoraCombatInterface>())
		{
			const UDamageHandler* GeneratorHandler = ISaiyoraCombatInterface::Execute_GetDamageHandler(Event.Info.AppliedBy);
			if (IsValid(GeneratorHandler))
			{
				TArray<FString> OutgoingDump;
				GeneratorHandler->DEBUG_DumpState(ECombatEventDirection::Outgoing,OutgoingDump);
				for (const FString& DumpString : OutgoingDump)
				{
					UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
				}
			}
		}
	}
}

void UCombatDebugOptions::DEBUG_LogDeath(const UDamageHandler* Handler, const int EventID, const bool bWasPendingKillingBlow) const
{
	FString NameString = Handler->GetOwner()->GetName();
	const FCombatantID ID = UCombatLogComponent::GetCombatantID(Handler->GetOwner());
	if (const ASaiyoraGameState* GameState = Handler->GetWorld()->GetGameState<ASaiyoraGameState>())
	{
		FText CombatantName;
		if (GameState->GetCombatLog()->GetCombatantDisplayName(ID, CombatantName))
		{
			NameString = CombatantName.ToString();
		}
	}
	UE_LOG(LogSaiyoraHealth, Log, TEXT("Combatant \"%s\" DIED from health event %i%s"), *NameString, EventID, bWasPendingKillingBlow ? TEXT(" (was pending)") : TEXT(""));
}

void UCombatDebugOptions::DEBUG_LogRespawn(const UDamageHandler* Handler, const FPendingResurrection& Resurrection) const
{
	FString NameString = Handler->GetOwner()->GetName();
	const FCombatantID ID = UCombatLogComponent::GetCombatantID(Handler->GetOwner());
	if (const ASaiyoraGameState* GameState = Handler->GetWorld()->GetGameState<ASaiyoraGameState>())
	{
		FText CombatantName;
		if (GameState->GetCombatLog()->GetCombatantDisplayName(ID, CombatantName))
		{
			NameString = CombatantName.ToString();
		}
	}
	UE_LOG(LogSaiyoraHealth, Log, TEXT("Combatant \"%s\" RESPAWNED%s"), *NameString,
		Resurrection.ResID > -1 ? *FString::Printf(L" from resurrection %i (%s)", Resurrection.ResID, *GetNameSafe(Resurrection.BuffClass)) : TEXT(""));
}

void UCombatDebugOptions::DEBUG_LogHealthEventRestrictionAdded(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction) const
{
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s Health Event Restriction Added to handler for %s: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Restriction));
	//Optionally dump the whole restrictions list on the handler.
	if (HealthEventRestrictionLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpRestrictions(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogHealthEventRestrictionAddFailed(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction,
	const FString& FailReason) const
{
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s Health Event Restriction FAILED to Add to handler for %s: %s, FailReason: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Restriction),
		*FailReason);
	//Optionally dump the whole restrictions list on the handler.
	if (HealthEventRestrictionLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpRestrictions(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogHealthEventRestrictionRemoved(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction) const
{
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s Health Event Restriction Removed from handler for %s: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Restriction));
	//Optionally dump the whole restrictions list on the handler.
	if (HealthEventRestrictionLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpRestrictions(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogHealthEventRestrictionRemoveFailed(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventRestriction& Restriction,
	const FString& FailReason) const
{
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s Health Event Restriction FAILED to Remove from handler for %s: %s, FailReason: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Restriction),
		*FailReason);
	//Optionally dump the whole restrictions list on the handler.
	if (HealthEventRestrictionLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpRestrictions(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogHealthEventModifierAdded(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier) const
{
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s Health Event Modifier Added to handler for %s: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Modifier));
	//Optionally dump the whole modifier list on the handler.
	if (HealthEventModifierLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpModifiers(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogHealthEventModifierAddFailed(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier,
	const FString& FailReason) const
{
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s Health Event Modifier FAILED to Add to handler for %s: %s, FailReason: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Modifier),
		*FailReason);
	//Optionally dump the whole modifier list on the handler.
	if (HealthEventModifierLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpModifiers(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogHealthEventModifierRemoved(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier) const
{
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s Health Event Modifier Removed from handler for %s: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Modifier));
	//Optionally dump the whole modifier list on the handler.
	if (HealthEventModifierLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpModifiers(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
	}
}

void UCombatDebugOptions::DEBUG_LogHealthEventModifierRemoveFailed(const UDamageHandler* AppliedToHandler, const ECombatEventDirection Direction, const FHealthEventModCondition& Modifier,
	const FString& FailReason) const
{
	UE_LOG(LogSaiyoraHealth, Log, TEXT("%s Health Event Modifier FAILED to Remove from handler for %s: %s, FailReason: %s"),
		*UEnum::GetDisplayValueAsText(Direction).ToString(),
		*AppliedToHandler->GetOwner()->GetName(),
		*USaiyoraCombatLibrary::DelegateToLogString(Modifier),
		*FailReason);
	//Optionally dump the whole modifier list on the handler.
	if (HealthEventModifierLoggingVerbosity == EDebugLogVerbosity::FullDump)
	{
		TArray<FString> Dump;
		AppliedToHandler->DEBUG_DumpModifiers(Direction, Dump);
		for (const FString& DumpString : Dump)
		{
			UE_LOG(LogSaiyoraHealth, Log, TEXT("%s"), *DumpString);
		}
	}
}

#pragma endregion

#endif
#pragma endregion 