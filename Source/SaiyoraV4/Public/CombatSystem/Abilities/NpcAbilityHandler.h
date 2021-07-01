#pragma once

#include "CoreMinimal.h"
#include "CombatSystem/Abilities/AbilityHandler.h"
#include "NpcAbilityHandler.generated.h"

UCLASS()
class SAIYORAV4_API UNpcAbilityHandler : public UAbilityHandler
{
	GENERATED_BODY()

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	virtual bool GetIsCasting() const override;
	virtual float GetCurrentCastLength() const override;
	virtual float GetCastTimeRemaining() const override;
	virtual bool GetIsInterruptible() const override;
	virtual UCombatAbility* GetCurrentCastAbility() const override;
	virtual bool GetGlobalCooldownActive() const override;
	virtual float GetCurrentGlobalCooldownLength() const override;
	virtual float GetGlobalCooldownTimeRemaining() const override;

	virtual FCastEvent UseAbility(TSubclassOf<UCombatAbility> const AbilityClass) override;
	virtual FCancelEvent CancelCurrentCast() override;
	virtual FInterruptEvent InterruptCurrentCast(AActor* AppliedBy, UObject* InterruptSource, bool const bIgnoreRestrictions) override;

private:

	UPROPERTY(ReplicatedUsing = OnRep_CastingState)
	FCastingState CastingState;
	FGlobalCooldown GlobalCooldown;

	UFUNCTION()
	void OnRep_CastingState(FCastingState const& PreviousState);

	void StartGlobalCooldown(UCombatAbility* Ability);
	FTimerHandle GlobalCooldownHandle;
	UFUNCTION()
	void EndGlobalCooldown();

	void StartCast(UCombatAbility* Ability);
	UFUNCTION()
	void CompleteCast();
	UFUNCTION()
	void TickCast();
	void EndCast();
	FTimerHandle CastHandle;
	FTimerHandle TickHandle;

	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityTick(FCastEvent const& CastEvent, FCombatParameters const& Params);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityComplete(FCastEvent const& CastEvent);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityCancel(FCancelEvent const& CancelEvent, FCombatParameters const& Params);
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastAbilityInterrupt(FInterruptEvent const& InterruptEvent);
};
