#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatStructs.h"
#include "BuffStructs.h"
#include "StatHandler.generated.h"

//The StatHandler component handles any modifiable combat values, such as damage modifiers, max health, movement modifiers, threat modifiers, etc.

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UStatHandler : public UActorComponent
{
	GENERATED_BODY()

#pragma region Init
	
public:
	
	UStatHandler();
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

#pragma endregion 
#pragma region Stats

public:

	//Whether this stat handler actually has the specified stat. Invalid stats will always return -1 as a value.
	UFUNCTION(BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool IsStatValid(const FGameplayTag StatTag) const;
	//Get the current value of a stat. Returns -1 for invalid stats. Returns default value if on a client and the stat hasn't replicated.
	UFUNCTION(BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	float GetStatValue(const FGameplayTag StatTag) const;

	//Check if a given stat will be affected by modifiers.
	UFUNCTION(BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool IsStatModifiable(const FGameplayTag StatTag) const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	FCombatModifierHandle AddStatModifier(const FGameplayTag StatTag, const FCombatModifier& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void RemoveStatModifier(const FGameplayTag StatTag, const FCombatModifierHandle& ModifierHandle);
	//Called to update an existing stat modifier, usually from a buff's stack count changing.
	void UpdateStatModifier(const FGameplayTag StatTag, const FCombatModifierHandle& ModifierHandle, const FCombatModifier& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void SubscribeToStatChanged(const FGameplayTag StatTag, const FStatCallback& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void UnsubscribeFromStatChanged(const FGameplayTag StatTag, const FStatCallback& Callback);
	
private:

	//An optional template to get stat defaults from. This will empty and repopulate the CombatStats array when changed.
	UPROPERTY(EditAnywhere, Category = "Stats")
	UDataTable* StatTemplate;
	//Stat information for this actor.
	UPROPERTY(EditAnywhere, Category = "Stats")
	TArray<FCombatStat> CombatStats;
	//The runtime version of stats, using the CombatStats default values and replicating the modified values to clients.
	UPROPERTY(Replicated)
	FCombatStatArray Stats;

#pragma endregion 
};
