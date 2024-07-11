#include "NPCAbility.h"
#include "AbilityComponent.h"
#include "NPCSubsystem.h"
#include "ThreatHandler.h"

void UNPCAbility::PostInitializeAbility_Implementation()
{
	Super::PostInitializeAbility_Implementation();

	if (GetHandler()->GetOwner()->HasAuthority() && bUseTokens)
	{
		NPCSubsystemRef = GetWorld()->GetSubsystem<UNPCSubsystem>();
		checkf(IsValid(NPCSubsystemRef), TEXT("Invalid NPC Subsystem ref in NPC Ability."));
		
		TokenCallback.BindDynamic(this, &UNPCAbility::OnTokenAvailabilityChanged);
		NPCSubsystemRef->InitTokensForAbilityClass(GetClass(), TokenCallback);
		UpdateCastable();
	}
}

void UNPCAbility::OnServerTick_Implementation(const int32 TickNumber)
{
	Super::OnServerTick_Implementation(TickNumber);
	//For instant abilities, we request a token at the start of the server tick.
	//Cast time abilities will request a token from the AbilityComponent's cast state change delegate,
	//because if an ability has no initial tick we won't hit this until later in the cast.
	if (bUseTokens && CastType == EAbilityCastType::Instant)
	{
		//Request a token for this ability use, so that other instances of this ability won't be castable.
		const bool bGotToken = NPCSubsystemRef->RequestAbilityToken(this);
		if (!bGotToken)
		{
			UE_LOG(LogTemp, Error, TEXT("Ability %s was used even though no token was available!"), *GetName());
			return;
		}
		//Since this is an instant ability, we can instantly return the token. We only took it to start its cooldown.
		NPCSubsystemRef->ReturnAbilityToken(this);
	}
}

void UNPCAbility::AdditionalCastableUpdate(TArray<ECastFailReason>& AdditionalFailReasons)
{
	Super::AdditionalCastableUpdate(AdditionalFailReasons);

	if (bUseTokens && !NPCSubsystemRef->IsAbilityTokenAvailable(this))
	{
		AdditionalFailReasons.AddUnique(ECastFailReason::Token);
	}
}