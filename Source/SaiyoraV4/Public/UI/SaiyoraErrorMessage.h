#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "SaiyoraErrorMessage.generated.h"

class UTextBlock;

UCLASS(Abstract)
class SAIYORAV4_API USaiyoraErrorMessage : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetErrorMessage(const FText& Message, const float Duration);

protected:

	UFUNCTION(BlueprintPure)
	FText GetMessageText() const { return MessageText; }
	UFUNCTION(BlueprintPure)
	float GetMessageDuration() const { return MessageDuration; }
	UFUNCTION(BlueprintPure)
	float GetMessageDurationRemaining() const { return MessageDuration > 0.0f ? GetWorld()->GetTimerManager().GetTimerRemaining(ExpireHandle) : -1.0f; }

private:

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true", BindWidget))
	UTextBlock* ErrorText;
	float MessageDuration = -1.0f;
	FText MessageText;
	FTimerHandle ExpireHandle;
	UFUNCTION()
	void ExpireErrorMessage();
};
