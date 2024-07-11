#include "NPCSubsystem.h"
#include "CombatDebugOptions.h"
#include "NPCAbility.h"
#include "SaiyoraGameInstance.h"

bool UNPCSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	const UWorld* World = Cast<UWorld>(Outer);
	//Don't spawn this subsystem on clients. Only the server needs to manage NPCs.
	if (!IsValid(World) || World->IsNetMode(NM_Client))
	{
		return false;
	}
	return true;
}

void UNPCSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const USaiyoraGameInstance* GameInstanceRef = Cast<USaiyoraGameInstance>(GetWorld()->GetGameInstance());
	if (IsValid(GameInstanceRef))
	{
		DebugOptions = GameInstanceRef->CombatDebugOptions;
	}
}

void UNPCSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Debug options for NPC systems.
	if (IsValid(DebugOptions))
	{
		if (DebugOptions->bDisplayClaimedLocations)
		{
			DebugOptions->DisplayClaimedLocations(ClaimedLocations);
		}
		if (DebugOptions->bDisplayTokenInformation)
		{
			DebugOptions->DisplayTokenInfo(AbilityTokens);
		}
	}
}

#pragma region Location Claiming

void UNPCSubsystem::ClaimLocation(AActor* Actor, const FVector& Location)
{
	if (!IsValid(Actor))
	{
		return;
	}
	ClaimedLocations.Add(Actor, Location);
}

void UNPCSubsystem::FreeLocation(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}
	ClaimedLocations.Remove(Actor);
}

float UNPCSubsystem::GetScorePenaltyForLocation(const AActor* Actor, const FVector& Location) const
{
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
#pragma region Ability Tokens

void UNPCSubsystem::InitTokensForAbilityClass(const TSubclassOf<UNPCAbility> AbilityClass, const FAbilityTokenCallback& TokenCallback)
{
	if (!IsValid(AbilityClass))
	{
		return;
	}
	FNPCAbilityTokens& ClassTokens = AbilityTokens.FindOrAdd(AbilityClass);
	//If this is the first instance of this ability class, we need to populate the array of tokens.
	if (ClassTokens.Tokens.Num() == 0)
	{
		ClassTokens.Tokens.AddDefaulted(AbilityClass->GetDefaultObject<UNPCAbility>()->GetMaxTokens());
		ClassTokens.AvailableCount = ClassTokens.Tokens.Num();
	}
	//Bind the ability's token update delegate, so that it gets updates when tokens become available or unavailable.
	ClassTokens.OnTokenAvailabilityChanged.AddUnique(TokenCallback);
}

bool UNPCSubsystem::RequestAbilityToken(const UNPCAbility* Ability, const bool bReservation)
{
	if (!IsValid(Ability))
	{
		return false;
	}
	FNPCAbilityTokens* TokensForClass = AbilityTokens.Find(Ability->GetClass());
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

void UNPCSubsystem::ReturnAbilityToken(const UNPCAbility* Ability)
{
	if (!IsValid(Ability))
	{
		return;
	}
	FNPCAbilityTokens* TokensForClass = AbilityTokens.Find(Ability->GetClass());
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
			CooldownDelegate.BindUObject(this, &UNPCSubsystem::FinishTokenCooldown, TSubclassOf<UNPCAbility>(Ability->GetClass()), i);
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

void UNPCSubsystem::FinishTokenCooldown(TSubclassOf<UNPCAbility> AbilityClass, int TokenIndex)
{
	if (!IsValid(AbilityClass))
	{
		return;
	}
	FNPCAbilityTokens* TokensForClass = AbilityTokens.Find(AbilityClass);
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

bool UNPCSubsystem::IsAbilityTokenAvailable(const UNPCAbility* Ability) const
{
	if (!IsValid(Ability))
	{
		return false;
	}
	const FNPCAbilityTokens* TokensForClass = AbilityTokens.Find(Ability->GetClass());
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