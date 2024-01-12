#pragma once
#include "CoreMinimal.h"
#include "UserWidget.h"
#include "FloatingBuffIcon.generated.h"

struct FBuffRemoveEvent;
struct FBuffApplyEvent;
class UBuff;

UCLASS()
class SAIYORAV4_API UFloatingBuffIcon : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(UBuff* Buff);

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateBuffIcon();

	UFUNCTION(BlueprintPure)
	UBuff* GetCurrentBuff() const { return CurrentBuff; }

private:

	UPROPERTY()
	UBuff* CurrentBuff;

	UFUNCTION()
	void OnBuffUpdated(const FBuffApplyEvent& Event);
	UFUNCTION()
	void OnBuffRemoved(const FBuffRemoveEvent& Event);
};
