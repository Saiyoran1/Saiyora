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
	UpdateHealthBarPositions();
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
	NewHealthBarInfo.WidgetRef->SetAlignmentInViewport(FVector2d(0.5f, 0.5f));
	NewHealthBarInfo.WidgetRef->Init(Target);
	FloatingBars.Add(NewHealthBarInfo);
}

void UFloatingHealthBarManager::UpdateHealthBarPositions()
{
	//TODO: All the fun stuff.
	int32 ViewportX;
	int32 ViewportY;
	GetOwningPlayer()->GetViewportSize(ViewportX, ViewportY);
	const FVector2d Center = FVector2d(ViewportX / 2.0f, ViewportY / 2.0f);
	for (FFloatingHealthBarInfo& HealthBarInfo : FloatingBars)
	{
		const bool bIsLineOfSight = IsValid(LocalPlayer) && IsValid(LocalPlayerCombatStatus) ?
			UAbilityFunctionLibrary::CheckLineOfSightInPlane(LocalPlayer, LocalPlayer->GetActorLocation(), HealthBarInfo.Target->GetActorLocation(), LocalPlayerCombatStatus->GetCurrentPlane())
				: true;
		HealthBarInfo.bOnScreen = bIsLineOfSight &&
			UGameplayStatics::ProjectWorldToScreen(GetOwningPlayer(), HealthBarInfo.TargetComponent->GetSocketLocation(HealthBarInfo.TargetSocket), HealthBarInfo.DesiredPosition, true);
		if (HealthBarInfo.bOnScreen)
		{
			HealthBarInfo.DesiredPosition.X = FMath::Clamp(HealthBarInfo.DesiredPosition.X, Center.X - ((ViewportX / 2.0f) * .9f), Center.X + ((ViewportX / 2.0f) * 0.9f));
			HealthBarInfo.DesiredPosition.Y = FMath::Clamp(HealthBarInfo.DesiredPosition.Y, Center.Y - ((ViewportY / 2.0f) * 0.9f), Center.Y + ((ViewportY / 2.0f) * 0.9f));
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::HitTestInvisible);
			//HealthBarInfo.WidgetRef->SetPositionInViewport(HealthBarInfo.DesiredPosition);
		}
		else
		{
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	FloatingBars.Sort([Center](const FFloatingHealthBarInfo& Left, const FFloatingHealthBarInfo& Right)
	{
		if (!Left.bOnScreen)
		{
			return true;
		}
		if (!Right.bOnScreen)
		{
			return false;
		}
		return (Left.DesiredPosition - Center).Length() < (Right.DesiredPosition - Center).Length();
	});
	for (int32 i = 0; i < FloatingBars.Num(); i++)
	{
		if (FloatingBars[i].bOnScreen)
		{
			/*const float Size = FloatingBars[i].WidgetRef->GetDesiredSize().X;
			for (int32 j = 0; j < i; j++)
			{
				const float Dist = (FloatingBars[i].DesiredPosition - FloatingBars[j].DesiredPosition).Length();
				if (Dist < Size)
				{
					const float Factor = (Size - Dist)/Dist;
					FloatingBars[i].DesiredPosition.X += 
				}
			}*/
			FloatingBars[i].WidgetRef->SetPositionInViewport(FloatingBars[i].DesiredPosition);
		}
	}
}
