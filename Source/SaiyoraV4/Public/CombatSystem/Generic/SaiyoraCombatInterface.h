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
class UPlaneComponent;
class UFactionComponent;
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
	UPlaneComponent* GetPlaneComponent() const;
	virtual UPlaneComponent* GetPlaneComponent_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Faction")
	UFactionComponent* GetFactionComponent() const;
	virtual UFactionComponent* GetFactionComponent_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Threat")
	UThreatHandler* GetThreatHandler() const;
	virtual UThreatHandler* GetThreatHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movement")
	USaiyoraMovementComponent* GetCustomMovementComponent() const;
	virtual USaiyoraMovementComponent* GetCustomMovementComponent_Implementation() const { return nullptr; }

//Socket Getters

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
	USceneComponent* GetFloatingHealthSocket(FName& SocketName) const;
	virtual USceneComponent* GetFloatingHealthSocket_Implementation(FName& SocketName) const { SocketName = FName(TEXT("")); return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
	USceneComponent* GetDamageEffectSocket(FName& SocketName) const;
	virtual USceneComponent* GetDamageEffectSocket_Implementation(FName& SocketName) const { SocketName = FName(TEXT("")); return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Abilities")
	USceneComponent* GetAbilityOriginSocket(FName& SocketName) const;
	virtual USceneComponent* GetAbilityOriginSocket_Implementation(FName& SocketName) const { SocketName = FName(TEXT("")); return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Abilities")
	USceneComponent* GetAimSocket(FName& SocketName) const;
	virtual USceneComponent* GetAimSocket_Implementation(FName& SocketName) const { SocketName = FName(TEXT("")); return nullptr; }
};
