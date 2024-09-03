#pragma once
#include "CoreMinimal.h"
#include "AbilityEnums.h"
#include "CombatEnums.h"
#include "UserWidget.h"
#include "ActionSlot.generated.h"

class ASaiyoraPlayerCharacter;
class UProgressBar;
class UTextBlock;
class UImage;
class UAbilityComponent;
class UCombatAbility;

UCLASS()
class SAIYORAV4_API UActionSlot : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitActionSlot(UAbilityComponent* AbilityComponent, const ESaiyoraPlane Plane, const int32 SlotIdx);
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:

	virtual void UpdateCooldown();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	UMaterialInterface* AbilityIconMaterial;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UImage* AbilityIcon;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* KeybindText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* CooldownProgress;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* CooldownText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* ChargesText;

	UPROPERTY()
	UMaterialInstanceDynamic* ImageInstance = nullptr;
	
	UPROPERTY()
	UAbilityComponent* OwnerAbilityComp = nullptr;
	UPROPERTY()
	ASaiyoraPlayerCharacter* OwnerCharacter = nullptr;
	ESaiyoraPlane AssignedPlane = ESaiyoraPlane::Modern;
	int32 AssignedIdx = 0;
	
	void SetAbilityClass(const TSubclassOf<UCombatAbility> NewAbilityClass);
	TSubclassOf<UCombatAbility> AbilityClass;
	UPROPERTY()
	UCombatAbility* AssociatedAbility = nullptr;

	UFUNCTION()
	void OnAbilityAdded(UCombatAbility* NewAbility);
	UFUNCTION()
	void OnAbilityRemoved(UCombatAbility* RemovedAbility);
	UFUNCTION()
	void OnChargesChanged(UCombatAbility* Ability, const int32 PreviousCharges, const int32 NewCharges);
	UFUNCTION()
	void OnCastableChanged(UCombatAbility* Ability, const bool bCastable, const TArray<ECastFailReason>& FailReasons);
	UFUNCTION()
	void OnMappingChanged(const ESaiyoraPlane Plane, const int32 Index, TSubclassOf<UCombatAbility> AbilityClass);

	void UpdateAbilityInstance(UCombatAbility* NewAbility);

	bool bInitialized = false;
};
