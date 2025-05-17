#include "CombatDebugOptions.h"
#include "AbilityStructs.h"
#include "CombatAbility.h"
#include "EngineUtils.h"
#include "Hitbox.h"
#include "NPCStructs.h"
#include "NPCAbility.h"
#include "PredictableProjectile.h"
#include "RotationBehaviors.h"
#include "SaiyoraPlayerCharacter.h"

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
