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
	if (!GameStateRef || !GetPlayerState() || (GetLocalRole() != ROLE_SimulatedProxy && !GetController()))
	{
		return;
	}
	if (!PlayerControllerRef)
	{
		PlayerControllerRef = Cast<ASaiyoraPlayerController>(GetController());
		if (!PlayerControllerRef)
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
