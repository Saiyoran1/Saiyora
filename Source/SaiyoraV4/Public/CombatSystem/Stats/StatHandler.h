#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatStructs.h"
#include "BuffStructs.h"
#include "StatHandler.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UStatHandler : public UActorComponent
{
	GENERATED_BODY()
	
public:
	
	UStatHandler();
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

//Stats

public:

	UFUNCTION(BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool IsStatValid(const FGameplayTag StatTag) const;
	UFUNCTION(BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	float GetStatValue(const FGameplayTag StatTag) const;
	UFUNCTION(BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool IsStatModifiable(const FGameplayTag StatTag) const;
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void SubscribeToStatChanged(const FGameplayTag StatTag, const FStatCallback& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void UnsubscribeFromStatChanged(const FGameplayTag StatTag, const FStatCallback& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	FCombatModifierHandle AddStatModifier(const FGameplayTag StatTag, const FCombatModifier& Modifier);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void RemoveStatModifier(const FGameplayTag StatTag, const FCombatModifierHandle& ModifierHandle);
	void UpdateStatModifier(const FGameplayTag StatTag, const FCombatModifierHandle& ModifierHandle, const FCombatModifier& Modifier);
	
private:

	UPROPERTY(EditAnywhere, Category = "Stats")
	UDataTable* StatTemplate;
	UPROPERTY(EditAnywhere, Replicated, Category = "Stats")
	FCombatStatArray Stats;
};
