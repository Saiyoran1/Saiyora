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
	for (FFloatingHealthBarInfo& HealthBarInfo : FloatingBars)
	{
		//If the bar was previously on screen last frame, we save the previous offset so that health bars aren't jumping around like crazy.
		HealthBarInfo.PreviousOffset = HealthBarInfo.bOnScreen ? HealthBarInfo.FinalOffset : FVector2D(0.0f);
		//Update whether the bar should be on screen this frame, by checking line of sight from the center of the bar's actor to the local player.
		//TODO: In the future this should probably use camera location, but the c++ player character class doesn't have a camera.
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
		}
		else
		{
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	//Sort floating health bars by distance from center screen, with all hidden bars in the first part of the array.
	FloatingBars.Sort([CenterScreen](const FFloatingHealthBarInfo& Left, const FFloatingHealthBarInfo& Right)
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
	});
	
	bool bFoundFirstVisible = false;
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
		const float WidgetSizeX = FloatingBars[i].WidgetRef->GetDesiredSize().X;
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
	}
}
