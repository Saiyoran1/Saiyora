#include "CoreClasses/SaiyoraPlayerCharacter.h"
#include "BuffHandler.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "CombatStatusComponent.h"
#include "Weapons/FireWeapon.h"
#include "PredictableProjectile.h"
#include "ResourceHandler.h"
#include "SaiyoraMovementComponent.h"
#include "CoreClasses/SaiyoraPlayerController.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "CoreClasses/SaiyoraGameState.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/PlayerState.h"
#include "Specialization/AncientSpecialization.h"
#include "Weapons/Reload.h"
#include "Weapons/StopFiring.h"
#include "Weapons/Weapon.h"

#pragma region Initialization

ASaiyoraPlayerCharacter::ASaiyoraPlayerCharacter(const class FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<USaiyoraMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	CombatStatusComponent = CreateDefaultSubobject<UCombatStatusComponent>(TEXT("CombatStatusComponent"));
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
	AbilityComponent->OnCastStateChanged.AddDynamic(this, &ASaiyoraPlayerCharacter::UpdateQueueOnCastEnd);
	AbilityComponent->OnGlobalCooldownChanged.AddDynamic(this, &ASaiyoraPlayerCharacter::UpdateQueueOnGlobalEnd);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ASaiyoraPlayerCharacter::HandleBeginXPlaneOverlap);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &ASaiyoraPlayerCharacter::HandleEndXPlaneOverlap);
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

void ASaiyoraPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASaiyoraPlayerCharacter, AncientSpec);
}

bool ASaiyoraPlayerCharacter::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	if (IsValid(AncientSpec))
	{
		bWroteSomething |= Channel->ReplicateSubobject(AncientSpec, *Bunch, *RepFlags);
	}
	if (IsValid(RecentlyUnlearnedAncientSpec))
	{
		bWroteSomething |= Channel->ReplicateSubobject(AncientSpec, *Bunch, *RepFlags);
	}
	return bWroteSomething;
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
		SetupAbilityMappings();
		CreateUserInterface();
		CombatStatusComponent->OnPlaneSwapped.AddDynamic(this, &ASaiyoraPlayerCharacter::ClearQueueAndAutoFireOnPlaneSwap);
	}
	GameStateRef->InitPlayer(this);
	bInitialized = true;
}

#pragma endregion
#pragma region Ability Mappings

void ASaiyoraPlayerCharacter::SetupAbilityMappings()
{
	for (int i = 0; i < MaxAbilityBinds; i++)
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
	if (NewAbility->HasTag(FSaiyoraCombatTags::Get().FireWeaponAbility))
	{
		ModernAbilityMappings.Add(0, NewAbility);
		FireWeaponAbility = Cast<UFireWeapon>(NewAbility);
		OnMappingChanged.Broadcast(ESaiyoraPlane::Modern, 0, NewAbility);
	}
	else if (NewAbility->HasTag(FSaiyoraCombatTags::Get().ReloadAbility))
	{
		ReloadAbility = Cast<UReload>(NewAbility);
	}
	else if (NewAbility->HasTag(FSaiyoraCombatTags::Get().StopFireAbility))
	{
		StopFiringAbility = Cast<UStopFiring>(NewAbility);
	}
	else if (NewAbility->GetAbilityPlane() == ESaiyoraPlane::Ancient)
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
	if (RemovedAbility == AutomaticInputAbility)
	{
		AutomaticInputAbility = nullptr;
	}
}

#pragma endregion
#pragma region Ability Input

void ASaiyoraPlayerCharacter::AbilityInput(const int32 InputNum, const bool bPressed)
{
	if (!IsValid(AbilityComponent) || !IsValid(CombatStatusComponent))
	{
		return;
	}
	UCombatAbility* AbilityToUse = CombatStatusComponent->GetCurrentPlane() == ESaiyoraPlane::Ancient ? AncientAbilityMappings.FindRef(InputNum) : ModernAbilityMappings.FindRef(InputNum);
	if (!IsValid(AbilityToUse))
	{
		return;
	}
	if (bPressed)
	{
		//If we are firing and we press a new ability that isn't fire weapon, stop firing first.
		if (AbilityToUse != FireWeaponAbility && IsValid(Weapon) && Weapon->IsBurstFiring() && IsValid(StopFiringAbility))
		{
			AbilityComponent->UseAbility(StopFiringAbility->GetClass());
		}
		//Attempt to use the ability.
		const FAbilityEvent InitialAttempt = AbilityComponent->UseAbility(AbilityToUse->GetClass());
		if (InitialAttempt.ActionTaken == ECastAction::Fail && (InitialAttempt.FailReason == ECastFailReason::AlreadyCasting || InitialAttempt.FailReason == ECastFailReason::OnGlobalCooldown))
		{
			//If the attempt failed due to cast in progress or GCD, try and queue.
			if (!TryQueueAbility(AbilityToUse->GetClass()) && InitialAttempt.FailReason == ECastFailReason::AlreadyCasting)
			{
				//If we couldn't que, and originally failed due to casting, cancel the current cast and then re-attempt.
				if (AbilityComponent->CancelCurrentCast().bSuccess)
				{
					AbilityComponent->UseAbility(AbilityToUse->GetClass());
				}
			}
		}
		//If this was a failure to fire a weapon, stop firing and try to reload.
		else if (AbilityToUse == FireWeaponAbility && InitialAttempt.ActionTaken == ECastAction::Fail)
		{
			if (IsValid(Weapon) && Weapon->IsBurstFiring() && IsValid(StopFiringAbility))
			{
				AbilityComponent->UseAbility(StopFiringAbility->GetClass());
			}
			if (InitialAttempt.FailReason == ECastFailReason::CostsNotMet && IsValid(ReloadAbility))
			{
				AbilityComponent->UseAbility(ReloadAbility->GetClass());
			}
		}
		if (AbilityToUse->IsAutomatic())
		{
			AutomaticInputAbility = AbilityToUse;
		}
	}
	else
	{
		if (AbilityToUse == FireWeaponAbility && IsValid(Weapon) && Weapon->IsBurstFiring() && IsValid(StopFiringAbility))
		{
			AbilityComponent->UseAbility(StopFiringAbility->GetClass());
		}
		//If the input we are releasing is for the current cast, that cast is cancellable on release, AND we aren't within the queue window, cancel our cast. (Within the queue window, we allow the cast to finish).
		if (AbilityComponent->IsCasting() && AbilityComponent->GetCurrentCast() == AbilityToUse && AbilityComponent->GetCurrentCast()->WillCancelOnRelease() && (AbilityComponent->GetCastTimeRemaining() > AbilityQueueWindow))
		{
			AbilityComponent->CancelCurrentCast();
		}
		//Else if the input we are releasing is for an ability that is currently queued, and that ability is cancellable on release, we just clear the queue (the cast would instantly end anyway).
		else if (QueueStatus != EQueueStatus::Empty && QueuedAbility->GetClass() == AbilityToUse->GetClass() && AbilityToUse->WillCancelOnRelease())
		{
			//TODO: There is potentially a problem if we WANT to instant tick and then cancel a cast at the end of another cast. We'll see if this is something that's ever desirable.
			ExpireQueue();
		}
		if (AbilityToUse->IsAutomatic() && AbilityToUse == AutomaticInputAbility)
		{
			AutomaticInputAbility = nullptr;
		}
	}
}

void ASaiyoraPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsLocallyControlled() && QueueStatus == EQueueStatus::Empty && IsValid(AutomaticInputAbility))
	{
		const FAbilityEvent Event = AbilityComponent->UseAbility(AutomaticInputAbility->GetClass());
		if (AutomaticInputAbility == FireWeaponAbility && Event.ActionTaken == ECastAction::Fail)
		{
			if (Event.FailReason != ECastFailReason::AbilityConditionsNotMet && IsValid(Weapon) && Weapon->IsBurstFiring() && IsValid(StopFiringAbility))
			{
				AbilityComponent->UseAbility(StopFiringAbility->GetClass());
			}
			if (Event.FailReason == ECastFailReason::CostsNotMet && IsValid(ReloadAbility))
			{
				AbilityComponent->UseAbility(ReloadAbility->GetClass());
			}
		}
	}
}

bool ASaiyoraPlayerCharacter::TryQueueAbility(const TSubclassOf<UCombatAbility> AbilityClass)
{
	if (!IsValid(AbilityComponent) || !IsLocallyControlled() || !IsValid(AbilityClass))
	{
		return false;
	}
	//If  we aren't casting and aren't on GCD, don't queue.
	if (!AbilityComponent->IsCasting() && !AbilityComponent->IsGlobalCooldownActive())
	{
		return false;
	}
	const float GlobalTimeRemaining = AbilityComponent->GetGlobalCooldownTimeRemaining();
	const float CastTimeRemaining = AbilityComponent->GetCastTimeRemaining();
	if (GlobalTimeRemaining > AbilityQueueWindow || CastTimeRemaining > AbilityQueueWindow)
	{
		return false;
	}
	if (GlobalTimeRemaining == CastTimeRemaining)
	{
		QueueStatus = EQueueStatus::WaitForBoth;
	}
	else if (GlobalTimeRemaining > CastTimeRemaining)
	{
		QueueStatus = EQueueStatus::WaitForGlobal;
	}
	else
	{
		QueueStatus = EQueueStatus::WaitForCast;
	}
	QueuedAbility = AbilityClass;
	GetWorld()->GetTimerManager().SetTimer(QueueExpirationHandle, this, &ASaiyoraPlayerCharacter::ExpireQueue, AbilityQueueWindow);
	return true;
}

void ASaiyoraPlayerCharacter::UpdateQueueOnGlobalEnd(const FGlobalCooldown& OldGlobalCooldown, const FGlobalCooldown& NewGlobalCooldown)
{
	if (NewGlobalCooldown.bActive)
	{
		return;
	}
	if (QueueStatus == EQueueStatus::WaitForGlobal)
	{
		const TSubclassOf<UCombatAbility> AbilityClass = QueuedAbility;
		ExpireQueue();
		//Execute the queued ability on next tick to prevent conflicts with other delegates tied to GCD end that might trigger an ability.
		const FTimerDelegate QueueDelegate = FTimerDelegate::CreateUObject(this, &ASaiyoraPlayerCharacter::UseAbilityFromQueue, AbilityClass);
		GetWorld()->GetTimerManager().SetTimerForNextTick(QueueDelegate);
	}
	else if (QueueStatus == EQueueStatus::WaitForBoth)
	{
		QueueStatus = EQueueStatus::WaitForCast;
	}
}

void ASaiyoraPlayerCharacter::UpdateQueueOnCastEnd(const FCastingState& OldState, const FCastingState& NewState)
{
	if (NewState.bIsCasting)
	{
		return;
	}
	if (QueueStatus == EQueueStatus::WaitForCast)
	{
		const TSubclassOf<UCombatAbility> AbilityClass = QueuedAbility;
		ExpireQueue();
		//Execute the queued ability on next tick to prevent conflicts with other delegates tied to cast end that might trigger an ability.
		const FTimerDelegate QueueDelegate = FTimerDelegate::CreateUObject(this, &ASaiyoraPlayerCharacter::UseAbilityFromQueue, AbilityClass);
		GetWorld()->GetTimerManager().SetTimerForNextTick(QueueDelegate);
	}
	else if (QueueStatus == EQueueStatus::WaitForBoth)
	{
		QueueStatus = EQueueStatus::WaitForGlobal;
	}
}

void ASaiyoraPlayerCharacter::UseAbilityFromQueue(const TSubclassOf<UCombatAbility> AbilityClass)
{
	const FAbilityEvent Event = AbilityComponent->UseAbility(AbilityClass);
	if (AbilityClass == FireWeaponAbility->GetClass() && Event.ActionTaken == ECastAction::Fail)
	{
		if (IsValid(Weapon) && Weapon->IsBurstFiring() && IsValid(StopFiringAbility))
		{
			AbilityComponent->UseAbility(StopFiringAbility->GetClass());
		}
		if (Event.FailReason == ECastFailReason::CostsNotMet && IsValid(ReloadAbility))
		{
			AbilityComponent->UseAbility(ReloadAbility->GetClass());
		}
	}
}

void ASaiyoraPlayerCharacter::ExpireQueue()
{
	QueueStatus = EQueueStatus::Empty;
	QueuedAbility = nullptr;
	GetWorld()->GetTimerManager().ClearTimer(QueueExpirationHandle);
}

void ASaiyoraPlayerCharacter::ClearQueueAndAutoFireOnPlaneSwap(const ESaiyoraPlane PreviousPlane,
                                                               const ESaiyoraPlane NewPlane, UObject* Source)
{
	AutomaticInputAbility = nullptr;
	ExpireQueue();
}

#pragma endregion
#pragma region Collision

void ASaiyoraPlayerCharacter::HandleBeginXPlaneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const bool bPreviouslyOverlapping = XPlaneOverlaps.Num() > 0;
	ESaiyoraPlane OtherCompPlane = ESaiyoraPlane::None;
	switch (OtherComp->GetCollisionObjectType())
	{
	case FSaiyoraCollision::O_WorldAncient :
		OtherCompPlane = ESaiyoraPlane::Ancient;
		break;
	case FSaiyoraCollision::O_WorldModern :
		OtherCompPlane = ESaiyoraPlane::Modern;
		break;
	default :
		OtherCompPlane = ESaiyoraPlane::Both;
		break;
	}
	if (UCombatStatusComponent::CheckForXPlane(CombatStatusComponent->GetCurrentPlane(), OtherCompPlane))
	{
		XPlaneOverlaps.Add(OtherComp);
	}
	if (!bPreviouslyOverlapping && XPlaneOverlaps.Num() > 0)
	{
		CombatStatusComponent->AddPlaneSwapRestriction(GetCapsuleComponent());
	}
}

void ASaiyoraPlayerCharacter::HandleEndXPlaneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const bool bPreviouslyOverlapping = XPlaneOverlaps.Num() > 0;
	XPlaneOverlaps.Remove(OtherComp);
	if (bPreviouslyOverlapping && XPlaneOverlaps.Num() <= 0)
	{
		CombatStatusComponent->RemovePlaneSwapRestriction(GetCapsuleComponent());
	}
}

#pragma endregion 
#pragma region Projectiles

void ASaiyoraPlayerCharacter::RegisterClientProjectile(APredictableProjectile* Projectile)
{
	if (GetLocalRole() != ROLE_AutonomousProxy || !IsValid(Projectile))
	{
		return;
	}
	FPredictedTickProjectiles* TickProjectiles = PredictedProjectiles.Find(Projectile->GetSourceInfo().SourceTick);
	if (!TickProjectiles)
	{
		TickProjectiles = &PredictedProjectiles.Add(Projectile->GetSourceInfo().SourceTick, FPredictedTickProjectiles());
	}
	TickProjectiles->Projectiles.Add(Projectile->GetSourceInfo().ID, Projectile);
}

void ASaiyoraPlayerCharacter::ClientNotifyFailedProjectileSpawn_Implementation(const FPredictedTick& Tick,
                                                                               const int32 ProjectileID)
{
	FPredictedTickProjectiles* Projectiles = PredictedProjectiles.Find(Tick);
	if (Projectiles)
	{
		APredictableProjectile* Projectile = Projectiles->Projectiles.FindRef(ProjectileID);
		if (IsValid(Projectile))
		{
			Projectile->Destroy();
		}
		Projectiles->Projectiles.Remove(ProjectileID);
		if (Projectiles->Projectiles.Num() == 0)
		{
			PredictedProjectiles.Remove(Tick);
		}
	}
}

void ASaiyoraPlayerCharacter::ReplaceProjectile(APredictableProjectile* AuthProjectile)
{
	if (!IsValid(AuthProjectile) || AuthProjectile->IsFake())
	{
		return;
	}
	FPredictedTickProjectiles* TickProjectiles = PredictedProjectiles.Find(AuthProjectile->GetSourceInfo().SourceTick);
	if (TickProjectiles)
	{
		APredictableProjectile* FakeProjectile = TickProjectiles->Projectiles.FindRef(AuthProjectile->GetSourceInfo().ID);
		if (IsValid(FakeProjectile))
		{
			const bool bWasLocallyDestroyed = FakeProjectile->Replace();
			AuthProjectile->UpdateLocallyDestroyed(bWasLocallyDestroyed);
		}
		TickProjectiles->Projectiles.Remove(AuthProjectile->GetSourceInfo().ID);
		if (TickProjectiles->Projectiles.Num() == 0)
		{
			PredictedProjectiles.Remove(AuthProjectile->GetSourceInfo().SourceTick);
		}
	}
}

#pragma endregion 
#pragma region Specialization

void ASaiyoraPlayerCharacter::SetAncientSpecialization(const TSubclassOf<UAncientSpecialization> NewSpec)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	UAncientSpecialization* PreviousSpec = AncientSpec;
	if (IsValid(AncientSpec))
	{
		if (AncientSpec->GetClass() == NewSpec)
		{
			return;
		}
		AncientSpec->UnlearnSpec();
		RecentlyUnlearnedAncientSpec = AncientSpec;
		FTimerHandle CleanupHandle;
		GetWorld()->GetTimerManager().SetTimer(CleanupHandle, this, &ASaiyoraPlayerCharacter::CleanupOldAncientSpecialization, 1.0f);
		AncientSpec = nullptr;
	}
	if (IsValid(NewSpec))
	{
		AncientSpec = NewObject<UAncientSpecialization>(this, NewSpec);
		if (IsValid(AncientSpec))
		{
			AncientSpec->InitializeSpecialization(this);
		}
	}
	OnAncientSpecChanged.Broadcast(PreviousSpec, AncientSpec);
}

void ASaiyoraPlayerCharacter::OnRep_AncientSpec(UAncientSpecialization* PreviousSpec)
{
	if (AncientSpec != PreviousSpec && IsValid(PreviousSpec))
	{
		PreviousSpec->UnlearnSpec();
	}
	if (IsValid(AncientSpec))
	{
		AncientSpec->InitializeSpecialization(this);
	}
	OnAncientSpecChanged.Broadcast(PreviousSpec, AncientSpec);
}

#pragma endregion 