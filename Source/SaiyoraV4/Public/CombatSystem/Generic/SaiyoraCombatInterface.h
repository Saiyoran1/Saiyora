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

//Component Getters
	
public:
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
	UDamageHandler* GetDamageHandler() const;
	virtual UDamageHandler* GetDamageHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Buffs")
	UBuffHandler* GetBuffHandler() const;
	virtual UBuffHandler* GetBuffHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Abilities")
	UAbilityComponent* GetAbilityComponent() const;
	virtual UAbilityComponent* GetAbilityComponent_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Crowd Control")
	UCrowdControlHandler* GetCrowdControlHandler() const;
	virtual UCrowdControlHandler* GetCrowdControlHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Resources")
	UResourceHandler* GetResourceHandler() const;
	virtual UResourceHandler* GetResourceHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Stats")
	UStatHandler* GetStatHandler() const;
	virtual UStatHandler* GetStatHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Plane")
	UCombatStatusComponent* GetCombatStatusComponent() const;
	virtual UCombatStatusComponent* GetCombatStatusComponent_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Threat")
	UThreatHandler* GetThreatHandler() const;
	virtual UThreatHandler* GetThreatHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movement")
	USaiyoraMovementComponent* GetCustomMovementComponent() const;
	virtual USaiyoraMovementComponent* GetCustomMovementComponent_Implementation() const { return nullptr; }

//Socket Getters

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
};
