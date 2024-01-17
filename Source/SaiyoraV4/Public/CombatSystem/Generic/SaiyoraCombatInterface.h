#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaiyoraCombatInterface.generated.h"

class UThreatHandler;
class UDamageHandler;
class UBuffHandler;
class UAbilityComponent;
class UCrowdControlHandler;
class UResourceHandler;
class UStatHandler;
class UCombatStatusComponent;
class USaiyoraMovementComponent;

UINTERFACE(Blueprintable)
class USaiyoraCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class SAIYORAV4_API ISaiyoraCombatInterface
{
	GENERATED_BODY()

#pragma region Component Getters

	/*
	 * These getters are for getting the various components involved in combat.
	 * The default implementation calls FindComponentByClass the first time, and caches off the result, returning it on subsequent calls.
	 * This should be overridden in derived classes to directly return the component pointer or nullptr, but I forget to do that sometimes.
	 */
	
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
	UDamageHandler* GetDamageHandler() const;
	virtual UDamageHandler* GetDamageHandler_Implementation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Buffs")
	UBuffHandler* GetBuffHandler() const;
	virtual UBuffHandler* GetBuffHandler_Implementation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Abilities")
	UAbilityComponent* GetAbilityComponent() const;
	virtual UAbilityComponent* GetAbilityComponent_Implementation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Crowd Control")
	UCrowdControlHandler* GetCrowdControlHandler() const;
	virtual UCrowdControlHandler* GetCrowdControlHandler_Implementation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Resources")
	UResourceHandler* GetResourceHandler() const;
	virtual UResourceHandler* GetResourceHandler_Implementation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Stats")
	UStatHandler* GetStatHandler() const;
	virtual UStatHandler* GetStatHandler_Implementation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Plane")
	UCombatStatusComponent* GetCombatStatusComponent() const;
	virtual UCombatStatusComponent* GetCombatStatusComponent_Implementation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Threat")
	UThreatHandler* GetThreatHandler() const;
	virtual UThreatHandler* GetThreatHandler_Implementation() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movement")
	USaiyoraMovementComponent* GetCustomMovementComponent() const;
	virtual USaiyoraMovementComponent* GetCustomMovementComponent_Implementation() const;

private:
	
	mutable bool bCachedDamageHandler = false;
	mutable UDamageHandler* CachedDamageHandler = nullptr;

	mutable bool bCachedBuffHandler = false;
	mutable UBuffHandler* CachedBuffHandler = nullptr;

	mutable bool bCachedAbilityComponent = false;
	mutable UAbilityComponent* CachedAbilityComponent = nullptr;

	mutable bool bCachedCrowdControlHandler = false;
	mutable UCrowdControlHandler* CachedCrowdControlHandler = nullptr;

	mutable bool bCachedResourceHandler = false;
	mutable UResourceHandler* CachedResourceHandler = nullptr;

	mutable bool bCachedStatHandler = false;
	mutable UStatHandler* CachedStatHandler = nullptr;

	mutable bool bCachedCombatStatusComponent = false;
	mutable UCombatStatusComponent* CachedCombatStatusComponent = nullptr;

	mutable bool bCachedThreatHandler = false;
	mutable UThreatHandler* CachedThreatHandler = nullptr;

	mutable bool bCachedMovementComponent = false;
	mutable USaiyoraMovementComponent* CachedMovementComponent = nullptr;

#pragma endregion 

#pragma region Socket Getters

	/*
	 * These are getters for various component sockets required for combat.
	 * These include sockets for where visuals (such as a health bar or floating damage text) should display  for an actor,
	 * as well as sockets that are used to predict and validate third-person aiming, or place weapons correctly.
	 */

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
	USceneComponent* GetFloatingHealthSocket(FName& SocketName) const;
	virtual USceneComponent* GetFloatingHealthSocket_Implementation(FName& SocketName) const { SocketName = NAME_None; return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
	USceneComponent* GetDamageEffectSocket(FName& SocketName) const;
	virtual USceneComponent* GetDamageEffectSocket_Implementation(FName& SocketName) const { SocketName = NAME_None; return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Abilities")
	USceneComponent* GetAbilityOriginSocket(FName& SocketName) const;
	virtual USceneComponent* GetAbilityOriginSocket_Implementation(FName& SocketName) const { SocketName = NAME_None; return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Abilities")
	USceneComponent* GetAimSocket(FName& SocketName) const;
	virtual USceneComponent* GetAimSocket_Implementation(FName& SocketName) const { SocketName = NAME_None; return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	USceneComponent* GetPrimaryWeaponSocket(FName& SocketName) const;
	virtual USceneComponent* GetPrimaryWeaponSocket_Implementation(FName& SocketName) const { SocketName = NAME_None; return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	USceneComponent* GetSecondaryWeaponSocket(FName& SocketName) const;
	virtual USceneComponent* GetSecondaryWeaponSocket_Implementation(FName& SocketName) const { SocketName = NAME_None; return nullptr; }

#pragma endregion
	
};
