#pragma once
#include "CoreMinimal.h"
#include "Button.h"
#include "UserWidget.h"
#include "EscapeMenu.generated.h"

UCLASS(Abstract)
class SAIYORAV4_API UEscapeMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	
	void Init();

private:

	UPROPERTY(meta = (BindWidget))
	UButton* LeaveGameButton;
	UFUNCTION()
	void OnLeaveGamePressed();
	UFUNCTION()
	void OnSessionDestroyed(const bool bSuccess, const FString& Error);
};
