#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "Object.h"
#include "SpecializationStructs.h"
#include "ResourceStructs.h"
#include "ModernSpecialization.generated.h"

class ASaiyoraPlayerCharacter;
class UResource;
class UAbilityComponent;

UCLASS(Blueprintable)
class SAIYORAV4_API UModernSpecialization : public UObject
{
	GENERATED_BODY()

public:

	UModernSpecialization();
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitializeSpecialization(ASaiyoraPlayerCharacter* PlayerCharacter);
	void UnlearnSpec();

	UPROPERTY(BlueprintAssignable)
	FModernTalentChangeNotification OnTalentChanged;

	UFUNCTION(BlueprintPure, Category = "Specialization")
	ASaiyoraPlayerCharacter* GetOwningPlayer() const { return OwningPlayer; }
	
	UFUNCTION(BlueprintPure, Category = "Specialization")
	void GetLoadout(TArray<FModernTalentChoice>& OutLoadout) const { OutLoadout = Loadout.Items; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Specialization")
	void SelectModernAbility(const int32 Slot, const TSubclassOf<UCombatAbility> Ability);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Specialization")
	void ClearAbilitySelection(const int32 Slot);

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FName SpecName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FText SpecDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	UTexture2D* SpecImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	EHealthEventSchool SpecSchool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UCombatAbility>> CoreAbilities;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	TSubclassOf<UResource> ResourceClass;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	FResourceInitInfo ResourceInitInfo;
	UFUNCTION(BlueprintNativeEvent)
	void OnLearn();
	void OnLearn_Implementation() {}
	UFUNCTION(BlueprintNativeEvent)
	void OnUnlearn();
	void OnUnlearn_Implementation() {}

private:

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
	UPROPERTY()
	UAbilityComponent* OwnerAbilityCompRef;
	bool bInitialized = false;
	UPROPERTY()
	UAbilityPool* AbilityPool;
	UPROPERTY(Replicated)
	FModernTalentSet Loadout;
};
