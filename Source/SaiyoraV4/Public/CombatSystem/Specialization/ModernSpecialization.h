#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "FireWeapon.h"
#include "Object.h"
#include "SpecializationStructs.h"
#include "ResourceStructs.h"
#include "ModernSpecialization.generated.h"

class ASaiyoraPlayerCharacter;
class UResource;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UModernSpecialization : public UObject
{
	GENERATED_BODY()

#pragma region Core

public:
	
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitializeSpecialization(ASaiyoraPlayerCharacter* PlayerCharacter);
	void UnlearnSpec();

	UFUNCTION(BlueprintPure, Category = "Specialization")
	ASaiyoraPlayerCharacter* GetOwningPlayer() const { return OwningPlayer; }

protected:

	UFUNCTION(BlueprintNativeEvent)
	void OnLearn();
	void OnLearn_Implementation() {}
	UFUNCTION(BlueprintNativeEvent)
	void OnUnlearn();
	void OnUnlearn_Implementation() {}

private:

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
	bool bInitialized = false;

#pragma endregion 
#pragma region Info

public:

	UFUNCTION(BlueprintPure)
	FName GetSpecName() const { return SpecName; }
	UFUNCTION(BlueprintPure)
	FText GetSpecDescription() const { return SpecDescription; }
	UFUNCTION(BlueprintPure)
	UTexture2D* GetSpecIcon() const { return SpecIcon; }
	UFUNCTION(BlueprintPure)
	EElementalSchool GetSpecSchool() const { return SpecSchool; }
	UFUNCTION(BlueprintPure)
	TSubclassOf<UResource> GetSpecResourceClass() const { return ResourceClass; }

private:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FName SpecName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FText SpecDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	UTexture2D* SpecIcon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	EElementalSchool SpecSchool;

	UPROPERTY(EditDefaultsOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UResource> ResourceClass;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true", EditCondition = "ResourceClass"))
	FResourceInitInfo ResourceInitInfo;

#pragma endregion 
#pragma region Talents

public:

	UFUNCTION(BlueprintPure)
	TSubclassOf<UFireWeapon> GetWeaponClass() const { return Weapon; }
	UFUNCTION(BlueprintPure)
	void GetSpecAbilities(TArray<TSubclassOf<UCombatAbility>>& OutAbilities) const { OutAbilities = CoreAbilities; }

	UFUNCTION(BlueprintPure, Category = "Specialization")
	void GetLoadout(TArray<FModernTalentChoice>& OutLoadout) const { OutLoadout = Loadout.Items; }

	void SelectModernAbilities(const TArray<TSubclassOf<UCombatAbility>>& Selections);
	UPROPERTY(BlueprintAssignable)
	FModernTalentChangeNotification OnTalentChanged;

private:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UFireWeapon> Weapon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UCombatAbility>> CoreAbilities;
	UPROPERTY(Replicated)
	FModernTalentSet Loadout;

#pragma endregion 
};
