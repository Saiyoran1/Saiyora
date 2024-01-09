#include "SaiyoraErrorMessage.h"
#include "TextBlock.h"

void USaiyoraErrorMessage::SetErrorMessage(const FText& Message, const float Duration)
{
	ErrorText->SetText(Message);
	MessageDuration = Duration;
	if (MessageDuration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ExpireHandle, this, &USaiyoraErrorMessage::ExpireErrorMessage, MessageDuration);
	}
}

void USaiyoraErrorMessage::ExpireErrorMessage()
{
	RemoveFromParent();
}
