﻿#pragma once
#include "CoreMinimal.h"
#include "DamageEnums.h"
#include "Object.h"
#include "ResourceStructs.h"
#include "SpecializationStructs.h"
#include "AncientSpecialization.generated.h"

class ASaiyoraPlayerCharacter;
class UResource;

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UAncientSpecialization : public UObject
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UResource> ResourceClass;
	UPROPERTY(EditDefaultsOnly, Category = "Specialization", meta = (AllowPrivateAccess = "true", EditCondition = "ResourceClass"))
	FResourceInitInfo ResourceInitInfo;

#pragma endregion
#pragma region Talents

public:

	UFUNCTION(BlueprintPure)
	void GetTalentRows(TArray<FAncientTalentRow>& OutTalents) const { OutTalents = TalentRows; }
	
	UFUNCTION(BlueprintPure)
	void GetCurrentTalentLoadout(TArray<FAncientTalentChoice>& OutTalents) const { OutTalents = Loadout.Items; }
	
	void SelectAncientTalent(const FAncientTalentSelection& NewSelection);

private:

	UPROPERTY(EditDefaultsOnly, Category = "Specialization")
	TArray<FAncientTalentRow> TalentRows;
	UPROPERTY(Replicated)
	FAncientTalentSet Loadout;

#pragma endregion 
};