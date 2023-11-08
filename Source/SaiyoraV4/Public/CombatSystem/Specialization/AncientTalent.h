#pragma once
#include "CoreMinimal.h"
#include "AncientSpecialization.h"
#include "Object.h"
#include "AncientTalent.generated.h"

UCLASS(Blueprintable)
class SAIYORAV4_API UAncientTalent : public UObject
{
	GENERATED_BODY()

public:

	void SelectTalent(UAncientSpecialization* OwningSpecialization, const TSubclassOf<UCombatAbility> BaseAbility);
	void UnselectTalent();

protected:

	UFUNCTION(BlueprintPure, Category = "Talent")
	UAncientSpecialization* GetOwningSpec() const { return OwningSpec; }

	UFUNCTION(BlueprintNativeEvent)
	void OnTalentSelected();
	void OnTalentSelected_Implementation() {}
	UFUNCTION(BlueprintNativeEvent)
	void OnTalentUnselected();
	void OnTalentUnselected_Implementation() {}

private:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FName TalentName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FText TalentDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	UTexture2D* TalentIcon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	EHealthEventSchool TalentSchool;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UCombatAbility> ReplaceBaseWith = nullptr;
	
	UPROPERTY()
	UAncientSpecialization* OwningSpec;
	TSubclassOf<UCombatAbility> BaseAbilityClass;

	void ReplacePlayerAbilityWith(const TSubclassOf<UCombatAbility> AbilityToReplace, const TSubclassOf<UCombatAbility> NewAbility);
};
