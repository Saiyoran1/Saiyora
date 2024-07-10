﻿#include "CoreClasses/SaiyoraGameState.h"
#include "CombatDebugOptions.h"
#include "Hitbox.h"
#include "NPCAbility.h"
#include "SaiyoraGameInstance.h"
#include "SaiyoraPlayerCharacter.h"
#include "Kismet/KismetSystemLibrary.h"

static TAutoConsoleVariable<int32> DrawRewindHitboxes(
		TEXT("game.DrawRewindHitboxes"),
		0,
		TEXT("Determines whether server-side validation of hits should draw the rewound hitbox used."),
		ECVF_Default);

ASaiyoraGameState::ASaiyoraGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASaiyoraGameState::BeginPlay()
{
	Super::BeginPlay();

	GameInstanceRef = Cast<USaiyoraGameInstance>(GetGameInstance());
}

void ASaiyoraGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	WorldTime += DeltaSeconds;

	//Display debug information for ability tokens and claimed locations.
	if (IsValid(GameInstanceRef) && IsValid(GameInstanceRef->CombatDebugOptions))
	{
		if (GameInstanceRef->CombatDebugOptions->bDisplayTokenInformation)
		{
			GameInstanceRef->CombatDebugOptions->DisplayTokenInfo(Tokens);
		}
		if (GameInstanceRef->CombatDebugOptions->bDisplayClaimedLocations)
		{
			GameInstanceRef->CombatDebugOptions->DisplayClaimedLocations(ClaimedLocations);
		}
	}
}

void ASaiyoraGameState::InitPlayer(ASaiyoraPlayerCharacter* Player)
{
	if (!IsValid(Player) || ActivePlayers.Contains(Player))
	{
		return;
	}
	ActivePlayers.Add(Player);
	OnPlayerAdded.Broadcast(Player);
}

#pragma region Snapshotting

void ASaiyoraGameState::RegisterNewHitbox(UHitbox* Hitbox)
{
    if (Snapshots.Contains(Hitbox))
    {
        return;
    }
    Snapshots.Add(Hitbox);
    if (Snapshots.Num() == 1)
    {
        GetWorld()->GetTimerManager().SetTimer(SnapshotHandle, this, &ASaiyoraGameState::CreateSnapshot, SnapshotInterval, true);
    }
}

void ASaiyoraGameState::CreateSnapshot()
{
    for (TTuple<UHitbox*, FRewindRecord>& Snapshot : Snapshots)
    {
        if (IsValid(Snapshot.Key))
        {
            Snapshot.Value.Transforms.Add(TTuple<float, FTransform>(GetServerWorldTimeSeconds(), Snapshot.Key->GetComponentTransform()));
            //Everything else in here is just making sure we aren't saving snapshots significantly older than the max lag compensation.
            int32 LowestAcceptableIndex = -1;
            for (int i = 0; i < Snapshot.Value.Transforms.Num(); i++)
            {
                if (Snapshot.Value.Transforms[i].Key < GetServerWorldTimeSeconds() - MaxLagCompensation)
                {
                    LowestAcceptableIndex = i;
                }
                else
                {
                    break;
                }
            }
            if (LowestAcceptableIndex > -1)
            {
                Snapshot.Value.Transforms.RemoveAt(0, LowestAcceptableIndex + 1);
            }
        }
    }
}

FTransform ASaiyoraGameState::RewindHitbox(UHitbox* Hitbox, const float Timestamp)
{
	const float RewindTimestamp = FMath::Clamp(Timestamp, GetServerWorldTimeSeconds() - MaxLagCompensation, GetServerWorldTimeSeconds());
    const FTransform OriginalTransform = Hitbox->GetComponentTransform();
    //Zero rewind time means just use the current transform.
    if (RewindTimestamp == GetServerWorldTimeSeconds())
    {
    	return OriginalTransform;
    }
    FRewindRecord* Record = Snapshots.Find(Hitbox);
    //If this hitbox wasn't registered or hasn't had a snapshot yet, we won't rewind it.
    if (!Record)
    {
    	return OriginalTransform;
    }
    if (Record->Transforms.Num() == 0)
    {
    	return OriginalTransform;
    }
    int32 AfterIndex = -1;
    //Iterate trying to find the first record that comes AFTER the timestamp.
    for (int i = 0; i < Record->Transforms.Num(); i++)
    {
    	if (RewindTimestamp <= Record->Transforms[i].Key)
    	{
    		AfterIndex = i;
    		break;
    	}
    }
    //If the very first snapshot is after the timestamp, immediately apply max lag compensation (rewinding to the oldest snapshot).
    if (AfterIndex == 0)
    {
    	Hitbox->SetWorldTransform(Record->Transforms[0].Value);
    	if (DrawRewindHitboxes.GetValueOnGameThread() > 0)
    	{
    		UKismetSystemLibrary::DrawDebugBox(Hitbox, Hitbox->GetComponentLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Blue, Hitbox->GetComponentRotation(), 5.0f, 2);
    		UKismetSystemLibrary::DrawDebugBox(Hitbox, OriginalTransform.GetLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Green, OriginalTransform.Rotator(), 5.0f, 2);
    	}
    	return OriginalTransform;
    }
    float BeforeTimestamp;
    float AfterTimestamp;
    FTransform BeforeTransform;
    FTransform AfterTransform;
    if (Record->Transforms.IsValidIndex(AfterIndex))
    {
    	BeforeTimestamp = Record->Transforms[AfterIndex - 1].Key;
    	BeforeTransform = Record->Transforms[AfterIndex - 1].Value;
    	AfterTimestamp = Record->Transforms[AfterIndex].Key;
    	AfterTransform = Record->Transforms[AfterIndex].Value;
    }
    else
    {
    	//If we didn't find a record after the timestamp, we can interpolate from the last record to current position.
    	BeforeTimestamp = Record->Transforms[Record->Transforms.Num() - 1].Key;
    	BeforeTransform = Record->Transforms[Record->Transforms.Num() - 1].Value;
    	AfterTimestamp = GetServerWorldTimeSeconds();
    	AfterTransform = Hitbox->GetComponentTransform();
    }
    //Find out what fraction of the way from the before timestamp to the after timestamp our target timestamp is.
    const float SnapshotGap = AfterTimestamp - BeforeTimestamp;
    const float SnapshotFraction = (RewindTimestamp - BeforeTimestamp) / SnapshotGap;
    //Interpolate location.
    const FVector LocDiff = AfterTransform.GetLocation() - BeforeTransform.GetLocation();
    const FVector InterpVector = LocDiff * SnapshotFraction;
    Hitbox->SetWorldLocation(BeforeTransform.GetLocation() + InterpVector);
    //Interpolate rotation.
    const FRotator RotDiff = AfterTransform.Rotator() - BeforeTransform.Rotator();
    const FRotator InterpRotator = RotDiff * SnapshotFraction;
    Hitbox->SetWorldRotation(BeforeTransform.Rotator() + InterpRotator);
    //Don't interpolate scale (I don't currently have smooth scale changes). Just pick whichever is closer to the target timestamp.
    Hitbox->SetWorldScale3D(SnapshotFraction <= 0.5f ? BeforeTransform.GetScale3D() : AfterTransform.GetScale3D());
	if (DrawRewindHitboxes.GetValueOnGameThread() > 0)
	{
		UKismetSystemLibrary::DrawDebugBox(Hitbox, Hitbox->GetComponentLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Blue, Hitbox->GetComponentRotation(), 5.0f, 2);
		UKismetSystemLibrary::DrawDebugBox(Hitbox, OriginalTransform.GetLocation(), Hitbox->Bounds.BoxExtent, FLinearColor::Green, OriginalTransform.Rotator(), 5.0f, 2);
	}
    return OriginalTransform;
}

#pragma endregion
#pragma region Tokens

void ASaiyoraGameState::InitTokensForAbilityClass(const TSubclassOf<UNPCAbility> AbilityClass, const FAbilityTokenCallback& TokenCallback)
{
	if (!IsValid(AbilityClass))
	{
		return;
	}
	FNPCAbilityTokens& ClassTokens = Tokens.FindOrAdd(AbilityClass);
	//If this is the first instance of this ability class, we need to populate the array of tokens.
	if (ClassTokens.Tokens.Num() == 0)
	{
		ClassTokens.Tokens.AddDefaulted(AbilityClass->GetDefaultObject<UNPCAbility>()->GetMaxTokens());
		ClassTokens.AvailableCount = ClassTokens.Tokens.Num();
	}
	//Bind the ability's token update delegate, so that it gets updates when tokens become available or unavailable.
	ClassTokens.OnTokenAvailabilityChanged.AddUnique(TokenCallback);
}

bool ASaiyoraGameState::RequestAbilityToken(const UNPCAbility* Ability, const bool bReservation)
{
	if (!IsValid(Ability))
	{
		return false;
	}
	FNPCAbilityTokens* TokensForClass = Tokens.Find(Ability->GetClass());
	if (!TokensForClass)
	{
		return false;
	}
	//Search to find a token that is available.
	bool bClaimedToken = false;
	bool bAvailabilityChanged = false;
	int FirstAvailableIndex = -1;
	for (int i = 0; i < TokensForClass->Tokens.Num(); i++)
	{
		FNPCAbilityToken& Token = TokensForClass->Tokens[i];
		//If this is a reservation, we want a token that is available.
		if (bReservation)
		{
			//TODO: Prevent multiple reservations for one ability?
			if (Token.State == ENPCAbilityTokenState::Available)
			{
				Token.State = ENPCAbilityTokenState::Reserved;
				Token.OwningInstance = Ability;
				bClaimedToken = true;
				TokensForClass->AvailableCount--;
				bAvailabilityChanged = true;
				break;
			}
		}
		//If this is not a reservation, we can look for an existing reservation to mark as in use.
		//If there is no existing reservation, then we can use a token that is available.
		else
		{
			if (Token.State == ENPCAbilityTokenState::Reserved)
			{
				if (Token.OwningInstance == Ability)
				{
					Token.State = ENPCAbilityTokenState::InUse;
					bClaimedToken = true;
					break;
				}
			}
			//If the token is available, save off its index so we can use it if we don't find a reservation.
			else if (Token.State == ENPCAbilityTokenState::Available && FirstAvailableIndex == -1)
			{
				FirstAvailableIndex = i;
			}
		}
	}
	//If this is not a reservation, and we didn't find an existing reservation, but we found an available token, claim it here.
	if (!bReservation && !bClaimedToken && FirstAvailableIndex != -1)
	{
		FNPCAbilityToken& Token = TokensForClass->Tokens[FirstAvailableIndex];
		Token.State = ENPCAbilityTokenState::InUse;
		Token.OwningInstance = Ability;
		bClaimedToken = true;
		TokensForClass->AvailableCount--;
		bAvailabilityChanged = true;
	}
	//If this was the last available token, update all instances of the ability so that they become uncastable.
	if (bAvailabilityChanged && TokensForClass->AvailableCount == 0)
	{
		TokensForClass->OnTokenAvailabilityChanged.Broadcast(false);
	}
	return bClaimedToken;
}

void ASaiyoraGameState::ReturnAbilityToken(const UNPCAbility* Ability)
{
	if (!IsValid(Ability))
	{
		return;
	}
	FNPCAbilityTokens* TokensForClass = Tokens.Find(Ability->GetClass());
	if (!TokensForClass)
	{
		return;
	}
	//Find the token assigned to this ability instance.
	for (int i = 0; i < TokensForClass->Tokens.Num(); i++)
	{
		FNPCAbilityToken& Token = TokensForClass->Tokens[i];
		if ((Token.State == ENPCAbilityTokenState::InUse || Token.State == ENPCAbilityTokenState::Reserved) && Token.OwningInstance == Ability)
		{
			//Place this token on cooldown. It will become usable either after 1 frame (for no cooldown, or when freeing up a reservation) or after the cooldown time.
			FTimerDelegate CooldownDelegate;
			CooldownDelegate.BindUObject(this, &ASaiyoraGameState::FinishTokenCooldown, TSubclassOf<UNPCAbility>(Ability->GetClass()), i);
			if (Token.State == ENPCAbilityTokenState::Reserved || Ability->GetTokenCooldownTime() <= 0.0f)
			{
				GetWorld()->GetTimerManager().SetTimerForNextTick(CooldownDelegate);
			}
			else
			{
				GetWorld()->GetTimerManager().SetTimer(Token.CooldownHandle, CooldownDelegate, Ability->GetTokenCooldownTime(), false);
			}
			Token.OwningInstance = nullptr;
			Token.State = ENPCAbilityTokenState::Cooldown;
			return;
		}
	}
}

void ASaiyoraGameState::FinishTokenCooldown(TSubclassOf<UNPCAbility> AbilityClass, int TokenIndex)
{
	if (!IsValid(AbilityClass))
	{
		return;
	}
	FNPCAbilityTokens* TokensForClass = Tokens.Find(AbilityClass);
	if (!TokensForClass || !TokensForClass->Tokens.IsValidIndex(TokenIndex))
	{
		return;
	}
	//Find the token this cooldown event fired for.
	FNPCAbilityToken& Token = TokensForClass->Tokens[TokenIndex];
	Token.State = ENPCAbilityTokenState::Available;
	GetWorld()->GetTimerManager().ClearTimer(Token.CooldownHandle);
	TokensForClass->AvailableCount++;
	//If this is the only (newly) available token, alert ability instances so they can become castable.
	if (TokensForClass->AvailableCount == 1)
	{
		TokensForClass->OnTokenAvailabilityChanged.Broadcast(true);
	}
}

bool ASaiyoraGameState::IsAbilityTokenAvailable(const UNPCAbility* Ability) const
{
	if (!IsValid(Ability))
	{
		return false;
	}
	const FNPCAbilityTokens* TokensForClass = Tokens.Find(Ability->GetClass());
	if (!TokensForClass)
	{
		return false;
	}
	//If any tokens are freely available, return true.
	if (TokensForClass->AvailableCount > 0)
	{
		return true;
	}
	//If all tokens are in use or reserved, check to see if any of the tokens were reserved for this ability.
	for (const FNPCAbilityToken& Token : TokensForClass->Tokens)
	{
		if (Token.State == ENPCAbilityTokenState::Reserved && Token.OwningInstance == Ability)
		{
			return true;
		}
	}
	return false;
}

#pragma endregion
#pragma region NPC Location Claiming

void ASaiyoraGameState::ClaimLocation(AActor* Actor, const FVector& Location)
{
	if (!HasAuthority() || !IsValid(Actor))
	{
		return;
	}
	ClaimedLocations.Add(Actor, Location);
}

void ASaiyoraGameState::FreeLocation(AActor* Actor)
{
	if (!HasAuthority() || !IsValid(Actor))
	{
		return;
	}
	ClaimedLocations.Remove(Actor);
}

float ASaiyoraGameState::GetScorePenaltyForLocation(const AActor* Actor, const FVector& Location) const
{
	if (!HasAuthority())
	{
		return 0.0f;
	}
	float Penalty = 0.0f;
	for (const TTuple<AActor*, FVector>& ClaimedTuple : ClaimedLocations)
	{
		if (!IsValid(ClaimedTuple.Key) || (IsValid(Actor) && ClaimedTuple.Key == Actor))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Location, ClaimedTuple.Value);
		if (DistSq < FMath::Square(200.0f))
		{
			Penalty -= (1.0f - (FMath::Sqrt(DistSq) / 200.0f));
		}
	}
	return Penalty;
}

#pragma endregion 