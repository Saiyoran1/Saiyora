#include "CombatSystem/Damage/Hitbox.h"
#include "SaiyoraCombatInterface.h"
#include "CoreClasses/SaiyoraGameState.h"
#include "CombatStatusComponent.h"

UHitbox::UHitbox()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UHitbox::InitializeComponent()
{
	SetCollisionProfileName(TEXT("NoCollision"));
}

void UHitbox::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		const UCombatStatusComponent* CombatStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
		if (IsValid(CombatStatusComponent))
		{
			UpdateFactionCollision(CombatStatusComponent->GetCurrentFaction());
		}
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		GameState = GetWorld()->GetGameState<ASaiyoraGameState>();
		if (IsValid(GameState))
		{
			GameState->RegisterNewHitbox(this);
		}
	}
}

void UHitbox::UpdateFactionCollision(const EFaction NewFaction)
{
	switch (NewFaction)
	{
	case EFaction::Enemy :
		SetCollisionProfileName(FSaiyoraCollision::P_NPCHitbox);
		break;
	case EFaction::Neutral :
		SetCollisionProfileName(FSaiyoraCollision::P_NPCHitbox);
		break;
	case EFaction::Friendly :
		SetCollisionProfileName(FSaiyoraCollision::P_PlayerHitbox);
		break;
	default:
		break;
	}
}