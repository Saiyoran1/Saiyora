#pragma once
#include "CoreMinimal.h"
#include "WeaponStructs.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

class AWeapon;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UWeaponComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapon")
	void SelectWeapon(const bool bPrimarySlot, const TSubclassOf<AWeapon> WeaponClass);
	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeapon* GetCurrentPrimaryWeapon() const { return CurrentPrimaryWeapon; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeapon* GetCurrentSecondaryWeapon() const { return CurrentSecondaryWeapon; }

	UPROPERTY(BlueprintAssignable)
	FOnWeaponChanged OnWeaponChanged;

private:

	UPROPERTY(EditAnywhere, Category = "Weapon")
	TSubclassOf<AWeapon> DefaultPrimaryWeaponClass;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bCanDualWield = false;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TSubclassOf<AWeapon> DefaultSecondaryWeaponClass;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPrimaryWeapon)
	AWeapon* CurrentPrimaryWeapon = nullptr;
	UFUNCTION()
	void OnRep_CurrentPrimaryWeapon(AWeapon* PreviousWeapon);
	UPROPERTY(ReplicatedUsing = OnRep_CurrentSecondaryWeapon)
	AWeapon* CurrentSecondaryWeapon = nullptr;
	UFUNCTION()
	void OnRep_CurrentSecondaryWeapon(AWeapon* PreviousWeapon);
};
