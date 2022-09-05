#include "CoreClasses/SaiyoraPlayerCharacter.h"
#include "BuffHandler.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "FactionComponent.h"
#include "PlaneComponent.h"
#include "ResourceHandler.h"
#include "SaiyoraMovementComponent.h"
#include "SAT.h"
#include "CoreClasses/SaiyoraPlayerController.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "CoreClasses/SaiyoraGameState.h"
#include "GameFramework/PlayerState.h"

const int32 ASaiyoraPlayerCharacter::MAXABILITYBINDS = 6;

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

void ASaiyoraPlayerCharacter::AbilityInput(const int32 InputNum, const bool bPressed)
{
	if (!IsValid(AbilityComponent) || !IsValid(PlaneComponent))
	{
		return;
	}
	if (PlaneComponent->GetCurrentPlane() == ESaiyoraPlane::Ancient)
	{
		const UCombatAbility* Ability = AncientAbilityMappings.FindRef(InputNum);
		if (bPressed)
		{
			if (IsValid(Ability))
			{
				AbilityComponent->UseAbility(Ability->GetClass());
			}
		}
		else
		{
			//TODO: Cancel ability if CancelOnRelease and check que timer?
		}
	}
	else
	{
		const UCombatAbility* Ability = ModernAbilityMappings.FindRef(InputNum);
		if (bPressed)
		{
			if (IsValid(Ability))
			{
				AbilityComponent->UseAbility(Ability->GetClass());
			}
		}
		else
		{
			//TODO: Cancel ability if CancelOnRelease and check que timer?
		}
	}
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
	for (int i = 0; i < MAXABILITYBINDS; i++)
	{
		AncientAbilityMappings.Add(i, nullptr);
		ModernAbilityMappings.Add(i, nullptr);
	}
	TArray<UCombatAbility*> ActiveAbilities;
	AbilityComponent->GetActiveAbilities(ActiveAbilities);
	for (UCombatAbility* Ability : ActiveAbilities)
	{
		AddAbilityMapping(Ability);
	}
	AbilityComponent->OnAbilityAdded.AddDynamic(this, &ASaiyoraPlayerCharacter::AddAbilityMapping);
	AbilityComponent->OnAbilityRemoved.AddDynamic(this, &ASaiyoraPlayerCharacter::RemoveAbilityMapping);
}

void ASaiyoraPlayerCharacter::AddAbilityMapping(UCombatAbility* NewAbility)
{
	if (NewAbility->HasTag(FSaiyoraCombatTags::Get().ReloadAbility) || NewAbility->HasTag(FSaiyoraCombatTags::Get().StopFireAbility))
	{
		return;
	}
	if (NewAbility->HasTag(FSaiyoraCombatTags::Get().FireWeaponAbility))
	{
		ModernAbilityMappings.Add(0, NewAbility);
		OnMappingChanged.Broadcast(ESaiyoraPlane::Modern, 0, NewAbility);
		return;
	}
	if (NewAbility->GetAbilityPlane() == ESaiyoraPlane::Ancient)
	{
		for (TTuple<int32, UCombatAbility*>& Mapping : AncientAbilityMappings)
		{
			if (!IsValid(Mapping.Value))
			{
				Mapping.Value = NewAbility;
				OnMappingChanged.Broadcast(ESaiyoraPlane::Ancient, Mapping.Key, Mapping.Value);
				break;
			}
		}
	}
	else
	{
		for (TTuple<int32, UCombatAbility*>& Mapping : ModernAbilityMappings)
		{
			if (Mapping.Key != 0 && !IsValid(Mapping.Value))
			{
				Mapping.Value = NewAbility;
				OnMappingChanged.Broadcast(ESaiyoraPlane::Modern, Mapping.Key, Mapping.Value);
				break;
			}
		}
	}
}

void ASaiyoraPlayerCharacter::RemoveAbilityMapping(UCombatAbility* RemovedAbility)
{
	if (RemovedAbility->GetAbilityPlane() == ESaiyoraPlane::Ancient)
	{
		for (TTuple<int32, UCombatAbility*>& Mapping : AncientAbilityMappings)
		{
			if (IsValid(Mapping.Value) && Mapping.Value == RemovedAbility)
			{
				Mapping.Value = nullptr;
				OnMappingChanged.Broadcast(ESaiyoraPlane::Ancient, Mapping.Key, nullptr);
				break;
			}
		}
	}
	else
	{
		for (TTuple<int32, UCombatAbility*>& Mapping : ModernAbilityMappings)
		{
			if (IsValid(Mapping.Value) && Mapping.Value == RemovedAbility)
			{
				Mapping.Value = nullptr;
				OnMappingChanged.Broadcast(ESaiyoraPlane::Modern, Mapping.Key, nullptr);
				break;
			}
		}
	}
}
