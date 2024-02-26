#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "DamageEnums.h"
#include "Overlay.h"
#include "ProgressBar.h"
#include "TextBlock.h"
#include "WrapBox.h"
#include "Blueprint/UserWidget.h"
#include "FloatingHealthBar.generated.h"

class UAbilityComponent;
struct FBuffApplyEvent;
class UFloatingBuffIcon;
class ASaiyoraPlayerCharacter;
class UDamageHandler;
class UBuffHandler;

UCLASS(Abstract)
class SAIYORAV4_API UFloatingHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(AActor* TargetActor);

#pragma region Health

private:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UOverlay* HealthOverlay;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UProgressBar* HealthBar;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UTextBlock* HealthText;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UProgressBar* AbsorbBar;

	UPROPERTY()
	UDamageHandler* TargetDamageHandler = nullptr;

	UFUNCTION()
	void UpdateHealth(AActor* Actor, const float PreviousHealth, const float NewHealth);
	UFUNCTION()
	void UpdateAbsorb(AActor* Actor, const float PreviousHealth, const float NewHealth);
	UFUNCTION()
	void UpdateLifeStatus(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);

#pragma endregion
#pragma region Buffs

private:
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UWrapBox* BuffBox;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UWrapBox* DebuffBox;

	UPROPERTY(EditDefaultsOnly, Category = "Buffs", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UFloatingBuffIcon> BuffIconClass;
	
	UPROPERTY()
	UBuffHandler* TargetBuffHandler = nullptr;
	UPROPERTY()
	ASaiyoraPlayerCharacter* LocalPlayer = nullptr;

	UFUNCTION()
	void OnIncomingBuffApplied(const FBuffApplyEvent& Event);

#pragma endregion
#pragma region CastBar

public:

protected:

	UFUNCTION(BlueprintPure)
	UAbilityComponent* GetTargetAbilityComponent() const { return TargetAbilityComponent; }
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnCastStateChanged(const FCastingState& PreviousState, const FCastingState& NewState);
	UFUNCTION(BlueprintImplementableEvent)
	void OnCastInterrupted(const FInterruptEvent& Event);
	UFUNCTION(BlueprintImplementableEvent)
	void OnCastCancelled(const FCancelEvent& Event);

private:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UProgressBar* CastBar;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = "true"))
	UTextBlock* CastBarText;

	UPROPERTY()
	UAbilityComponent* TargetAbilityComponent = nullptr;

#pragma endregion 
};
