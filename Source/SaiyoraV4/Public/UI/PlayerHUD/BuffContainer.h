#pragma once
#include "CoreMinimal.h"
#include "BuffEnums.h"
#include "BuffStructs.h"
#include "UserWidget.h"
#include "BuffContainer.generated.h"

class UBuffHandler;
class ASaiyoraPlayerCharacter;
class UPlayerHUD;
class UBuffBar;
class UVerticalBox;

UCLASS()
class SAIYORAV4_API UBuffContainer : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitBuffContainer(UPlayerHUD* OwningHUD, ASaiyoraPlayerCharacter* OwningPlayer, const EBuffType BuffType);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UVerticalBox* BuffBox;
	UPROPERTY(EditDefaultsOnly, Category = "Buffs")
	TSubclassOf<UBuffBar> BuffWidgetClass;

	UPROPERTY()
	UPlayerHUD* OwnerHUD = nullptr;
	UPROPERTY()
	UBuffHandler* OwnerBuffHandler = nullptr;
	EBuffType ContainerType = EBuffType::Buff;
	
	UPROPERTY()
	TMap<UBuff*, UBuffBar*> ActiveBuffWidgets;
	UPROPERTY()
	TArray<UBuffBar*> InactiveBuffWidgets;

	UFUNCTION()
	void OnBuffApplied(const FBuffApplyEvent& Event);
	UFUNCTION()
	void OnBuffRemoved(const FBuffRemoveEvent& Event);
};
