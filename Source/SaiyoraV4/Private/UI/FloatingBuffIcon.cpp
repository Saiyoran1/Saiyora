#include "FloatingBuffIcon.h"
#include "Buff.h"

void UFloatingBuffIcon::Init(UBuff* Buff)
{
	if (!IsValid(Buff))
	{
		RemoveFromParent();
		return;
	}
	CurrentBuff = Buff;
	CurrentBuff->OnUpdated.AddDynamic(this, &UFloatingBuffIcon::OnBuffUpdated);
	CurrentBuff->OnRemoved.AddDynamic(this, &UFloatingBuffIcon::OnBuffRemoved);
	UpdateBuffIcon();
}

void UFloatingBuffIcon::OnBuffUpdated(const FBuffApplyEvent& Event)
{
	UpdateBuffIcon();
}

void UFloatingBuffIcon::OnBuffRemoved(const FBuffRemoveEvent& Event)
{
	if (IsValid(CurrentBuff))
	{
		CurrentBuff->OnUpdated.RemoveDynamic(this, &UFloatingBuffIcon::OnBuffUpdated);
		CurrentBuff->OnRemoved.RemoveDynamic(this, &UFloatingBuffIcon::OnBuffRemoved);
	}
	RemoveFromParent();
}
