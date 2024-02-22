#include "NPCAbility.h"
#include "AbilityComponent.h"
#include "SaiyoraGameState.h"
#include "ThreatHandler.h"

void UNPCAbility::PostInitializeAbility_Implementation()
{
	Super::PostInitializeAbility_Implementation();

	if (GetHandler()->GetOwner()->HasAuthority() && bUseTokens)
	{
		GameStateRef = Cast<ASaiyoraGameState>(GetWorld()->GetGameState());
		if (IsValid(GameStateRef))
		{
			TokenCallback.BindDynamic(this, &UNPCAbility::OnTokenAvailabilityChanged);
			GameStateRef->InitTokensForAbilityClass(GetClass(), TokenCallback);
			UpdateCastable();
		}
	}
}

void UNPCAbility::OnServerTick_Implementation(const int32 TickNumber)
{
	Super::OnServerTick_Implementation(TickNumber);
	//For instant abilities, we request a token at the start of the server tick.
	//Cast time abilities will request a token from the AbilityComponent's cast state change delegate,
	//because if an ability has no initial tick we won't hit this until later in the cast.
	if (bUseTokens && IsValid(GameStateRef) && CastType == EAbilityCastType::Instant)
	{
		//Request a token for this ability use, so that other instances of this ability won't be castable.
		const bool bGotToken = GameStateRef->RequestTokenForAbility(this);
		if (!bGotToken)
		{
			UE_LOG(LogTemp, Error, TEXT("Ability %s was used even though no token was available!"), *GetName());
			return;
		}
		//Since this is an instant ability, we can instantly return the token. We only took it to start its cooldown.
		GameStateRef->ReturnTokenForAbility(this);
	}
}

void UNPCAbility::AdditionalCastableUpdate(TArray<ECastFailReason>& AdditionalFailReasons)
{
	Super::AdditionalCastableUpdate(AdditionalFailReasons);

	if (bUseTokens && IsValid(GameStateRef))
	{
		if (!GameStateRef->IsTokenAvailableForClass(GetClass()))
		{
			AdditionalFailReasons.AddUnique(ECastFailReason::Token);
		}
	}
}