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
#include "ThreatHandler.h"

TMap<int32, UCombatStatusComponent*> UCombatStatusComponent::StencilValues = TMap<int32, UCombatStatusComponent*>();

#pragma region Setup

UCombatStatusComponent::UCombatStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
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
	Super::InitializeComponent();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
	PlaneStatus.CurrentPlane = DefaultPlane;
	PlaneStatus.LastSwapSource = nullptr;
	ThreatHandlerRef = ISaiyoraCombatInterface::Execute_GetThreatHandler(GetOwner());
}

void UCombatStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->Implements<USaiyoraCombatInterface>(), TEXT("Owner does not implement combat interface, but has Plane Component."));

	//Check to see if the local player is already valid.
	const ASaiyoraPlayerCharacter* LocalPlayer = USaiyoraCombatLibrary::GetLocalSaiyoraPlayer(this);
	if (IsValid(LocalPlayer))
	{
		//If the local player is valid, we setup our name widget to face correctly toward his camera, and bind a function to update rendering when he swaps planes.
		LocalPlayerStatusComponent = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(LocalPlayer);
		if (IsValid(LocalPlayerStatusComponent))
		{
			LocalPlayerStatusComponent->OnPlaneSwapped.AddDynamic(this, &UCombatStatusComponent::OnLocalPlayerPlaneSwap);
		}
		SetupNameWidget(LocalPlayer);
	}
	else
	{
		//If the local player isn't valid, bind to the GameState to get callbacks when players are added, until we get a valid local player.
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
	//Initial setting of outline and plane material and collision profile.
	UpdateOwnerCustomRendering();
	UpdateOwnerPlaneCollision();

	//Setup binding for showing/hiding combat name when entering combat as an NPC.
	if (GetCurrentFaction() != EFaction::Friendly && IsValid(ThreatHandlerRef))
	{
		ThreatHandlerRef->OnCombatChanged.AddDynamic(this, &UCombatStatusComponent::OnCombatChanged);
	}
}

void UCombatStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Face the floating name widget to the local player's camera on Tick.
	if (IsValid(LocalPlayerCamera))
	{
		SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), GetComponentLocation() - LocalPlayerCamera->GetForwardVector()));
	}
}

void UCombatStatusComponent::OnPlayerAdded(ASaiyoraPlayerCharacter* NewPlayer)
{
	//If we get a valid local player, we can set up the name widget to face his camera and bind to plane swapping to update the outline and plane material.
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
	OnRep_CombatName(Previous);
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

void UCombatStatusComponent::OnCombatChanged(UThreatHandler* Combatant, const bool bNewCombat)
{
	//Hide the name widget while in combat, show it outside of combat.
	if (!IsValid(GetWidget()))
	{
		return;
	}
	if (bNewCombat)
	{
		GetWidget()->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		GetWidget()->SetVisibility(ESlateVisibility::HitTestInvisible);
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
	const FPlaneStatus PreviousStatus = PlaneStatus;
	//If the swap is to a specific plane, go to that plane.
	if (bToSpecificPlane)
	{
		PlaneStatus.CurrentPlane = TargetPlane;
	}
	//Otherwise, we just go to the "opposite" of our current plane.
	//This logic may not be desirable for Both/Neither, but we'll go with it for now.
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
	OnRep_PlaneStatus(PreviousStatus);
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

void UCombatStatusComponent::UpdateOwnerPlaneCollision()
{
	//This function specifically looks for the "Pawn" object type.
	//This is intended to cause collision with level geometry for the actual actor. Hitboxes handle their own collision profiles.
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
#pragma region Rendering

void UCombatStatusComponent::UpdateOwnerCustomRendering()
{
	//This function gets an appropriate stencil value that will correspond to the correct faction and plane relationships with the local player.
	//It then sets all mesh components to use that stencil value and enables custom depth, to make the outline/plane material work.
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
	//If we don't have a local player or don't know our faction or plane relationship to him, don't bother trying to set a good stencil value or make the materials work.
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
		//Find the correct range of values to check from our plane and faction relationship to the local player.
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
		//If our current stencil value is outside the range, we need a new one.
		if (StencilValue < RangeStart || StencilValue > RangeEnd)
		{
			//Check if any IDs are free within the range.
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
			//If no IDs are free within the range, use the default ID for the range.
			//The default ID will work correctly in all cases except where we overlap another actor using the same default ID, in which case outlines won't appear where the two actors overlap.
			//This is a fairly minor problem and would require a large number of actors to all have the same plane and faction relationship to the local player while overlapping to occur.
			//The resulting issue is visually unappealing but not gamebreaking, so this seems like a decent approach rather than trying to find a way to identify different overlapping actors within the material itself.
			if (!bFoundNewID)
			{
				StencilValue = DefaultID;
				bUsingDefaultID = true;
			}
		}
	}
	//Free up the old stencil value if we aren't using it anymore and it wasn't a default ID.
	if (PreviousStencil != StencilValue && bPreviouslyUsingCustomDepth && StencilValues.Contains(PreviousStencil) && !bPreviouslyUsingDefaultID)
	{
		StencilValues.Remove(PreviousStencil);
	}
}

#pragma endregion