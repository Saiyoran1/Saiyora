#include "CombatStatusComponent.h"
#include "AbilityFunctionLibrary.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "SaiyoraGameState.h"
#include "SaiyoraPlayerCharacter.h"
#include "UnrealNetwork.h"
#include "WidgetComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "FloatingName.h"

TMap<int32, UCombatStatusComponent*> UCombatStatusComponent::StencilValues = TMap<int32, UCombatStatusComponent*>();

#pragma region Setup

UCombatStatusComponent::UCombatStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
	SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
	SetRenderCustomDepth(false);
}

void UCombatStatusComponent::GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatStatusComponent, PlaneStatus);
	DOREPLIFETIME_CONDITION(UCombatStatusComponent, bPlaneSwapRestricted, COND_OwnerOnly);
	DOREPLIFETIME(UCombatStatusComponent, CombatName);
}

void UCombatStatusComponent::InitializeComponent()
{
	PlaneStatus.CurrentPlane = DefaultPlane;
	PlaneStatus.LastSwapSource = nullptr;
}

void UCombatStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()), TEXT("Owner does not implement combat interface, but has Plane Component."));
	const ASaiyoraPlayerCharacter* LocalPlayer = USaiyoraCombatLibrary::GetLocalSaiyoraPlayer(this);
	if (IsValid(LocalPlayer))
	{
		LocalPlayerStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(LocalPlayer);
		if (IsValid(LocalPlayerStatusComponent))
		{
			LocalPlayerStatusComponent->OnPlaneSwapped.AddDynamic(this, &UCombatStatusComponent::OnLocalPlayerPlaneSwap);
		}
		SetupNameWidget(LocalPlayer);
	}
	else
	{
		GameStateRef = Cast<ASaiyoraGameState>(GetWorld()->GetGameState());
		if (!IsValid(GameStateRef))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid game state ref in Combat Status Component BeginPlay."));
		}
		else
		{
			GameStateRef->OnPlayerAdded.AddDynamic(this, &UCombatStatusComponent::OnPlayerAdded);
		}
	}
	UpdateOwnerCustomRendering();
	UpdateOwnerPlaneCollision();
}

void UCombatStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsValid(LocalPlayerCamera))
	{
		SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), GetComponentLocation() - LocalPlayerCamera->GetForwardVector()));
	}
}

void UCombatStatusComponent::OnPlayerAdded(const ASaiyoraPlayerCharacter* NewPlayer)
{
	if (IsValid(NewPlayer) && NewPlayer->IsLocallyControlled())
	{
		LocalPlayerStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(NewPlayer);
		if (IsValid(LocalPlayerStatusComponent))
		{
			LocalPlayerStatusComponent->OnPlaneSwapped.AddDynamic(this, &UCombatStatusComponent::OnLocalPlayerPlaneSwap);
			UpdateOwnerCustomRendering();
		}
		GameStateRef->OnPlayerAdded.RemoveDynamic(this, &UCombatStatusComponent::OnPlayerAdded);
		SetupNameWidget(NewPlayer);
	}
}

#pragma endregion
#pragma region Name

void UCombatStatusComponent::SetCombatName(const FName NewName)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const FName Previous = CombatName;
	CombatName = NewName;
	OnNameChanged.Broadcast(Previous, CombatName);
}

void UCombatStatusComponent::SetupNameWidget(const ASaiyoraPlayerCharacter* LocalPlayer)
{
	LocalPlayerCamera = LocalPlayer->Camera;
	if (IsValid(LocalPlayerCamera) && IsValid(NameWidgetClass))
	{
		UFloatingName* FloatingNameWidget = CreateWidget<UFloatingName>(GetWorld(), NameWidgetClass);
		if (IsValid(FloatingNameWidget))
		{
			SetWidget(FloatingNameWidget);
			SetDrawAtDesiredSize(true);
			SetPivot(FVector2D(0.5f, 0.0f));
			FloatingNameWidget->Init(this);
		}
		FName SocketName = NAME_None;
		USceneComponent* SceneComponent = ISaiyoraCombatInterface::Execute_GetFloatingHealthSocket(GetOwner(), SocketName);
		if (IsValid(SceneComponent))
		{
			const FAttachmentTransformRules TransformRules = FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false);
			AttachToComponent(SceneComponent, TransformRules, SocketName);
		}
	}
}

#pragma endregion
#pragma region Plane

ESaiyoraPlane UCombatStatusComponent::PlaneSwap(const bool bIgnoreRestrictions, UObject* Source,
                                                const bool bToSpecificPlane, const ESaiyoraPlane TargetPlane)
{
	if (GetOwnerRole() != ROLE_Authority || !bCanEverPlaneSwap)
	{
		return PlaneStatus.CurrentPlane;
	}
	if (!bIgnoreRestrictions && IsPlaneSwapRestricted())
	{
		return PlaneStatus.CurrentPlane;
	}
	const ESaiyoraPlane PreviousPlane = PlaneStatus.CurrentPlane;
	if (bToSpecificPlane)
	{
		PlaneStatus.CurrentPlane = TargetPlane;
	}
	else
	{
		switch (PlaneStatus.CurrentPlane)
		{
			case ESaiyoraPlane::Ancient :
				PlaneStatus.CurrentPlane = ESaiyoraPlane::Modern;
				break;
			case ESaiyoraPlane::Modern :
				PlaneStatus.CurrentPlane = ESaiyoraPlane::Ancient;
				break;
			case ESaiyoraPlane::Both :
				PlaneStatus.CurrentPlane = ESaiyoraPlane::Neither;
				break;
			case ESaiyoraPlane::Neither :
				PlaneStatus.CurrentPlane = ESaiyoraPlane::Both;
				break;
			default :
				return PlaneStatus.CurrentPlane;
		}
	}
	PlaneStatus.LastSwapSource = Source;
	if (PreviousPlane != PlaneStatus.CurrentPlane)
	{
		OnPlaneSwapped.Broadcast(PreviousPlane, PlaneStatus.CurrentPlane, Source);
		UpdateOwnerCustomRendering();
		UpdateOwnerPlaneCollision();
	}
	return PlaneStatus.CurrentPlane;
}

void UCombatStatusComponent::AddPlaneSwapRestriction(UObject* Source)
{
	const bool bPreviouslyRestricted = bPlaneSwapRestricted;
	PlaneSwapRestrictions.Add(Source);
	if (!bPreviouslyRestricted && PlaneSwapRestrictions.Num() > 0)
	{
		bPlaneSwapRestricted = true;
		OnPlaneSwapRestrictionChanged.Broadcast(bPlaneSwapRestricted);
	}
}

void UCombatStatusComponent::RemovePlaneSwapRestriction(UObject* Source)
{
	const bool bPreviouslyRestricted = bPlaneSwapRestricted;
	PlaneSwapRestrictions.Remove(Source);
	if (bPreviouslyRestricted && PlaneSwapRestrictions.Num() == 0)
	{
		bPlaneSwapRestricted = false;
		OnPlaneSwapRestrictionChanged.Broadcast(bPlaneSwapRestricted);
	}
}

void UCombatStatusComponent::OnRep_PlaneStatus(const FPlaneStatus& PreviousStatus)
{
	if (PreviousStatus.CurrentPlane != PlaneStatus.CurrentPlane)
	{
		OnPlaneSwapped.Broadcast(PreviousStatus.CurrentPlane, PlaneStatus.CurrentPlane, PlaneStatus.LastSwapSource);
		UpdateOwnerCustomRendering();
		UpdateOwnerPlaneCollision();
	}
}

void UCombatStatusComponent::UpdateOwnerCustomRendering()
{
	UpdateStencilValue();
	TArray<UMeshComponent*> Meshes;
	GetOwner()->GetComponents<UMeshComponent>(Meshes);
	Meshes.Remove(this);
	TArray<AActor*> AttachedActors;
	GetOwner()->GetAttachedActors(AttachedActors, false, true);
	for (AActor* AttachedActor : AttachedActors)
	{
		TArray<UMeshComponent*> AttachedMeshes;
		AttachedActor->GetComponents<UMeshComponent>(AttachedMeshes);
		Meshes.Append(AttachedMeshes);
	}
	for (UMeshComponent* Mesh : Meshes)
	{
		if (IsValid(Mesh))
		{
			Mesh->SetRenderCustomDepth(bUseCustomDepth);
			if (bUseCustomDepth)
			{
				Mesh->SetCustomDepthStencilValue(StencilValue);
			}
		}
	}
}

void UCombatStatusComponent::UpdateStencilValue()
{
	const int32 PreviousStencil = StencilValue;
	const bool bPreviouslyUsingCustomDepth = bUseCustomDepth;
	const bool bPreviouslyUsingDefaultID = bUsingDefaultID;
	if (!IsValid(LocalPlayerStatusComponent) || LocalPlayerStatusComponent == this)
	{
		StencilValue = DefaultStencil;
		bUseCustomDepth = false;
		bUsingDefaultID = true;
	}
	else
	{
		bUseCustomDepth = true;
		
		int32 RangeStart = DefaultStencil;
		int32 RangeEnd = DefaultStencil;
		int32 DefaultID = DefaultStencil;
		const bool bIsXPlane = UAbilityFunctionLibrary::IsXPlane(LocalPlayerStatusComponent->GetCurrentPlane(), GetCurrentPlane());
		switch (GetCurrentFaction())
		{
		case EFaction::Friendly :
			RangeStart = bIsXPlane ? FriendlyXPlaneStart : FriendlySamePlaneStart;
			RangeEnd = bIsXPlane ? FriendlyXPlaneEnd : FriendlySamePlaneEnd;
			DefaultID = bIsXPlane?  DefaultFriendlyXPlane : DefaultFriendlySamePlane;
			break;
		case EFaction::Neutral :
			RangeStart = bIsXPlane ? NeutralXPlaneStart : NeutralSamePlaneStart;
			RangeEnd = bIsXPlane ? NeutralXPlaneEnd : NeutralSamePlaneEnd;
			DefaultID = bIsXPlane ? DefaultNeutralXPlane : DefaultNeutralSamePlane;
			break;
		case EFaction::Enemy :
			RangeStart = bIsXPlane ? EnemyXPlaneStart : EnemySamePlaneStart;
			RangeEnd = bIsXPlane ? EnemyXPlaneEnd : EnemySamePlaneEnd;
			DefaultID = bIsXPlane ? DefaultEnemyXPlane : DefaultEnemySamePlane;
			break;
		default :
			break;
		}
		if (StencilValue < RangeStart || StencilValue > RangeEnd)
		{
			bool bFoundNewID = false;
			for (int32 i = RangeStart; i <= RangeEnd; i++)
			{
				if (!StencilValues.Contains(i))
				{
					StencilValues.Add(i, this);
					StencilValue = i;
					bFoundNewID = true;
					bUsingDefaultID = false;
					break;
				}
			}
			if (!bFoundNewID)
			{
				StencilValue = DefaultID;
				bUsingDefaultID = true;
			}
		}
	}
	if (PreviousStencil != StencilValue && bPreviouslyUsingCustomDepth && StencilValues.Contains(PreviousStencil) && !bPreviouslyUsingDefaultID)
	{
		StencilValues.Remove(PreviousStencil);
	}
}

void UCombatStatusComponent::UpdateOwnerPlaneCollision()
{
	TArray<UPrimitiveComponent*> Primitives;
	GetOwner()->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Component : Primitives)
	{
		if (Component->GetCollisionObjectType() == ECC_Pawn)
		{
			switch (GetCurrentPlane())
			{
			case ESaiyoraPlane::Ancient :
				Component->SetCollisionProfileName(FSaiyoraCollision::P_AncientPawn);
				break;
			case ESaiyoraPlane::Modern :
				Component->SetCollisionProfileName(FSaiyoraCollision::P_ModernPawn);
				break;
			default :
				Component->SetCollisionProfileName(FSaiyoraCollision::P_Pawn);
			}
		}
	}
}

#pragma endregion