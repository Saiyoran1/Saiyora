#include "CombatSystem/Damage/Hitbox.h"
#include "SaiyoraCombatInterface.h"
#include "CoreClasses/SaiyoraGameState.h"
#include "Faction/FactionComponent.h"

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
		UFactionComponent* FactionComponent = ISaiyoraCombatInterface::Execute_GetFactionComponent(GetOwner());
		if (IsValid(FactionComponent))
		{
			UpdateFactionCollision(FactionComponent->GetCurrentFaction());
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

void UHitbox::UpdateFactionCollision(EFaction const NewFaction)
{
	switch (NewFaction)
	{
	case EFaction::Enemy :
		SetCollisionProfileName(TEXT("EnemyHitbox"));
		break;
	case EFaction::Neutral :
		SetCollisionProfileName(TEXT("EnemyHitbox"));
		break;
	case EFaction::Player :
		SetCollisionProfileName(TEXT("PlayerHitbox"));
		break;
	default:
		break;
	}
}