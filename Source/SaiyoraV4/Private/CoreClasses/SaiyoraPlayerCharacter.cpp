#include "CoreClasses/SaiyoraPlayerCharacter.h"
#include "BuffHandler.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "FactionComponent.h"
#include "PlaneComponent.h"
#include "ResourceHandler.h"
#include "SaiyoraMovementComponent.h"
#include "CoreClasses/SaiyoraPlayerController.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "CoreClasses/SaiyoraGameState.h"
#include "GameFramework/PlayerState.h"

ASaiyoraPlayerCharacter::ASaiyoraPlayerCharacter(const class FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<USaiyoraMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	FactionComponent = CreateDefaultSubobject<UFactionComponent>(TEXT("FactionComponent"));
	PlaneComponent = CreateDefaultSubobject<UPlaneComponent>(TEXT("PlaneComponent"));
	DamageHandler = CreateDefaultSubobject<UDamageHandler>(TEXT("DamageHandler"));
	ThreatHandler = CreateDefaultSubobject<UThreatHandler>(TEXT("ThreatHandler"));
	BuffHandler = CreateDefaultSubobject<UBuffHandler>(TEXT("BuffHandler"));
	StatHandler = CreateDefaultSubobject<UStatHandler>(TEXT("StatHandler"));
	CcHandler = CreateDefaultSubobject<UCrowdControlHandler>(TEXT("CcHandler"));
	AbilityComponent = CreateDefaultSubobject<UAbilityComponent>(TEXT("AbilityComponent"));
	ResourceHandler = CreateDefaultSubobject<UResourceHandler>(TEXT("ResourceHandler"));
}

void ASaiyoraPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	CustomMovementComponent = Cast<USaiyoraMovementComponent>(Super::GetMovementComponent());
}

void ASaiyoraPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	GameStateRef = GetWorld()->GetGameState<ASaiyoraGameState>();
	//Initialize character called here, with valid GameState.
	InitializeCharacter();
	if (IsLocallyControlled())
	{
		SetupAbilityMappings();
	}
}

void ASaiyoraPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	//Initialize character called here with valid controller, on the server.
	InitializeCharacter();
}

void ASaiyoraPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	//Initialize character called here with valid controller, on client.
	InitializeCharacter();
}

void ASaiyoraPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	//Initialize character called here with valid player state, on client.
	InitializeCharacter();
}

void ASaiyoraPlayerCharacter::InitializeCharacter()
{
	if (bInitialized)
	{
		return;
	}
	if (!IsValid(GameStateRef) || !IsValid(GetPlayerState()) || (GetLocalRole() != ROLE_SimulatedProxy && !IsValid(GetController())))
	{
		return;
	}
	if (!IsValid(PlayerControllerRef))
	{
		PlayerControllerRef = Cast<ASaiyoraPlayerController>(GetController());
		if (!IsValid(PlayerControllerRef))
		{
			return;
		}
	}
	if (IsLocallyControlled())
	{
		CreateUserInterface();
	}
	GameStateRef->InitPlayer(this);
	bInitialized = true;
}

void ASaiyoraPlayerCharacter::SetupAbilityMappings()
{
	TArray<UCombatAbility*> ActiveAbilities;
	AbilityComponent->GetActiveAbilities(ActiveAbilities);
	int32 AncientIndex = 0;
	int32 ModernIndex = 1;
	for (UCombatAbility* Ability : ActiveAbilities)
	{
		if (Ability->GetAbilityPlane() == ESaiyoraPlane::Ancient)
		{
			AncientAbilityMappings.Add(AncientIndex, Ability);
			OnMappingChanged.Broadcast(ESaiyoraPlane::Ancient, AncientIndex, Ability);
			AncientIndex++;
		}
		else if (Ability->GetAbilityPlane() == ESaiyoraPlane::Modern)
		{
			ModernAbilityMappings.Add(ModernIndex, Ability);
			OnMappingChanged.Broadcast(ESaiyoraPlane::Modern, ModernIndex, Ability);
			ModernIndex++;
		}
	}
}
