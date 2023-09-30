#include "FloatingHealthBarManager.h"
#include "CanvasPanelSlot.h"
#include "CombatGroup.h"
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
	for (FFloatingHealthBarInfo& HealthBarInfo : FloatingBars)
	{
		HealthBarInfo.bOnScreen = UGameplayStatics::ProjectWorldToScreen(GetOwningPlayer(), HealthBarInfo.TargetComponent->GetSocketLocation(HealthBarInfo.TargetSocket), HealthBarInfo.DesiredPosition, true);
		if (HealthBarInfo.bOnScreen)
		{
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::HitTestInvisible);
			HealthBarInfo.WidgetRef->SetPositionInViewport(HealthBarInfo.DesiredPosition);
		}
		else
		{
			HealthBarInfo.WidgetRef->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
