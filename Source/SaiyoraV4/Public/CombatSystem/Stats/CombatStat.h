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
	virtual void OnValueChanged(float const PreviousValue, int32 const PredictionID) override;
	
public:

	void SubscribeToStatChanged(FStatCallback const& Callback);
	void UnsubscribeFromStatChanged(FStatCallback const& Callback);
	bool IsInitialized() const { return bInitialized; }
	void Init(FStatInfo const& InitInfo, class UStatHandler* NewHandler);
	FGameplayTag GetStatTag() const { return StatTag; }
	bool ShouldReplicate() const { return bShouldReplicate; }
};

USTRUCT()
struct FCombatStat : public FFastArraySerializerItem
{
	GENERATED_BODY()

	void SubscribeToStatChanged(FStatCallback const& Callback);
	void UnsubscribeFromStatChanged(FStatCallback const& Callback);
	FGameplayTag GetStatTag() const { return StatTag; }
	bool ShouldReplicate() const { return bShouldReplicate; }
	bool IsInitialized() const { return bInitialized; }
	FCombatStat(FStatInfo const& InitInfo);
	float GetValue() const { return Value; }

private:
	
	UPROPERTY()
	FGameplayTag StatTag;
	float Value = 0.0f;
	float BaseValue = 0.0f;
	bool bHasMin = true;
	float Min = 0.0f;
	bool bHasMax = false;
	float Max = 0.0f;
	UPROPERTY()
	TArray<FCombatModifier> Modifiers;
	bool bShouldReplicate = false;
	bool bInitialized = false;
	FStatNotification OnStatChanged;
};

USTRUCT()
struct FCombatStatArray : FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FCombatStat> Items;
	UPROPERTY(NotReplicated)
	class UStatHandler* Handler;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
		return FFastArraySerializer::FastArrayDeltaSerialize<FCombatStat, FCombatStatArray>( Items, DeltaParms, *this );
    }
};

template<>
struct TStructOpsTypeTraits<FCombatStatArray> : public TStructOpsTypeTraitsBase2<FCombatStatArray>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};
