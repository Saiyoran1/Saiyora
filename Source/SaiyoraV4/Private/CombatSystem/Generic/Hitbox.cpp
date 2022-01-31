#include "CombatSystem/Generic/Hitbox.h"

#include "DrawDebugHelpers.h"
#include "SaiyoraCombatInterface.h"
#include "Faction/FactionComponent.h"
#include "GameFramework/GameStateBase.h"

const int32 UBoxHitbox::SnapshotBufferSize = 50;
const float UBoxHitbox::MaxLagCompTime = .5f;

UBoxHitbox::UBoxHitbox()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

USphereHitbox::USphereHitbox()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;	
}

UCapsuleHitbox::UCapsuleHitbox()
{
	PrimaryComponentTick.bCanEverTick = true;
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

void UBoxHitbox::RewindByTime(float const Ping)
{
	float const Timestamp = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - Ping;
	TransformNoRewind = GetComponentTransform();
	if (Snapshots.Num() == 0)
	{
		return;
	}
	int32 AfterIndex = -1;
	for (int i = 0; i < Snapshots.Num(); i++)
	{
		if (Timestamp <= Snapshots[i].Key)
		{
			AfterIndex = i;
			break;
		}
	}
	if (!Snapshots.IsValidIndex(AfterIndex))
	{
		return;
	}
	if (AfterIndex == 0)
	{
		//Timestamp before snapshot buffer, use max lag compensation.
		SetWorldTransform(Snapshots[0].Value);
	}
	else
	{
		float const SnapshotGap = Snapshots[AfterIndex].Key - Snapshots[AfterIndex - 1].Key;
		float const SnapshotFraction = (Timestamp - Snapshots[AfterIndex - 1].Key) / SnapshotGap;
		FVector const LocDiff = Snapshots[AfterIndex].Value.GetLocation() - Snapshots[AfterIndex - 1].Value.GetLocation();
		FVector const InterpVector = LocDiff * SnapshotFraction;
		SetWorldLocation(Snapshots[AfterIndex - 1].Value.GetLocation() + InterpVector);
		FRotator const RotDiff = Snapshots[AfterIndex].Value.Rotator() - Snapshots[AfterIndex - 1].Value.Rotator();
		FRotator const InterpRotator = RotDiff * SnapshotFraction;
		SetWorldRotation(Snapshots[AfterIndex - 1].Value.Rotator() + InterpRotator);
		SetWorldScale3D(SnapshotFraction <= 0.5f ? Snapshots[AfterIndex - 1].Value.GetScale3D() : Snapshots[AfterIndex].Value.GetScale3D());
	}
}

void UBoxHitbox::Unrewind()
{
	SetWorldTransform(TransformNoRewind);
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

void UBoxHitbox::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwnerRole() == ROLE_Authority)
	{
		TTuple<float, FTransform> Snapshot;
		Snapshot.Key = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		Snapshot.Value = GetComponentTransform();
		Snapshots.Add(Snapshot);
		int32 const NumToRemove = Snapshots.Num() - SnapshotBufferSize;
		if (NumToRemove > 0)
		{
			Snapshots.RemoveAt(0, NumToRemove);
		}
		if (Snapshots.Num() > 10)
		{
			for (int i = 0; i < 10; i++)
			{
				DrawDebugBox(GetWorld(), Snapshots[Snapshots.Num() - (i + 1)].Value.GetLocation(), Bounds.BoxExtent, FColor::Green, false, 2.0f);
			}
		}
	}
}

void USphereHitbox::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwnerRole() == ROLE_Authority)
	{
		TTuple<float, FTransform> Snapshot;
		Snapshot.Key = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		Snapshot.Value = GetComponentTransform();
		Snapshots.Add(Snapshot);
		int32 const NumToRemove = Snapshots.Num() - UBoxHitbox::SnapshotBufferSize;
		if (NumToRemove > 0)
		{
			Snapshots.RemoveAt(0, NumToRemove);
		}
	}
}

void UCapsuleHitbox::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwnerRole() == ROLE_Authority)
	{
		TTuple<float, FTransform> Snapshot;
		Snapshot.Key = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		Snapshot.Value = GetComponentTransform();
		Snapshots.Add(Snapshot);
		int32 const NumToRemove = Snapshots.Num() - UBoxHitbox::SnapshotBufferSize;
		if (NumToRemove > 0)
		{
			Snapshots.RemoveAt(0, NumToRemove);
		}
	}
}