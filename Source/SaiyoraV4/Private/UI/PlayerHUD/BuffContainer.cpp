#include "PlayerHUD/BuffContainer.h"
#include "Buff.h"
#include "BuffHandler.h"
#include "PlayerHUD.h"
#include "BuffBar.h"
#include "SaiyoraPlayerCharacter.h"
#include "VerticalBox.h"

void UBuffContainer::InitBuffContainer(UPlayerHUD* OwningHUD, ASaiyoraPlayerCharacter* OwningPlayer, const EBuffType BuffType)
{
	if (!IsValid(OwningHUD) || !IsValid(OwningPlayer) || !OwningPlayer->Implements<USaiyoraCombatInterface>()
		|| BuffType == EBuffType::HiddenBuff || !IsValid(BuffWidgetClass))
	{
		return;
	}
	OwnerBuffHandler = ISaiyoraCombatInterface::Execute_GetBuffHandler(OwningPlayer);
	if (!IsValid(OwnerBuffHandler))
	{
		return;
	}
	OwnerHUD = OwningHUD;
	ContainerType = BuffType;
	OwnerBuffHandler->OnIncomingBuffApplied.AddDynamic(this, &UBuffContainer::OnBuffApplied);
	OwnerBuffHandler->OnIncomingBuffRemoved.AddDynamic(this, &UBuffContainer::OnBuffRemoved);
	TArray<UBuff*> Buffs;
	if (ContainerType == EBuffType::Buff)
	{
		OwnerBuffHandler->GetBuffs(Buffs);
	}
	else
	{
		OwnerBuffHandler->GetDebuffs(Buffs);
	}
	for (UBuff* Buff : Buffs)
	{
		FBuffApplyEvent Event;
		Event.AffectedBuff = Buff;
		OnBuffApplied(Event);
	}
}

void UBuffContainer::OnBuffApplied(const FBuffApplyEvent& Event)
{
	if (!IsValid(Event.AffectedBuff) || Event.AffectedBuff->GetBuffType() != ContainerType)
	{
		return;
	}
	if (InactiveBuffWidgets.Num() == 0)
	{
		UBuffBar* NewBuffWidget = CreateWidget<UBuffBar>(this, BuffWidgetClass);
		if (!IsValid(NewBuffWidget))
		{
			return;
		}
		NewBuffWidget->InitBuffWidget(OwnerHUD);
		InactiveBuffWidgets.Add(NewBuffWidget);
	}
	UBuffBar* BuffWidget = InactiveBuffWidgets.Pop();
	BuffWidget->SetBuff(Event.AffectedBuff);
	BuffBox->AddChildToVerticalBox(BuffWidget);
	ActiveBuffWidgets.Add(Event.AffectedBuff, BuffWidget);
}

void UBuffContainer::OnBuffRemoved(const FBuffRemoveEvent& Event)
{
	if (!IsValid(Event.RemovedBuff) || Event.RemovedBuff->GetBuffType() != ContainerType)
	{
		return;
	}
	UBuffBar* BuffWidget = nullptr;
	ActiveBuffWidgets.RemoveAndCopyValue(Event.RemovedBuff, BuffWidget);
	if (!IsValid(BuffWidget))
	{
		return;
	}
	BuffWidget->RemoveFromParent();
	BuffWidget->SetBuff(nullptr);
	InactiveBuffWidgets.Add(BuffWidget);
}