#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaiyoraCombatInterface.generated.h"

class UCombatComponent;
class UThreatHandler;
class UDamageHandler;
class UBuffHandler;
class UAbilityHandler;
class UCrowdControlHandler;
class UResourceHandler;
class UStatHandler;
class UCombatReactionComponent;

UINTERFACE(Blueprintable)
class USaiyoraCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class SAIYORAV4_API ISaiyoraCombatInterface
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
	UDamageHandler* GetDamageHandler() const;
	virtual UDamageHandler* GetDamageHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Buffs")
	UBuffHandler* GetBuffHandler() const;
	virtual UBuffHandler* GetBuffHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Abilities")
	UAbilityHandler* GetAbilityHandler() const;
	virtual UAbilityHandler* GetAbilityHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Crowd Control")
	UCrowdControlHandler* GetCrowdControlHandler() const;
	virtual UCrowdControlHandler* GetCrowdControlHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Resources")
	UResourceHandler* GetResourceHandler() const;
	virtual UResourceHandler* GetResourceHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Stats")
	UStatHandler* GetStatHandler() const;
	virtual UStatHandler* GetStatHandler_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Reaction")
	UCombatReactionComponent* GetReactionComponent() const;
	virtual UCombatReactionComponent* GetReactionComponent_Implementation() const { return nullptr; }
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Threat")
	UThreatHandler* GetThreatHandler() const;
	virtual UThreatHandler* GetThreatHandler_Implementation() const { return nullptr; }
};
