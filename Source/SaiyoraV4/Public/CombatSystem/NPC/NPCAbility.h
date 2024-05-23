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

#pragma region Core

protected:

	virtual void PostInitializeAbility_Implementation() override;
	virtual void OnServerTick_Implementation(const int32 TickNumber) override;
	virtual void AdditionalCastableUpdate(TArray<ECastFailReason>& AdditionalFailReasons) override;

private:

	UPROPERTY()
	ASaiyoraGameState* GameStateRef = nullptr;

#pragma endregion
#pragma region Tokens

public:

	bool UsesTokens() const { return bUseTokens; }
	int32 GetMaxTokens() const { return MaxTokens; }
	float GetTokenCooldownTime() const { return TokenCooldown; }

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

#pragma endregion
#pragma region Range

public:

	UFUNCTION()
	bool HasMinimumRange() const { return bHasMinimumRange; }
	UFUNCTION()
	float GetMinimumRange() const { return bHasMinimumRange ? MinimumRange : -1.0f; }
	UFUNCTION()
	bool HasMaximumRange() const { return bHasMaximumRange; }
	UFUNCTION()
	float GetMaximumRange() const { return bHasMaximumRange ? MaximumRange : -1.0f; }

private:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	bool bHasMinimumRange = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (EditCondition = "bHasMinimumRange", AllowPrivateAccess = "true"))
	float MinimumRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	bool bHasMaximumRange = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (EditCondition = "bHasMaximumRange", AllowPrivateAccess = "true"))
	float MaximumRange = 9999.0f;

	//These ranges can't actually be used for any kind of cast restriction, because we don't know what this ability's target is.
	// It could be threat target, or it could be furthest target, or lowest health target, or any other arbitrary target.

	//TODO: Could potentially do a range check on tick (yikes) using the FNPCTargetContext-derived structs.

#pragma endregion 
};
