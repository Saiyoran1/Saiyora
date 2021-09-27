// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatSystem/Generic/Hitbox.h"

#include "SaiyoraCombatInterface.h"

UHitbox::UHitbox()
{
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
		UpdateFactionCollision(ISaiyoraCombatInterface::Execute_GetFaction(GetOwner()));
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
		SetCollisionProfileName(TEXT("NeutralHitbox"));
		break;
	case EFaction::Player :
		SetCollisionProfileName(TEXT("PlayerHitbox"));
		break;
	default:
		break;
	}
}
