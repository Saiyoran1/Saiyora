#include "CombatSystem/Generic/Hitbox.h"
#include "SaiyoraCombatInterface.h"
#include "Faction/FactionComponent.h"

UBoxHitbox::UBoxHitbox()
{
	bWantsInitializeComponent = true;
}

USphereHitbox::USphereHitbox()
{
	bWantsInitializeComponent = true;	
}

UCapsuleHitbox::UCapsuleHitbox()
{
	bWantsInitializeComponent = true;	
}

void UBoxHitbox::InitializeComponent()
{
	SetCollisionProfileName(TEXT("NoCollision"));
}

void USphereHitbox::InitializeComponent()
{
	SetCollisionProfileName(TEXT("NoCollision"));
}

void UCapsuleHitbox::InitializeComponent()
{
	SetCollisionProfileName(TEXT("NoCollision"));
}

void UBoxHitbox::BeginPlay()
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
}

void USphereHitbox::BeginPlay()
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
}

void UCapsuleHitbox::BeginPlay()
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
}

void UBoxHitbox::UpdateFactionCollision(EFaction const NewFaction)
{
	switch (NewFaction)
	{
	case EFaction::Enemy :
		SetCollisionProfileName(TEXT("EnemyHitbox"));
		break;
	case EFaction::Neutral :
		SetCollisionProfileName(TEXT("NeutralHitbox"));
		break;
	case EFaction::Player :
		SetCollisionProfileName(TEXT("PlayerHitbox"));
		break;
	default:
		break;
	}
}

void USphereHitbox::UpdateFactionCollision(EFaction const NewFaction)
{
	switch (NewFaction)
	{
	case EFaction::Enemy :
		SetCollisionProfileName(TEXT("EnemyHitbox"));
		break;
	case EFaction::Neutral :
		SetCollisionProfileName(TEXT("NeutralHitbox"));
		break;
	case EFaction::Player :
		SetCollisionProfileName(TEXT("PlayerHitbox"));
		break;
	default:
		break;
	}
}

void UCapsuleHitbox::UpdateFactionCollision(EFaction const NewFaction)
{
	switch (NewFaction)
	{
	case EFaction::Enemy :
		SetCollisionProfileName(TEXT("EnemyHitbox"));
		break;
	case EFaction::Neutral :
		SetCollisionProfileName(TEXT("NeutralHitbox"));
		break;
	case EFaction::Player :
		SetCollisionProfileName(TEXT("PlayerHitbox"));
		break;
	default:
		break;
	}
}