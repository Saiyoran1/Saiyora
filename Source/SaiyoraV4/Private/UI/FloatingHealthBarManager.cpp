#include "FloatingHealthBarManager.h"

#include "AbilityFunctionLibrary.h"
#include "CanvasPanelSlot.h"
#include "CombatGroup.h"
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
	for (FFloatingHealthBarInfo& HealthBarInfo : FloatingBars)
	{
		HealthBarInfo.bPreviouslyOnScreen = HealthBarInfo.bOnScreen;
		if (HealthBarInfo.bPreviouslyOnScreen)
		{
			HealthBarInfo.PreviousPosition = HealthBarInfo.FinalPosition;
		}
		const bool bIsLineOfSight = IsValid(LocalPlayer) && IsValid(LocalPlayerCombatStatus) ?
			UAbilityFunctionLibrary::CheckLineOfSightInPlane(LocalPlayer, LocalPlayer->GetActorLocation(), HealthBarInfo.Target->GetActorLocation(), LocalPlayerCombatStatus->GetCurrentPlane())
				: true;
		HealthBarInfo.bOnScreen = bIsLineOfSight &&
			UGameplayStatics::ProjectWorldToScreen(GetOwningPlayer(), HealthBarInfo.TargetComponent->GetSocketLocation(HealthBarInfo.TargetSocket), HealthBarInfo.DesiredPosition, true);
		if (HealthBarInfo.bOnScreen)
		{
			HealthBarInfo.DesiredPosition.X = FMath::Clamp(HealthBarInfo.DesiredPosition.X, CenterScreen.X - ((ViewportX / 2.0f) * .9f), CenterScreen.X + ((ViewportX / 2.0f) * 0.9f));
			HealthBarInfo.DesiredPosition.Y = FMath::Clamp(HealthBarInfo.DesiredPosition.Y, CenterScreen.Y - ((ViewportY / 2.0f) * 0.9f), CenterScreen.Y + ((ViewportY / 2.0f) * 0.9f));
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	FloatingBars.Sort([CenterScreen](const FFloatingHealthBarInfo& Left, const FFloatingHealthBarInfo& Right)
	{
		if (Left.bOnScreen != Right.bOnScreen)
		{
			//If one is on screen and the other isn't, the right must be the one on screen.
			return Right.bOnScreen;
		}
		return (Left.DesiredPosition - CenterScreen).Length() < (Right.DesiredPosition - CenterScreen).Length();
	});
	
	for (int32 i = 0; i < FloatingBars.Num(); i++)
	{
		if (!FloatingBars[i].bOnScreen)
		{
			continue;
		}
		const float WidgetSizeX = FloatingBars[i].WidgetRef->GetDesiredSize().X;
		//TODO: how the fuck?
		FVector2D Repulsion = FVector2D(0.0f, 0.0f);
		for (int32 j = 0; j < i; j++)
		{
			if (!FloatingBars[j].bOnScreen)
			{
				continue;
			}
			FVector2D Direction = FloatingBars[i].DesiredPosition - FloatingBars[j].FinalPosition;
			const float Distance = Direction.Length();
			if (Distance < WidgetSizeX)
			{
				Direction.Normalize();
				Repulsion += Direction * (WidgetSizeX - Distance);
			}
		}
		//Add repulsion from overlaps to desired position.
		FloatingBars[i].FinalPosition = FloatingBars[i].DesiredPosition + Repulsion;
		//Clamp the new position to within a radius of the previous position, if the widget was on screen last frame.
		if (FloatingBars[i].bPreviouslyOnScreen)
		{
			const float DistanceFromPrevious = (FloatingBars[i].FinalPosition - FloatingBars[i].PreviousPosition).Length();
			const float MAXMOVEPERFRAME = 200.0f * DeltaTime;
			if (DistanceFromPrevious > MAXMOVEPERFRAME)
			{
				FVector2D MovementDirection = FloatingBars[i].FinalPosition - FloatingBars[i].PreviousPosition;
				MovementDirection.Normalize();
				FloatingBars[i].FinalPosition = FloatingBars[i].PreviousPosition + (MovementDirection * MAXMOVEPERFRAME);
			}
		}
		//Clamp the widget within a wide radius of the desired position, to prevent too much lagging behind.
		const float DistanceFromDesired = (FloatingBars[i].FinalPosition - FloatingBars[i].DesiredPosition).Length();
		const float MAXDISTANCEFROMDESIRED = 500.0f;
		if (DistanceFromDesired > MAXDISTANCEFROMDESIRED)
		{
			FVector2D MovementDirection = FloatingBars[i].FinalPosition - FloatingBars[i].DesiredPosition;
			MovementDirection.Normalize();
			FloatingBars[i].FinalPosition = FloatingBars[i].DesiredPosition + (MovementDirection * MAXDISTANCEFROMDESIRED);
		}
		FloatingBars[i].WidgetRef->SetPositionInViewport(FloatingBars[i].FinalPosition);
	}
}
