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
	
	void AddStatModifier(const FGameplayTag StatTag, const FCombatModifier& Modifier);
	void RemoveStatModifier(const FGameplayTag StatTag, const UBuff* Source);	
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Stat")
	UDataTable* InitialStats;
	TMap<FGameplayTag, FStatInfo> StatDefaults;
	UPROPERTY(Replicated)
	FCombatStatArray ReplicatedStats;
	TMap<FGameplayTag, FCombatStat> NonReplicatedStats;
	FGameplayTagContainer StaticStats;
};
