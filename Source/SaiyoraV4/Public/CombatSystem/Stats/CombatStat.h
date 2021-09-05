
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
	UPROPERTY()
	class UStatHandler* Handler;
	FStatNotification OnStatChanged;
	bool bInitialized = false;
	bool bShouldReplicate = false;
	FFloatValueCallback BroadcastCallback;
	void BroadcastStatChanged(float const Previous, float const New);
public:
	bool IsInitialized() const { return bInitialized; }
	void Init(FStatInfo const& InitInfo, class UStatHandler* NewHandler);
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SubscribeToStatChanged(FStatCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void UnsubscribeFromStatChanged(FStatCallback const& Callback);
	FGameplayTag GetStatTag() const { return StatTag; }
	bool ShouldReplicate() const { return bShouldReplicate; }
};
