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

FVector2D UFloatingHealthBarManager::GetGridSlotLocation(const FVector2D CenterScreen, const float WidgetSize,
	const FHealthBarGridSlot& GridSlot)
{
	return FVector2D(CenterScreen.X + (WidgetSize * GridSlot.X),CenterScreen.Y + (WidgetSize * GridSlot.Y));
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
	int32 ViewportX;
	int32 ViewportY;
	GetOwningPlayer()->GetViewportSize(ViewportX, ViewportY);
	const FVector2D CenterScreen = FVector2D(ViewportX / 2.0f, ViewportY / 2.0f);
	//Update desired position and whether each bar is on screen (is line of sight, and project world to screen was successful.
	TArray<FFloatingHealthBarInfo> CenterGridBars;
	for (FFloatingHealthBarInfo& HealthBarInfo : FloatingBars)
	{
		//If the bar was previously on screen last frame, we save the previous offset so that health bars aren't jumping around like crazy.
		HealthBarInfo.PreviousOffset = HealthBarInfo.bOnScreen ? HealthBarInfo.FinalOffset : FVector2D(0.0f);
		//Update whether the bar should be on screen this frame, by checking line of sight from the center of the bar's actor to the local player.
		//TODO: In the future this should probably use camera location, but the c++ player character class doesn't have a camera.
		//Maybe combat interface can have a CheckLineOfSightFrom function?
		const bool bIsLineOfSight = IsValid(LocalPlayer) && IsValid(LocalPlayerCombatStatus) ?
			UAbilityFunctionLibrary::CheckLineOfSightInPlane(LocalPlayer, LocalPlayer->GetActorLocation(), HealthBarInfo.Target->GetActorLocation(), LocalPlayerCombatStatus->GetCurrentPlane())
				: true;
		HealthBarInfo.bOnScreen = bIsLineOfSight &&
			UGameplayStatics::ProjectWorldToScreen(GetOwningPlayer(), HealthBarInfo.TargetComponent->GetSocketLocation(HealthBarInfo.TargetSocket), HealthBarInfo.DesiredPosition, true);
		if (HealthBarInfo.bOnScreen)
		{
			//Update the desired center position of the bar, clamped to 90% of the screen.
			HealthBarInfo.DesiredPosition.X = FMath::Clamp(HealthBarInfo.DesiredPosition.X, CenterScreen.X - ((ViewportX / 2.0f) * .9f), CenterScreen.X + ((ViewportX / 2.0f) * 0.9f));
			HealthBarInfo.DesiredPosition.Y = FMath::Clamp(HealthBarInfo.DesiredPosition.Y, CenterScreen.Y - ((ViewportY / 2.0f) * 0.9f), CenterScreen.Y + ((ViewportY / 2.0f) * 0.9f));
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::HitTestInvisible);
			
			const float WidgetSizeX = HealthBarInfo.WidgetRef->GetDesiredSize().X;
			constexpr int32 GridSlotsRadius = 5;
			const float GridXMin = CenterScreen.X - (GridSlotsRadius * WidgetSizeX);
			const float GridXMax = CenterScreen.X + (GridSlotsRadius * WidgetSizeX);
			const float GridYMin = CenterScreen.Y - (GridSlotsRadius * WidgetSizeX);
			const float GridYMax = CenterScreen.Y + (GridSlotsRadius * WidgetSizeX);
			if (HealthBarInfo.DesiredPosition.X >= GridXMin && HealthBarInfo.DesiredPosition.X <= GridXMax && HealthBarInfo.DesiredPosition.Y >= GridYMin && HealthBarInfo.DesiredPosition.Y <= GridYMax)
			{
				//This health bar is in the center grid.
				CenterGridBars.Add(HealthBarInfo);
				const int32 SlotX = FMath::RoundToInt32((HealthBarInfo.DesiredPosition.X - CenterScreen.X) / WidgetSizeX);
				const int32 SlotY = FMath::RoundToInt32((HealthBarInfo.DesiredPosition.Y - CenterScreen.Y) / WidgetSizeX);
				HealthBarInfo.DesiredSlot = FHealthBarGridSlot(SlotX, SlotY);
				HealthBarInfo.DistanceFromGridSlot = (GetGridSlotLocation(CenterScreen, WidgetSizeX, HealthBarInfo.DesiredSlot) - HealthBarInfo.DesiredPosition).Length();
			}
			else
			{
				//TODO: Can do smoothing for health bars that have left the central grid if needed.
				HealthBarInfo.WidgetRef->SetPositionInViewport(HealthBarInfo.DesiredPosition);
			}
		}
		else
		{
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	CenterGridBars.Sort([](const FFloatingHealthBarInfo& Left, const FFloatingHealthBarInfo& Right)
	{
		return Left.DistanceFromGridSlot <= Right.DistanceFromGridSlot;
	});
	TArray<FHealthBarGridSlot> TakenGridSlots;
	for (FFloatingHealthBarInfo& HealthBarInfo : CenterGridBars)
	{
		const float WidgetSizeX = HealthBarInfo.WidgetRef->GetDesiredSize().X;
		if (TakenGridSlots.Contains(HealthBarInfo.DesiredSlot))
		{
			//Check whether we are looking for right side or left side slots.
			const bool bRightSide = HealthBarInfo.DesiredPosition.X > GetGridSlotLocation(CenterScreen, WidgetSizeX, HealthBarInfo.DesiredSlot).X;
			const bool bTop = HealthBarInfo.DesiredPosition.Y > GetGridSlotLocation(CenterScreen, WidgetSizeX, HealthBarInfo.DesiredSlot).Y;
			//Check if should start looking on the sides or on the top/bottom, depending on which axis the widget is further away from the center.
			const bool bStartHorizontal = FMath::Abs(HealthBarInfo.DesiredPosition.X - GetGridSlotLocation(CenterScreen, WidgetSizeX, HealthBarInfo.DesiredSlot).X) >
				FMath::Abs(HealthBarInfo.DesiredPosition.Y - GetGridSlotLocation(CenterScreen, WidgetSizeX, HealthBarInfo.DesiredSlot).Y);
			//Get which slot to check first.
			EGridSlotOffset Offset = BoolsToGridSlot(bRightSide, bTop, bStartHorizontal);
			const EGridSlotOffset StartingOffset = Offset;
			//Offset the slot integers depending on which slot to check.
			switch (Offset)
			{
			case EGridSlotOffset::Right :
				HealthBarInfo.DesiredSlot.X += 1;
				break;
			case EGridSlotOffset::BottomRight :
				HealthBarInfo.DesiredSlot.X += 1;
				HealthBarInfo.DesiredSlot.Y -= 1;
				break;
			case EGridSlotOffset::Bottom :
				HealthBarInfo.DesiredSlot.Y -= 1;
				break;
			case EGridSlotOffset::BottomLeft :
				HealthBarInfo.DesiredSlot.X -= 1;
				HealthBarInfo.DesiredSlot.Y -= 1;
				break;
			case EGridSlotOffset::Left :
				HealthBarInfo.DesiredSlot.X -= 1;
				break;
			case EGridSlotOffset::TopLeft :
				HealthBarInfo.DesiredSlot.X -= 1;
				HealthBarInfo.DesiredSlot.Y += 1;
				break;
			case EGridSlotOffset::Top :
				HealthBarInfo.DesiredSlot.Y += 1;
				break;
			case EGridSlotOffset::TopRight :
				HealthBarInfo.DesiredSlot.X += 1;
				HealthBarInfo.DesiredSlot.Y += 1;
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
			while (TakenGridSlots.Contains(HealthBarInfo.DesiredSlot))
			{
				IncrementGridSlot(bClockwise, Offset, HealthBarInfo.DesiredSlot);
				//Right now, any health bars beyond 9 that are vying for the same slot will end up overlapping.
				if (StartingOffset == Offset)
				{
					break;
				}
			}
		}
		
		TakenGridSlots.Add(HealthBarInfo.DesiredSlot);
		//Calculate the offset of the health bar from its desired position to the slot it should occupy.
		HealthBarInfo.FinalOffset = GetGridSlotLocation(CenterScreen, WidgetSizeX, HealthBarInfo.DesiredSlot) - HealthBarInfo.DesiredPosition;
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
		HealthBarInfo.WidgetRef->SetPositionInViewport(HealthBarInfo.FinalOffset + HealthBarInfo.DesiredPosition);
	}
	//Sort floating health bars by distance from center screen, with all hidden bars in the first part of the array.
	/*FloatingBars.Sort([CenterScreen](const FFloatingHealthBarInfo& Left, const FFloatingHealthBarInfo& Right)
	{
		if (Left.bOnScreen)
		{
			if (Right.bOnScreen)
			{
				return (Left.DesiredPosition - CenterScreen).Length() < (Right.DesiredPosition - CenterScreen).Length();
			}
			return false;
		}
		return true;
	});*/
	
	/*bool bFoundFirstVisible = false;
	for (int32 i = 0; i < FloatingBars.Num(); i++)
	{
		if (!FloatingBars[i].bOnScreen)
		{
			continue;
		}
		//First visible bar is the closest to center, we want to let it be at its desired position.
		const bool bIsFirstVisible = !bFoundFirstVisible;
		if (bIsFirstVisible)
		{
			bFoundFirstVisible = true;
		}
		
		FVector2D Repulsion = FVector2D(0.0f, 0.0f);
		bool bHasOverlap = false;
		if (!bIsFirstVisible)
		{
			for (int32 j = 0; j < FloatingBars.Num(); j++)
			{
				//Don't compare to self or invisible widgets.
				if (j == i || !FloatingBars[j].bOnScreen)
				{
					continue;
				}
				//Get distance between bar we are moving and bar we are comparing to.
				const FVector2D JToI = FloatingBars[i].DesiredPosition - (j < i ? FloatingBars[j].DesiredPosition + FloatingBars[j].FinalOffset : FloatingBars[j].DesiredPosition);
				const float DistanceFromMovingBar = JToI.Length();
				if (DistanceFromMovingBar < WidgetSizeX * 2)
				{
					//This bar is within range to apply repulsion to the moving bar.
					FVector2D JToINormalized = JToI;
					JToINormalized.Normalize();
					Repulsion += JToINormalized * ((WidgetSizeX * 2) - DistanceFromMovingBar);
					if (DistanceFromMovingBar < WidgetSizeX)
					{
						//This bar is within range to cause an overlap.
						bHasOverlap = true;
					}
				}
			}
		}
		FloatingBars[i].FinalOffset = bHasOverlap ? Repulsion : FVector2D(0.0f);
		if (FloatingBars[i].FinalOffset.Length() > WidgetSizeX)
		{
			//Clamp desired offset to WidgetSizeX.
			FloatingBars[i].FinalOffset.Normalize();
			FloatingBars[i].FinalOffset *= WidgetSizeX;
		}
		//Clamp the new offset to within a radius of the previous offset.
		const float DistanceFromPrevious = (FloatingBars[i].FinalOffset - FloatingBars[i].PreviousOffset).Length();
		const float MAXMOVEPERFRAME = 150.0f * DeltaTime;
		if (DistanceFromPrevious > MAXMOVEPERFRAME)
		{
			FVector2D MovementDirection = FloatingBars[i].FinalOffset - FloatingBars[i].PreviousOffset;
			MovementDirection.Normalize();
			FloatingBars[i].FinalOffset = FloatingBars[i].PreviousOffset + (MovementDirection * MAXMOVEPERFRAME);
		}
		FloatingBars[i].WidgetRef->SetPositionInViewport(FloatingBars[i].DesiredPosition + FloatingBars[i].FinalOffset);
	}*/
}
