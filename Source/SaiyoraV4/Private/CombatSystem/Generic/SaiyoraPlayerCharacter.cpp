#include "SaiyoraPlayerCharacter.h"
#include "BuffHandler.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "FactionComponent.h"
#include "PlaneComponent.h"
#include "ResourceHandler.h"
#include "SaiyoraGameState.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"

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
	if (GameStateRef)
	{
		GameStateRef->InitPlayer(this);
	}
}
