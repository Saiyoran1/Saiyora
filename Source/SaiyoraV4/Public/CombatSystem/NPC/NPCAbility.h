#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "NPCStructs.h"
#include "NPCAbility.generated.h"

class UNPCSubsystem;
class UCombatGroup;
class UThreatHandler;

UCLASS()
class SAIYORAV4_API UNPCAbility : public UCombatAbility
{
	GENERATED_BODY()

#pragma region Core

protected:

	//Override post-init because we need to setup our NPCSubsystem ref to be able to request and return ability tokens.
	virtual void PostInitializeAbility_Implementation() override;
	//Override castable update because we may need to require an ability token to be available to be castable.
	virtual void AdditionalCastableUpdate(TArray<ECastFailReason>& AdditionalFailReasons) override;

private:

	//Subsystem that handles some AI behaviors, including ability tokens.
	UPROPERTY()
	UNPCSubsystem* NPCSubsystemRef = nullptr;

#pragma endregion
#pragma region Tokens

public:

	//Whether this ability requires ability tokens to be used.
	bool UsesTokens() const { return bUseTokens; }
	//How many tokens for this ability can be available at once. Used to initialize a pool of tokens when the first instance of this ability initializes.
	int32 GetMaxTokens() const { return MaxTokens; }
	//Gets how long a token used by this ability goes on cooldown after it is returned. This controls how frequently NPCs can use this ability.
	float GetTokenCooldownTime() const { return TokenCooldown; }

protected:

	//Override cast start to claim an ability token if necessary.
	virtual void OnServerCastStart_Implementation() override;
	//Override cast end to return a claimed ability token if necessary.
	virtual void OnServerCastEnd_Implementation() override;

private:

	//Whether this ability requires an NPC Ability Token to be cast.
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	bool bUseTokens = false;
	//The max number of tokens available for this class of ability at one time, globally.
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (EditCondition = "bUseTokens"))
	int32 MaxTokens = 1;
	//How long a token goes on cooldown and is not usable by other NPCs once one NPC has claimed it to use an ability.
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (EditCondition = "bUseTokens"))
	float TokenCooldown = 0.0f;

	//Delegate for updating castable status when token availability changes.
	FAbilityTokenCallback TokenCallback;
	//Called when token availability for this class changes, to update castable status.
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
