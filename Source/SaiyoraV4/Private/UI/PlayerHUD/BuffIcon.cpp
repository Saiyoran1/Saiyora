#include "BuffIcon.h"
#include "Buff.h"

void UBuffIcon::Init(UBuff* AssignedBuff)
{
	if (!IsValid(AssignedBuff) || !IsValid(BuffMaterial))
	{
		return;
	}
	Buff = AssignedBuff;
	BuffMI = UMaterialInstanceDynamic::Create(BuffMaterial, this);
	BuffMI->SetTextureParameterValue(FName("IconTexture"), Buff->GetBuffIcon());
	BuffMI->SetVectorParameterValue(FName("IconColor"), Buff->GetBuffProgressColor());
	BuffMI->SetScalarParameterValue(FName("DurationRemainingPercent"), 1.0f);
	BuffIcon->SetBrushFromMaterial(BuffMI);

	Buff->OnUpdated.AddDynamic(this, &UBuffIcon::OnBuffUpdated);
	UpdateStacks(Buff->IsStackable() ? Buff->GetCurrentStacks() : 1);
}

void UBuffIcon::Cleanup()
{
	if (IsValid(Buff))
	{
		Buff->OnUpdated.RemoveDynamic(this, &UBuffIcon::OnBuffUpdated);
	}
	Buff = nullptr;
}

void UBuffIcon::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsValid(Buff) && Buff->HasFiniteDuration())
	{
		const float Timestamp = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		const float Elapsed = Timestamp - Buff->GetLastRefreshTime();
		BuffMI->SetScalarParameterValue(FName("DurationRemainingPercent"),
			FMath::Clamp(Buff->GetRemainingTime() / (Buff->GetRemainingTime() + Elapsed), 0.0f, 1.0f));
	}
}

void UBuffIcon::OnBuffUpdated(const FBuffApplyEvent& Event)
{
	UpdateStacks(Event.NewStacks);
}

void UBuffIcon::UpdateStacks(const int NewStacks)
{
	if (NewStacks > 1)
	{
		StackText->SetVisibility(ESlateVisibility::HitTestInvisible);
		StackText->SetText(FText::FromString(FString::FromInt(Buff->GetCurrentStacks())));
	}
	else
	{
		StackText->SetVisibility(ESlateVisibility::Collapsed);
	}
}