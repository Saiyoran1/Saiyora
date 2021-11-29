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
	bool bInitialized = false;
	bool bShouldReplicate = false;
	FStatNotification OnStatChanged;
	virtual void OnValueChanged(float const PreviousValue) override;
	
public:

	void SubscribeToStatChanged(FStatCallback const& Callback);
	void UnsubscribeFromStatChanged(FStatCallback const& Callback);
	bool IsInitialized() const { return bInitialized; }
	void Init(FStatInfo const& InitInfo, class UStatHandler* NewHandler);
	FGameplayTag GetStatTag() const { return StatTag; }
	bool ShouldReplicate() const { return bShouldReplicate; }
};
