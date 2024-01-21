#include "CoreClasses/SaiyoraPlayerCharacter.h"
#include "AbilityFunctionLibrary.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "CombatStatusComponent.h"
#include "DungeonGameState.h"
#include "IMessageTracer.h"
#include "ModernSpecialization.h"
#include "Weapons/FireWeapon.h"
#include "PredictableProjectile.h"
#include "ResourceHandler.h"
#include "SaiyoraErrorMessage.h"
#include "SaiyoraMovementComponent.h"
#include "SaiyoraUIDataAsset.h"
#include "CoreClasses/SaiyoraPlayerController.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "CoreClasses/SaiyoraGameState.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Specialization/AncientSpecialization.h"
#include "Weapons/Reload.h"
#include "Weapons/StopFiring.h"
#include "Weapons/Weapon.h"

#pragma region Initialization

ASaiyoraPlayerCharacter::ASaiyoraPlayerCharacter(const class FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<USaiyoraMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	SpringArm->TargetArmLength = 500.0f;
	SpringArm->ProbeSize = 15.0f;
	SpringArm->ProbeChannel = ECC_Camera;
	SpringArm->bDoCollisionTest = true;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraRotationLag = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 20.0f;
	SpringArm->CameraLagMaxDistance = 100.0f;
	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 30.0f);
	SpringArm->TargetOffset = FVector::ZeroVector;
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->FieldOfView = 100.0f;
	Camera->bUsePawnControlRotation = false;
	
	CombatStatusComponent = CreateDefaultSubobject<UCombatStatusComponent>(TEXT("CombatStatusComponent"));
	DamageHandler = CreateDefaultSubobject<UDamageHandler>(TEXT("DamageHandler"));
	ThreatHandler = CreateDefaultSubobject<UThreatHandler>(TEXT("ThreatHandler"));
	BuffHandler = CreateDefaultSubobject<UBuffHandler>(TEXT("BuffHandler"));
	StatHandler = CreateDefaultSubobject<UStatHandler>(TEXT("StatHandler"));
	CcHandler = CreateDefaultSubobject<UCrowdControlHandler>(TEXT("CcHandler"));
	AbilityComponent = CreateDefaultSubobject<UAbilityComponent>(TEXT("AbilityComponent"));
	ResourceHandler = CreateDefaultSubobject<UResourceHandler>(TEXT("ResourceHandler"));
}

void ASaiyoraPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!IsValid(PlayerInputComponent))
	{
		return;
	}
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputJump);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputStartCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASaiyoraPlayerCharacter::InputStopCrouch);
	PlayerInputComponent->BindAxis("MoveForward", this, &ASaiyoraPlayerCharacter::InputMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASaiyoraPlayerCharacter::InputMoveRight);
	PlayerInputComponent->BindAxis("LookHorizontal", this, &ASaiyoraPlayerCharacter::InputLookHorizontal);
	PlayerInputComponent->BindAxis("LookVertical", this, &ASaiyoraPlayerCharacter::InputLookVertical);

	PlayerInputComponent->BindAction("PlaneSwap", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputPlaneSwap);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputReload);

	PlayerInputComponent->BindAction("Action0", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputStartAbility0);
	PlayerInputComponent->BindAction("Action0", IE_Released, this, &ASaiyoraPlayerCharacter::InputStopAbility0);
	PlayerInputComponent->BindAction("Action1", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputStartAbility1);
	PlayerInputComponent->BindAction("Action1", IE_Released, this, &ASaiyoraPlayerCharacter::InputStopAbility1);
	PlayerInputComponent->BindAction("Action2", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputStartAbility2);
	PlayerInputComponent->BindAction("Action2", IE_Released, this, &ASaiyoraPlayerCharacter::InputStopAbility2);
	PlayerInputComponent->BindAction("Action3", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputStartAbility3);
	PlayerInputComponent->BindAction("Action3", IE_Released, this, &ASaiyoraPlayerCharacter::InputStopAbility3);
	PlayerInputComponent->BindAction("Action4", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputStartAbility4);
	PlayerInputComponent->BindAction("Action4", IE_Released, this, &ASaiyoraPlayerCharacter::InputStopAbility4);
	PlayerInputComponent->BindAction("Action5", IE_Pressed, this, &ASaiyoraPlayerCharacter::InputStartAbility5);
	PlayerInputComponent->BindAction("Action5", IE_Released, this, &ASaiyoraPlayerCharacter::InputStopAbility5);
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
	DOREPLIFETIME(ASaiyoraPlayerCharacter, ModernSpec);
	DOREPLIFETIME(ASaiyoraPlayerCharacter, PendingResurrection);
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
		bWroteSomething |= Channel->ReplicateSubobject(RecentlyUnlearnedAncientSpec, *Bunch, *RepFlags);
	}
	if (IsValid(ModernSpec))
	{
		bWroteSomething |= Channel->ReplicateSubobject(ModernSpec, *Bunch, *RepFlags);
	}
	if (IsValid(RecentlyUnlearnedModernSpec))
	{
		bWroteSomething |= Channel->ReplicateSubobject(RecentlyUnlearnedModernSpec, *Bunch, *RepFlags);
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
#pragma region Input

void ASaiyoraPlayerCharacter::Server_PlaneSwapInput_Implementation()
{
	if (IsValid(CombatStatusComponent))
	{
		CombatStatusComponent->PlaneSwap(false, this, false);
	}
}

void ASaiyoraPlayerCharacter::InputReload()
{
	if (!IsValid(AbilityComponent) || !IsValid(ReloadAbility))
	{
		return;
	}
	AbilityComponent->UseAbility(ReloadAbility->GetClass());
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
	AbilityComponent->OnAbilityAdded.AddDynamic(this, &ASaiyoraPlayerCharacter::OnAbilityAdded);
	AbilityComponent->OnAbilityRemoved.AddDynamic(this, &ASaiyoraPlayerCharacter::OnAbilityRemoved);
}

void ASaiyoraPlayerCharacter::SetAbilityMapping(const ESaiyoraPlane Plane, const int32 BindIndex, const TSubclassOf<UCombatAbility> AbilityClass)
{
	if (!IsLocallyControlled() || BindIndex < 0 || BindIndex > MaxAbilityBinds - 1)
	{
		return;
	}
	switch (Plane)
	{
	case ESaiyoraPlane::Ancient :
		{
			AncientAbilityMappings.Add(BindIndex, AbilityClass);
			OnMappingChanged.Broadcast(ESaiyoraPlane::Ancient, BindIndex, AbilityClass);
		}
		break;
	case ESaiyoraPlane::Modern :
		{
			ModernAbilityMappings.Add(BindIndex, AbilityClass);
			OnMappingChanged.Broadcast(ESaiyoraPlane::Modern, BindIndex, AbilityClass);
		}
		break;
	default :
		break;
	}
}

void ASaiyoraPlayerCharacter::OnAbilityAdded(UCombatAbility* NewAbility)
{
	//When we get a new FireWeapon, Reload, or StopFiring ability, save those off.
	if (NewAbility->HasTag(FSaiyoraCombatTags::Get().FireWeaponAbility))
	{
		FireWeaponAbility = Cast<UFireWeapon>(NewAbility);
	}
	else if (NewAbility->HasTag(FSaiyoraCombatTags::Get().ReloadAbility))
	{
		ReloadAbility = Cast<UReload>(NewAbility);
	}
	else if (NewAbility->HasTag(FSaiyoraCombatTags::Get().StopFireAbility))
	{
		StopFiringAbility = Cast<UStopFiring>(NewAbility);
	}
}

void ASaiyoraPlayerCharacter::OnAbilityRemoved(UCombatAbility* RemovedAbility)
{
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
	const TSubclassOf<UCombatAbility> AbilityClass = CombatStatusComponent->GetCurrentPlane() == ESaiyoraPlane::Ancient ? AncientAbilityMappings.FindRef(InputNum) : ModernAbilityMappings.FindRef(InputNum);
	UCombatAbility* AbilityToUse = AbilityComponent->FindActiveAbility(AbilityClass);
	if (!IsValid(AbilityToUse))
	{
		return;
	}
	if (bPressed)
	{
		//If we are firing and we press a new ability that isn't fire weapon, stop firing first.
		if (IsValid(FireWeaponAbility) && AbilityToUse != FireWeaponAbility && IsValid(Weapon) && Weapon->IsBurstFiring() && IsValid(StopFiringAbility))
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
		else if (IsValid(FireWeaponAbility) && AbilityToUse == FireWeaponAbility && InitialAttempt.ActionTaken == ECastAction::Fail)
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
		if (IsValid(FireWeaponAbility) && AbilityToUse == FireWeaponAbility && IsValid(Weapon) && Weapon->IsBurstFiring() && IsValid(StopFiringAbility))
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
		if (IsValid(FireWeaponAbility) && AutomaticInputAbility == FireWeaponAbility && Event.ActionTaken == ECastAction::Fail)
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
	if (IsValid(FireWeaponAbility) && AbilityClass == FireWeaponAbility->GetClass() && Event.ActionTaken == ECastAction::Fail)
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
	if (UAbilityFunctionLibrary::IsXPlane(CombatStatusComponent->GetCurrentPlane(), OtherCompPlane))
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

int32 ASaiyoraPlayerCharacter::GetNewProjectileID(const FPredictedTick& Tick)
{
	int32 ID = 0;
	int32* IDPtr = ProjectileIDs.Find(Tick);
	if (!IDPtr)
	{
		ProjectileIDs.Add(Tick, 1);
	}
	else
	{
		ID = *IDPtr;
		(*IDPtr)++;
	}
	return ID;
}

#pragma endregion 
#pragma region Specialization

void ASaiyoraPlayerCharacter::ApplyNewAncientLayout(const FAncientSpecLayout& NewLayout)
{
	if (!IsLocallyControlled())
	{
		return;
	}
	//Check if we are in a dungeon, and if so, if we are in the "waiting to start" phase.
	const ADungeonGameState* DungeonGameState = Cast<ADungeonGameState>(GameStateRef);
	if (IsValid(DungeonGameState) && DungeonGameState->GetDungeonPhase() != EDungeonPhase::WaitingToStart)
	{
		//If the dungeon has already started or finished, we can't change talents.
		return;
	}
	TArray<FAncientTalentSelection> Selections;
	for (const TTuple<int32, FAncientTalentChoice>& Choice : NewLayout.Talents)
	{
		Selections.Add(FAncientTalentSelection(Choice.Value.TalentRow.BaseAbilityClass, Choice.Value.CurrentSelection));
	}
	Server_UpdateAncientSpecAndTalents(NewLayout.Spec, Selections);
}

void ASaiyoraPlayerCharacter::Server_UpdateAncientSpecAndTalents_Implementation(
	TSubclassOf<UAncientSpecialization> NewSpec, const TArray<FAncientTalentSelection>& TalentSelections)
{
	if (!HasAuthority())
	{
		return;
	}
	//Check if we are in a dungeon, and if so, if we are in the "waiting to start" phase.
	const ADungeonGameState* DungeonGameState = Cast<ADungeonGameState>(GameStateRef);
	if (IsValid(DungeonGameState) && DungeonGameState->GetDungeonPhase() != EDungeonPhase::WaitingToStart)
	{
		//If the dungeon has already started or finished, we can't change talents.
		return;
	}
	//Change specs if the new spec is valid and different from our current one.
	if (IsValid(NewSpec) && (!IsValid(AncientSpec) || AncientSpec->GetClass() != NewSpec))
	{
		UAncientSpecialization* PreviousSpec = AncientSpec;
		//Unlearn the previous ancient spec if we have one.
		if (IsValid(AncientSpec))
		{
			AncientSpec->UnlearnSpec();
			//Leave a reference to the old spec for a second so that the garbage collector doesn't clean it up before clients can run the Unlearn logic.
			RecentlyUnlearnedAncientSpec = AncientSpec;
			FTimerHandle CleanupHandle;
			GetWorld()->GetTimerManager().SetTimer(CleanupHandle, this, &ASaiyoraPlayerCharacter::CleanupOldAncientSpecialization, 1.0f);
			AncientSpec = nullptr;
		}
		//Learn the new spec.
		AncientSpec = NewObject<UAncientSpecialization>(this, NewSpec);
		if (IsValid(AncientSpec))
		{
			AncientSpec->InitializeSpecialization(this);
		}
		OnAncientSpecChanged.Broadcast(PreviousSpec, AncientSpec);
	}
	//Change talent choices.
	if (IsValid(AncientSpec))
	{
		for (const FAncientTalentSelection& TalentSelection : TalentSelections)
		{
			AncientSpec->SelectAncientTalent(TalentSelection);
		}
	}
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

void ASaiyoraPlayerCharacter::ApplyNewModernLayout(const FModernSpecLayout& NewLayout)
{
	if (!IsLocallyControlled())
	{
		return;
	}
	//Check if we are in a dungeon, and if so, if we are in the "waiting to start" phase.
	const ADungeonGameState* DungeonGameState = Cast<ADungeonGameState>(GameStateRef);
	if (IsValid(DungeonGameState) && DungeonGameState->GetDungeonPhase() != EDungeonPhase::WaitingToStart)
	{
		//If the dungeon has already started or finished, we can't change talents.
		return;
	}
	TArray<FModernTalentChoice> TalentChoices;
	NewLayout.Talents.GenerateValueArray(TalentChoices);
	Server_UpdateModernSpecAndTalents(NewLayout.Spec, TalentChoices);
}

void ASaiyoraPlayerCharacter::Server_UpdateModernSpecAndTalents_Implementation(
	TSubclassOf<UModernSpecialization> NewSpec, const TArray<FModernTalentChoice>& TalentSelections)
{
	if (!HasAuthority())
	{
		return;
	}
	//Check if we are in a dungeon, and if so, if we are in the "waiting to start" phase.
	const ADungeonGameState* DungeonGameState = Cast<ADungeonGameState>(GameStateRef);
	if (IsValid(DungeonGameState) && DungeonGameState->GetDungeonPhase() != EDungeonPhase::WaitingToStart)
	{
		//If the dungeon has already started or finished, we can't change talents.
		return;
	}
	//Change specs if the new spec is valid and different from our current one.
	if (IsValid(NewSpec) && (!IsValid(ModernSpec) || ModernSpec->GetClass() != NewSpec))
	{
		UModernSpecialization* PreviousSpec = ModernSpec;
		//Unlearn the previous modern spec if we have one.
		if (IsValid(ModernSpec))
		{
			ModernSpec->UnlearnSpec();
			//Leave a reference to the old spec for a second so that garbage collection doesn't clean it up before clients can run the Unlearn logic.
			RecentlyUnlearnedModernSpec = ModernSpec;
			FTimerHandle CleanupHandle;
			GetWorld()->GetTimerManager().SetTimer(CleanupHandle, this, &ASaiyoraPlayerCharacter::CleanupOldModernSpecialization, 1.0f);
			ModernSpec = nullptr;
		}
		//Learn the new spec.
		ModernSpec = NewObject<UModernSpecialization>(this, NewSpec);
		if (IsValid(ModernSpec))
		{
			ModernSpec->InitializeSpecialization(this);
		}
		OnModernSpecChanged.Broadcast(PreviousSpec, ModernSpec);
	}
	//Change talent selections.
	if (IsValid(ModernSpec))
	{
		ModernSpec->SelectModernAbilities(TalentSelections);
	}
}

void ASaiyoraPlayerCharacter::OnRep_ModernSpec(UModernSpecialization* PreviousSpec)
{
	if (ModernSpec != PreviousSpec && IsValid(PreviousSpec))
	{
		PreviousSpec->UnlearnSpec();
	}
	if (IsValid(ModernSpec))
	{
		ModernSpec->InitializeSpecialization(this);
	}
	OnModernSpecChanged.Broadcast(PreviousSpec, ModernSpec);
}

#pragma endregion
#pragma region User Interface

void ASaiyoraPlayerCharacter::Client_DisplayErrorMessage_Implementation(const FText& Message, const float Duration)
{
	if (!IsLocallyControlled() || !bInitialized || Message.IsEmpty())
	{
		return;
	}
	if (!IsValid(UIDataAsset) || !IsValid(UIDataAsset->ErrorMessageWidgetClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("Tried to display an error message for a player, but the UI Data Asset or Error Widget class was null."));
		return;
	}
	if (IsValid(ErrorWidget))
	{
		ErrorWidget->RemoveFromParent();
		ErrorWidget = nullptr;
	}
	ErrorWidget = CreateWidget<USaiyoraErrorMessage>(GetSaiyoraPlayerController(), UIDataAsset->ErrorMessageWidgetClass);
	if (IsValid(ErrorWidget))
	{
		ErrorWidget->SetErrorMessage(Message, Duration);
		//TODO: Probably want to add this to a certain location, or have the player UI handle this?
		ErrorWidget->AddToViewport();
	}
}

#pragma endregion
#pragma region Resurrection

void ASaiyoraPlayerCharacter::OfferResurrection(UBuff* Source, const FVector& ResLocation,
	const FResDecisionCallback& DecisionCallback)
{
	if (!HasAuthority() || PendingResurrection.bResAvailable || !IsValid(Source) || !DecisionCallback.IsBound())
	{
		return;
	}
	PendingResurrection.bResAvailable = true;
	PendingResurrection.ResSource = Source;
	PendingResurrection.DecisionCallback = DecisionCallback;
	PendingResurrection.ResLocation = ResLocation;

	OnRep_PendingResurrection();
}

void ASaiyoraPlayerCharacter::RescindResurrection(const UBuff* Source)
{
	if (!HasAuthority() || !PendingResurrection.bResAvailable || !IsValid(Source) || Source != PendingResurrection.ResSource)
	{
		return;
	}
	PendingResurrection.Clear();

	OnRep_PendingResurrection();
}

void ASaiyoraPlayerCharacter::Server_MakeResDecision_Implementation(const bool bAccepted)
{
	if (!HasAuthority())
	{
		return;
	}
	if (PendingResurrection.bResAvailable && PendingResurrection.DecisionCallback.IsBound())
	{
		PendingResurrection.DecisionCallback.Execute(bAccepted);
	}
	else if (bAccepted)
	{
		DamageHandler->RespawnActor();
	}
}

#pragma endregion 