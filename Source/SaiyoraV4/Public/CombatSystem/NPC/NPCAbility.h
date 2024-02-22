#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "NPCStructs.h"
#include "NPCAbility.generated.h"

class ASaiyoraGameState;
class UCombatGroup;
class UThreatHandler;

UCLASS()
class SAIYORAV4_API UNPCAbility : public UCombatAbility
{
	GENERATED_BODY()

public:

	bool UsesTokens() const { return bUseTokens; }
	int32 GetMaxTokens() const { return MaxTokens; }
	float GetTokenCooldownTime() const { return TokenCooldown; }

protected:

	virtual void PostInitializeAbility_Implementation() override;
	virtual void OnServerTick_Implementation(const int32 TickNumber) override;
	virtual void AdditionalCastableUpdate(TArray<ECastFailReason>& AdditionalFailReasons) override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	bool bUseTokens = false;
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (EditCondition = "bUseTokens"))
	int32 MaxTokens = 1;
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (EditCondition = "bUseTokens"))
	float TokenCooldown = 0.0f;
	
	FAbilityTokenCallback TokenCallback;
	UFUNCTION()
	void OnTokenAvailabilityChanged(const bool bAvailable) { UpdateCastable(); }

	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;
};
