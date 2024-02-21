#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "NPCStructs.h"
#include "NPCAbility.generated.h"

class UCombatGroup;
class UThreatHandler;

UCLASS()
class SAIYORAV4_API UNPCAbility : public UCombatAbility
{
	GENERATED_BODY()

public:

	int32 GetMaxTokens() const { return MaxTokens; }
	float GetTokenCooldownTime() const { return TokenCooldown; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	bool bUseTokens = false;
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (EditCondition = "bUseTokens"))
	int32 MaxTokens = 1;
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (EditCondition = "bUseTokens"))
	float TokenCooldown = 0.0f;

	virtual void PostInitializeAbility_Implementation() override;
	virtual void AdditionalCastableUpdate(TArray<ECastFailReason>& AdditionalFailReasons) override;
};
