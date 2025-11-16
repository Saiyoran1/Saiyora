#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "FireWeapon.generated.h"

class UModernCrosshair;
class AWeapon;

USTRUCT(BlueprintType)
struct FAutoReloadState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsAutoReloading = false;
	UPROPERTY()
	float StartTime = 0.0f;
	UPROPERTY()
	float EndTime = 0.0f;
};

USTRUCT()
struct FShotVerificationInfo
{
	GENERATED_BODY()
	
	float Timestamp = 0.0f;
	float CooldownAtTime = 0.0f;

	FShotVerificationInfo() {}
	FShotVerificationInfo(const float InTimestamp, const float InCooldown)
		: Timestamp(InTimestamp), CooldownAtTime(InCooldown) {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponSpreadNotification, const float, NewSpread);

UCLASS(Abstract, Blueprintable)
class SAIYORAV4_API UFireWeapon : public UCombatAbility, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UFireWeapon();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma region Firing

public:

	virtual float GetCooldownLength() override;

protected:

	virtual void OnPredictedTick_Implementation(const int32 TickNumber) override;
	virtual void OnServerTick_Implementation(const int32 TickNumber) override;
	virtual void OnSimulatedTick_Implementation(const int32 TickNumber) override;

private:
	
	FTimerHandle FireDelayTimer;
	UFUNCTION()
	void EndFireDelay();

#pragma endregion 
#pragma region Ammo

public:

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAutoReloading() const { return AutoReloadState.bIsAutoReloading; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAutoReloadStartTime() const { return AutoReloadState.StartTime; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAutoReloadEndTime() const { return AutoReloadState.EndTime; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAutoReloadTimeRemaining() const { return AutoReloadState.bIsAutoReloading ? FMath::Max(0.0f, AutoReloadState.EndTime - GetWorld()->GetGameState()->GetServerWorldTimeSeconds()) : 0.0f; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetAutoReloadTime() const { return AutoReloadTime; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCombatAbility> ReloadAbilityClass;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bAutoReloadXPlane = true;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float AutoReloadTime = 5.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FResourceInitInfo AmmoInitInfo;
	
	UPROPERTY(Replicated)
	FAutoReloadState AutoReloadState;
	FTimerHandle AutoReloadHandle;
	UFUNCTION()
	void StartAutoReloadOnPlaneSwap(const ESaiyoraPlane PreviousPlane, const ESaiyoraPlane NewPlane, UObject* Source);
	UFUNCTION()
	void FinishAutoReload();

	UPROPERTY()
	UResourceHandler* ResourceHandlerRef = nullptr;
	UPROPERTY()
	UResource* AmmoResourceRef = nullptr;

#pragma endregion 
#pragma region Weapon
	
public:

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeapon* GetWeapon() const { return Weapon; }

protected:

	virtual void PostInitializeAbility_Implementation() override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeapon> WeaponClass;
	
	UPROPERTY()
	AWeapon* Weapon = nullptr;

#pragma endregion 
#pragma region Verification

private:

	int32 MaxShotsToVerify = 10;
	int32 MinShotsToVerify = 5;
	static constexpr float FIRERATEERRORMARGIN = 0.1f;
	TArray<FShotVerificationInfo> CurrentClipTimestamps;
	void AddNewShotForVerification(const float Timestamp);
	void VerifyFireRate();
	void EndFireRateVerificationWait() { DeactivateCastRestriction(FSaiyoraCombatTags::Get().AbilityFireRateRestriction); }
	void ResetVerificationWindow() { CurrentClipTimestamps.Empty(); }
	FTimerHandle ServerFireRateTimer;
	UFUNCTION()
	void OnReloadComplete(const FAbilityEvent& AbilityEvent);

#pragma endregion 
#pragma region Crosshair

public:

	TSubclassOf<UModernCrosshair> GetCrosshairClass() const { return CrosshairClass; }
	bool UsesSpread() const { return IsValid(SpreadCurve); }
	float GetOptimalDisplayRange() const { return OptimalDisplayRange; }
	float GetCurrentSpreadAngle() const { return CurrentSpreadAngle; }
	FWeaponSpreadNotification OnSpreadChanged;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	TSubclassOf<UModernCrosshair> CrosshairClass;

	//The curve determining spread in degrees as a function of a normalized value that is increased on every shot and decays over time.
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	UCurveFloat* SpreadCurve = nullptr;
	//How far along the spread curve each shot moves. The spread curve should be normalized from 0-1.
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	float SpreadAlphaPerShot = 0.1f;
	//Once spread decay starts, this is how long it would take to decay from max angle and alpha to 0.
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	float SpreadDecayLength = 1.0f;
	//The time between the weapon firing and spread beginning to decay back to 0.
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	float SpreadDecayDelay = 0.0f;
	//Used for displaying spread on the UI. Basically a good "average" range for the weapon.
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	float OptimalDisplayRange = 2000.0f;

	//Called when the weapon is fired to increase the spread alpha and recalculate the spread angle.
	//Also resets the spread decay delay and cancels the current decaying of spread.
	void UpdateSpreadForShot();
	//Once the spread decay delay time has been reached, this sets up spread decay as a linear function from decay start values.
	void StartSpreadDecay();
	//Called to zero out decay variables and cancel spread decay. Called when the weapon is fired or when decay finishes.
	void EndSpreadDecay();
	//Called every frame to calculate whether spread should be decaying or not, and recalculate spread alpha and angle if so.
	void TickSpreadDecay(const float DeltaTime);
	//Current spread alpha. This is the value fed into the spread curve to generate CurrentSpreadAngle when spread is increasing.
	float CurrentSpreadAlpha = 0.0f;
	//Current spread angle. This is actual angle read by the crosshair and applied to shots.
	float CurrentSpreadAngle = 0.0f;
	//Time remaining until spread decay can begin after firing a shot.
	float SpreadDecayDelayRemaining = 0.0f;
	//Whether we are currently decaying spread.
	bool bDecayingSpread = false;
	//Cached start value for spread alpha, so we can decay it linearly.
	float DecayStartAlpha = 0.0f;
	//Cached start value for spread angle, so we can decay it linearly.
	float DecayStartAngle = 0.0f;
	//The calculated length of time it will take from the start of spread decay until both angle and alpha would be zero.
	//Calculated using the assumption that SpreadDecayLength is how long it would take to decay from an alpha of 1.
	float CurrentDecayLength = 0.0f;
	//Time since spread decay started, used to lerp alpha and angle as decay happens.
	float TimeDecaying = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	UCurveFloat* RecoilCurve = nullptr;

#pragma endregion
#pragma region Tick

public:

	//Mark that instances of FireWeapon should only tick depending on certain criteria, not always.
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	//Only tick FireWeapon abilities if they have a valid spread or recoil curve. The only things we do in Tick are managing these.
	virtual bool IsTickable() const override;
	//Required function from the FTickableGameObject base class.
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UFireWeapon, STATGROUP_Tickables); }
	virtual void Tick(float DeltaTime) override { TickSpreadDecay(DeltaTime); }
	
#pragma endregion 
};
