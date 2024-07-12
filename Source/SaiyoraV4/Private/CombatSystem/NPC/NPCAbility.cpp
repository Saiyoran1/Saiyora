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

void UNPCAbility::AdditionalCastableUpdate(TArray<ECastFailReason>& AdditionalFailReasons)
{
	Super::AdditionalCastableUpdate(AdditionalFailReasons);

	if (bUseTokens && (!IsValid(NPCSubsystemRef) || !NPCSubsystemRef->IsAbilityTokenAvailable(this)))
	{
		AdditionalFailReasons.AddUnique(ECastFailReason::Token);
	}
}

void UNPCAbility::OnServerCastStart_Implementation()
{
	Super::OnServerCastStart_Implementation();
	if (bUseTokens)
	{
		NPCSubsystemRef->RequestAbilityToken(this);
	}
}

void UNPCAbility::OnServerCastEnd_Implementation()
{
	Super::OnServerCastEnd_Implementation();
	if (bUseTokens)
	{
		NPCSubsystemRef->ReturnAbilityToken(this);
	}
}