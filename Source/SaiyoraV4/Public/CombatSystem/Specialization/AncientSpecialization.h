#pragma once
#include "CoreMinimal.h"
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
	void SelectTalent(const TSubclassOf<UCombatAbility> BaseAbility, const TSubclassOf<UAncientTalent> TalentSelection);

	UPROPERTY(BlueprintAssignable)
	FTalentChangeNotification OnTalentChanged;

private:

	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Talents")
	FAncientTalentSet Loadout;
	UPROPERTY(EditDefaultsOnly, Category = "Resources")
	TSubclassOf<UResource> ResourceClass;
	UPROPERTY(EditDefaultsOnly, Category = "Resources")
	FResourceInitInfo ResourceInitInfo;

	UPROPERTY()
	ASaiyoraPlayerCharacter* OwningPlayer;
	bool bInitialized = false;
};