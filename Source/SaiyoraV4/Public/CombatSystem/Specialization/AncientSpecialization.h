#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "Object.h"
#include "ResourceStructs.h"
#include "SpecializationStructs.h"
#include "AncientSpecialization.generated.h"

class ASaiyoraPlayerCharacter;
class UResource;

UCLASS(Blueprintable)
class SAIYORAV4_API UAncientSpecialization : public UObject
{
	GENERATED_BODY()

public:

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitializeSpecialization(ASaiyoraPlayerCharacter* PlayerCharacter);
	void UnlearnSpec();

	UPROPERTY(BlueprintAssignable)
	FTalentChangeNotification OnTalentChanged;

	UFUNCTION(BlueprintPure, Category = "Specialization")
	ASaiyoraPlayerCharacter* GetOwningPlayer() const { return OwningPlayer; }
	
	UFUNCTION(BlueprintPure, Category = "Specialization")
	void GetLoadout(TArray<FAncientTalentChoice>& OutLoadout) const { OutLoadout = Loadout.Items; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Specialization")
	void SelectAncientTalent(const TSubclassOf<UCombatAbility> BaseAbility, const TSubclassOf<UAncientTalent> TalentSelection);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Specialization")
	void ClearTalentSelection(const TSubclassOf<UCombatAbility> BaseAbility);

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FName SpecName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FText SpecDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	UTexture2D* SpecImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	EHealthEventSchool SpecSchool;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	FAncientTalentSet Loadout;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	TSubclassOf<UResource> ResourceClass;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	FResourceInitInfo ResourceInitInfo;

private:

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
	bool bInitialized = false;
};