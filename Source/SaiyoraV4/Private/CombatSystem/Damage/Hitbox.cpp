#include "CombatSystem/Damage/Hitbox.h"
#include "AbilityComponent.h"
#include "CombatNetSubsystem.h"
#include "SaiyoraCombatInterface.h"
#include "CombatStatusComponent.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatLibrary.h"

UHitbox::UHitbox()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UHitbox::InitializeComponent()
{
	Super::InitializeComponent();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
	if (GetOwner()->Implements<USaiyoraCombatInterface>())
	{
		CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
		UAbilityComponent* AbilityCompRef = ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwner());
		if (IsValid(AbilityCompRef))
		{
			NPCComponentRef = Cast<UNPCAbilityComponent>(AbilityCompRef);
		}
	}
}

void UHitbox::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsValid(NPCComponentRef))
	{
		NPCComponentRef->OnCombatBehaviorChanged.AddDynamic(this, &UHitbox::OnCombatBehaviorChanged);
		OnCombatBehaviorChanged(ENPCCombatBehavior::None, NPCComponentRef->GetCombatBehavior());
	}
	else if (IsValid(CombatStatusComponentRef))
	{
		UpdateFactionCollision(CombatStatusComponentRef->GetCurrentFaction());
	}
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		UCombatNetSubsystem* NetSubsystem = GetWorld()->GetSubsystem<UCombatNetSubsystem>();
		if (IsValid(NetSubsystem))
		{
			NetSubsystem->RegisterNewHitbox(this);
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

void UHitbox::OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior, const ENPCCombatBehavior NewBehavior)
{
	if (!USaiyoraCombatLibrary::IsActiveNPCBehavior(NewBehavior))
	{
		//Disable collision when resetting.
		SetCollisionProfileName(TEXT("NoCollision"));
	}
	else if (IsValid(CombatStatusComponentRef)
		&& !USaiyoraCombatLibrary::IsActiveNPCBehavior(PreviousBehavior))
	{
		//If collision was previously disabled and we are entering Patrol/Combat behavior, enable collision.
		UpdateFactionCollision(CombatStatusComponentRef->GetCurrentFaction());
	}
}
