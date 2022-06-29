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
	void SelectWeapon(const TSubclassOf<AWeapon> WeaponClass);
	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	UPROPERTY(BlueprintAssignable)
	FOnWeaponChanged OnWeaponChanged;

private:

	UPROPERTY(EditAnywhere, Category = "Weapon")
	TSubclassOf<AWeapon> DefaultWeaponClass;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	AWeapon* CurrentWeapon;
	UFUNCTION()
	void OnRep_CurrentWeapon(AWeapon* PreviousWeapon);
};
