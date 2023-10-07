#include "FloatingHealthBarManager.h"
#include "AbilityFunctionLibrary.h"
#include "CombatStatusComponent.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"
#include "Kismet/GameplayStatics.h"

void UFloatingHealthBarManager::NativeConstruct()
{
	Super::NativeConstruct();
	CanvasPanelRef = Cast<UCanvasPanel>(GetParent());
	if (IsValid(GetOwningPlayerPawn()) && GetOwningPlayerPawn()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		LocalPlayer = Cast<ASaiyoraPlayerCharacter>(GetOwningPlayerPawn());
		if (IsValid(LocalPlayer))
		{
			LocalPlayer->OnEnemyCombatChanged.AddDynamic(this, &UFloatingHealthBarManager::OnEnemyCombatChanged);
			LocalPlayerCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(LocalPlayer);
		}
	}
}

void UFloatingHealthBarManager::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	GetOwningPlayer()->GetViewportSize(ViewportX, ViewportY);
	CenterScreen = FVector2D(ViewportX / 2.0f, ViewportY / 2.0f);
	UpdateHealthBarPositions(InDeltaTime);
}

void UFloatingHealthBarManager::OnEnemyCombatChanged(AActor* Combatant, const bool bNewCombat)
{
	if (bNewCombat)
	{
		NewHealthBar(Combatant);
	}
	else
	{
		for (int32 i = FloatingBars.Num() - 1; i >= 0; i--)
		{
			if (FloatingBars[i].Target == Combatant)
			{
				FloatingBars[i].WidgetRef->RemoveFromParent();
				FloatingBars.RemoveAt(i);
				break;
			}
		}
	}
}

FVector2D UFloatingHealthBarManager::GetGridSlotLocation(const FHealthBarGridSlot& GridSlot) const
{
	return FVector2D(CenterScreen.X + (GridSlotSize * GridSlot.X),CenterScreen.Y + (GridSlotSize * GridSlot.Y));
}

EGridSlotOffset UFloatingHealthBarManager::BoolsToGridSlot(const bool bRight, const bool bTop,
	const bool bStartHorizontal)
{
	return bStartHorizontal ? bRight ? EGridSlotOffset::Right : EGridSlotOffset::Left : bTop ? EGridSlotOffset::Top : EGridSlotOffset::Bottom;
}

void UFloatingHealthBarManager::IncrementGridSlot(const bool bClockwise, EGridSlotOffset& GridSlotOffset,
	FHealthBarGridSlot& GridSlot)
{
	switch (GridSlotOffset)
	{
	case EGridSlotOffset::Right :
		GridSlotOffset = bClockwise ? EGridSlotOffset::BottomRight : EGridSlotOffset::TopRight;
		if (bClockwise)
		{
			GridSlot.Y -= 1;
		}
		else
		{
			GridSlot.Y += 1;
		}
		break;
	case EGridSlotOffset::BottomRight :
		GridSlotOffset = bClockwise ? EGridSlotOffset::Bottom : EGridSlotOffset::Right;
		if (bClockwise)
		{
			GridSlot.X -= 1;
		}
		else
		{
			GridSlot.Y += 1;
		}
		break;
	case EGridSlotOffset::Bottom :
		GridSlotOffset = bClockwise ? EGridSlotOffset::BottomLeft : EGridSlotOffset::BottomRight;
		if (bClockwise)
		{
			GridSlot.X -= 1;
		}
		else
		{
			GridSlot.X += 1;
		}
		break;
	case EGridSlotOffset::BottomLeft :
		GridSlotOffset = bClockwise ? EGridSlotOffset::Left : EGridSlotOffset::Bottom;
		if (bClockwise)
		{
			GridSlot.Y += 1;
		}
		else
		{
			GridSlot.X += 1;
		}
		break;
	case EGridSlotOffset::Left :
		GridSlotOffset = bClockwise ? EGridSlotOffset::TopLeft : EGridSlotOffset::BottomLeft;
		if (bClockwise)
		{
			GridSlot.Y += 1;
		}
		else
		{
			GridSlot.Y -= 1;
		}
		break;
	case EGridSlotOffset::TopLeft :
		GridSlotOffset = bClockwise ? EGridSlotOffset::Top : EGridSlotOffset::Left;
		if (bClockwise)
		{
			GridSlot.X += 1;
		}
		else
		{
			GridSlot.Y -= 1;
		}
		break;
	case EGridSlotOffset::Top :
		GridSlotOffset = bClockwise ? EGridSlotOffset::TopRight : EGridSlotOffset::TopLeft;
		if (bClockwise)
		{
			GridSlot.X += 1;
		}
		else
		{
			GridSlot.X -= 1;
		}
		break;
	case EGridSlotOffset::TopRight :
		GridSlotOffset = bClockwise ? EGridSlotOffset::Right : EGridSlotOffset::Top;
		if (bClockwise)
		{
			GridSlot.Y -= 1;
		}
		else
		{
			GridSlot.X -= 1;
		}
		break;
	}
}

void UFloatingHealthBarManager::NewHealthBar(AActor* Target)
{
	FFloatingHealthBarInfo NewHealthBarInfo;
	NewHealthBarInfo.Target = Target;
	if (Target->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		NewHealthBarInfo.TargetComponent = ISaiyoraCombatInterface::Execute_GetFloatingHealthSocket(Target, NewHealthBarInfo.TargetSocket);
	}
	if (!IsValid(NewHealthBarInfo.TargetComponent))
	{
		NewHealthBarInfo.TargetComponent = Target->GetRootComponent();
	}
	NewHealthBarInfo.WidgetRef = CreateWidget<UFloatingHealthBar>(this, IsValid(HealthBarClass) ? HealthBarClass : UFloatingHealthBar::StaticClass());
	NewHealthBarInfo.WidgetRef->AddToViewport();
	NewHealthBarInfo.WidgetRef->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	NewHealthBarInfo.WidgetRef->Init(Target);
	FloatingBars.Add(NewHealthBarInfo);
}

void UFloatingHealthBarManager::UpdateHealthBarPositions(const float DeltaTime)
{
	TArray<FFloatingHealthBarInfo*> BarsWithoutSlots;
	TArray<FHealthBarGridSlot> GridSlotsThisFrame;
	
	for (FFloatingHealthBarInfo& HealthBarInfo : FloatingBars)
	{
		HealthBarInfo.PreviousOffset = HealthBarInfo.bOnScreen ? HealthBarInfo.FinalOffset : FVector2D(0.0f);
		const bool bPreviouslyInCenterGrid = HealthBarInfo.bInCenterGrid;
		const FHealthBarGridSlot PreviousSlot = HealthBarInfo.bOnScreen && bPreviouslyInCenterGrid ? HealthBarInfo.DesiredSlot : FHealthBarGridSlot(0, 0);
		//Updates the root position of the bar, and also whether it is on screen or not.
		UpdateHealthBarRootPosition(HealthBarInfo);
		if (HealthBarInfo.bOnScreen)
		{
			//Adjust the root position to not sit on the very edge of the screen.
			ClampBarRootPosition(HealthBarInfo.RootPosition);
			HealthBarInfo.bInCenterGrid = IsInCenterGrid(HealthBarInfo.RootPosition);
			if (HealthBarInfo.bInCenterGrid)
			{
				if (bPreviouslyInCenterGrid && (GetGridSlotLocation(PreviousSlot) - HealthBarInfo.RootPosition).Length() < GridSlotSize)
				{
					GridSlotsThisFrame.Add(PreviousSlot);
				}
				else
				{
					HealthBarInfo.DesiredSlot = FindDesiredGridSlot(HealthBarInfo.RootPosition);
					BarsWithoutSlots.Add(&HealthBarInfo);
				}
			}
		}
		else
		{
			HealthBarInfo.bInCenterGrid = false;
			HealthBarInfo.DesiredSlot = FHealthBarGridSlot(0, 0);
		}
	}

	//Bars closest to their desired grid slot get priority.
	BarsWithoutSlots.Sort([this](const FFloatingHealthBarInfo& Left, const FFloatingHealthBarInfo& Right)
	{
		return (GetGridSlotLocation(Left.DesiredSlot) - Left.RootPosition).Length() <= (GetGridSlotLocation(Right.DesiredSlot) - Right.RootPosition).Length();
	});

	//Find slots for all the bars that need one, and update their desired slot.
	for (FFloatingHealthBarInfo* HealthBarInfo : BarsWithoutSlots)
	{
		if (GridSlotsThisFrame.Contains(HealthBarInfo->DesiredSlot))
		{
			//Check whether we are looking for right side or left side slots.
			const bool bRightSide = HealthBarInfo->RootPosition.X > GetGridSlotLocation(HealthBarInfo->DesiredSlot).X;
			const bool bTop = HealthBarInfo->RootPosition.Y > GetGridSlotLocation(HealthBarInfo->DesiredSlot).Y;
			//Check if should start looking on the sides or on the top/bottom, depending on which axis the widget is further away from the center.
			const bool bStartHorizontal = FMath::Abs(HealthBarInfo->RootPosition.X - GetGridSlotLocation(HealthBarInfo->DesiredSlot).X) >
				FMath::Abs(HealthBarInfo->RootPosition.Y - GetGridSlotLocation(HealthBarInfo->DesiredSlot).Y);
			//Get which slot to check first.
			EGridSlotOffset Offset = BoolsToGridSlot(bRightSide, bTop, bStartHorizontal);
			const EGridSlotOffset StartingOffset = Offset;
			//Offset the slot integers depending on which slot to check.
			switch (Offset)
			{
			case EGridSlotOffset::Right :
				HealthBarInfo->DesiredSlot.X += 1;
				break;
			case EGridSlotOffset::BottomRight :
				HealthBarInfo->DesiredSlot.X += 1;
				HealthBarInfo->DesiredSlot.Y -= 1;
				break;
			case EGridSlotOffset::Bottom :
				HealthBarInfo->DesiredSlot.Y -= 1;
				break;
			case EGridSlotOffset::BottomLeft :
				HealthBarInfo->DesiredSlot.X -= 1;
				HealthBarInfo->DesiredSlot.Y -= 1;
				break;
			case EGridSlotOffset::Left :
				HealthBarInfo->DesiredSlot.X -= 1;
				break;
			case EGridSlotOffset::TopLeft :
				HealthBarInfo->DesiredSlot.X -= 1;
				HealthBarInfo->DesiredSlot.Y += 1;
				break;
			case EGridSlotOffset::Top :
				HealthBarInfo->DesiredSlot.Y += 1;
				break;
			case EGridSlotOffset::TopRight :
				HealthBarInfo->DesiredSlot.X += 1;
				HealthBarInfo->DesiredSlot.Y += 1;
				break;
			}
			//Determine whether to check further in a clockwise or counterclockwise direction.
			bool bClockwise = false;
			if (bStartHorizontal)
			{
				if (bRightSide)
				{
					bClockwise = !bTop;
				}
				else
				{
					bClockwise = bTop;
				}
			}
			else
			{
				if (bTop)
				{
					bClockwise = bRightSide;
				}
				else
				{
					bClockwise = !bRightSide;
				}
			}
			while (GridSlotsThisFrame.Contains(HealthBarInfo->DesiredSlot))
			{
				IncrementGridSlot(bClockwise, Offset, HealthBarInfo->DesiredSlot);
				//Right now, any health bars beyond 9 that are vying for the same slot will end up overlapping.
				if (StartingOffset == Offset)
				{
					break;
				}
			}
		}
		
		if (GridSlotsThisFrame.Contains(HealthBarInfo->DesiredSlot))
		{
			//Here we check again. If we didn't actually find a viable grid slot that isn't already in use, we just treat the health bar as if it wasn't part of the grid.
			HealthBarInfo->bInCenterGrid = false;
			HealthBarInfo->DesiredSlot = FHealthBarGridSlot(0, 0);
		}
		else
		{
			GridSlotsThisFrame.Add(HealthBarInfo->DesiredSlot);
		}
	}

	//Final loop to set visibility, smooth positioning, and place widgets on the screen.
	for (FFloatingHealthBarInfo& HealthBarInfo : FloatingBars)
	{
		if (!HealthBarInfo.bOnScreen)
		{
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::Hidden);
			continue;
		}
		HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::HitTestInvisible);
		//Calculate the offset of the health bar from its desired position to the slot it should occupy.
		HealthBarInfo.FinalOffset = HealthBarInfo.bInCenterGrid ? GetGridSlotLocation(HealthBarInfo.DesiredSlot) - HealthBarInfo.RootPosition : FVector2D(0.0f);
		//Check how much the offset has changed since last frame.
		const float DistanceFromPrevious = (HealthBarInfo.FinalOffset - HealthBarInfo.PreviousOffset).Length();
		const float MAXMOVEPERFRAME = 300.0f * DeltaTime;
		//If the offset will change too much, clamp it.
		if (DistanceFromPrevious > MAXMOVEPERFRAME)
		{
			FVector2D MovementDirection = HealthBarInfo.FinalOffset - HealthBarInfo.PreviousOffset;
			MovementDirection.Normalize();
			HealthBarInfo.FinalOffset = HealthBarInfo.PreviousOffset + (MovementDirection * MAXMOVEPERFRAME);
		}
		/*FString ToPrint = FString::Printf(TEXT("%f %f root position. Wants center grid slot: %d. Desired grid slot: %i %i. Desired slot location: %f %f. Num slots taken: %i."),
			HealthBarInfo.RootPosition.X, HealthBarInfo.RootPosition.Y, HealthBarInfo.bInCenterGrid, HealthBarInfo.DesiredSlot.X, HealthBarInfo.DesiredSlot.Y,
			GetGridSlotLocation(HealthBarInfo.DesiredSlot).X, GetGridSlotLocation(HealthBarInfo.DesiredSlot).Y, GridSlotsThisFrame.Num());
		UKismetSystemLibrary::PrintString(GetOwningPlayer(), ToPrint,true, true, FLinearColor(FColor::White), 0.0f, FName(TEXT("DebugBarPosition")));*/
		HealthBarInfo.WidgetRef->SetPositionInViewport(HealthBarInfo.FinalOffset + HealthBarInfo.RootPosition);
	}
}

FHealthBarGridSlot UFloatingHealthBarManager::FindDesiredGridSlot(const FVector2D& RootPosition) const
{
	const int32 SlotX = FMath::RoundToInt32((RootPosition.X - CenterScreen.X) / GridSlotSize);
	const int32 SlotY = FMath::RoundToInt32((RootPosition.Y - CenterScreen.Y) / GridSlotSize);
	return FHealthBarGridSlot(SlotX, SlotY);
}

void UFloatingHealthBarManager::UpdateHealthBarRootPosition(FFloatingHealthBarInfo& HealthBar) const
{
	//Update whether the bar should be on screen this frame, by checking line of sight from the center of the bar's actor to the local player.
	//TODO: In the future this should probably use camera location, but the c++ player character class doesn't have a camera.
	//Maybe combat interface can have a CheckLineOfSightFrom function?
	const bool bIsLineOfSight = IsValid(LocalPlayer) && IsValid(LocalPlayerCombatStatus) ?
		UAbilityFunctionLibrary::CheckLineOfSightInPlane(LocalPlayer, LocalPlayer->GetActorLocation(), HealthBar.Target->GetActorLocation(), LocalPlayerCombatStatus->GetCurrentPlane())
			: true;
	HealthBar.bOnScreen = bIsLineOfSight &&
		UGameplayStatics::ProjectWorldToScreen(GetOwningPlayer(), HealthBar.TargetComponent->GetSocketLocation(HealthBar.TargetSocket), HealthBar.RootPosition, true);
}

void UFloatingHealthBarManager::ClampBarRootPosition(FVector2D& Root) const
{
	Root.X = FMath::Clamp(Root.X, CenterScreen.X - ((ViewportX / 2.0f) * ClampPercent), CenterScreen.X + ((ViewportX / 2.0f) * ClampPercent));
	Root.Y = FMath::Clamp(Root.Y, CenterScreen.Y - ((ViewportY / 2.0f) * ClampPercent), CenterScreen.Y + ((ViewportY / 2.0f) * ClampPercent));
}

bool UFloatingHealthBarManager::IsInCenterGrid(const FVector2D& Location) const
{
	const float GridXMin = CenterScreen.X - (GridRadius * GridSlotSize);
	const float GridXMax = CenterScreen.X + (GridRadius * GridSlotSize);
	const float GridYMin = CenterScreen.Y - (GridRadius * GridSlotSize);
	const float GridYMax = CenterScreen.Y + (GridRadius * GridSlotSize);
	return Location.X >= GridXMin && Location.X <= GridXMax && Location.Y >= GridYMin && Location.Y <= GridYMax;
}
