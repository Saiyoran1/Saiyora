
#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StatStructs.h"
#include "SaiyoraObjects.h"
#include "CombatStat.generated.h"

UCLASS()
class SAIYORAV4_API UCombatStat : public UModifiableFloatValue
{
	GENERATED_BODY()

	FGameplayTag StatTag;
	bool bShouldReplicate = false;
	UPROPERTY()
	class UStatHandler* Handler;
	FStatNotification OnStatChanged;
public:
	void Init(FStatInfo const& InitInfo, class UStatHandler* Handler);
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SubscribeToStatChanged(FStatCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void UnsubscribeFromStatChanged(FStatCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stats")
	float GetStatValue() const { return GetValue(); }
};
