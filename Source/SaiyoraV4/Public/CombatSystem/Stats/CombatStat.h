#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StatStructs.h"
#include "SaiyoraObjects.h"
#include "CombatStat.generated.h"

/*UCLASS()
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
};*/

USTRUCT()
struct FCombatStat : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FCombatStat(FStatInfo const& InitInfo);
	void SubscribeToStatChanged(FStatCallback const& Callback);
	void UnsubscribeFromStatChanged(FStatCallback const& Callback);
	void AddModifier(FCombatModifier const& Modifier);
	void RemoveModifier(UBuff* Source);
	
	FGameplayTag GetStatTag() const { return Defaults.StatTag; }
	bool ShouldReplicate() const { return Defaults.bShouldReplicate; }
	bool IsModifiable() const { return Defaults.bModifiable; }
	bool IsInitialized() const { return bInitialized; }
	float GetValue() const { return Value; }
	float GetDefaultValue() const { return Defaults.DefaultValue; }
	bool HasMinimum() const { return Defaults.bCappedLow; }
	float GetMinimum() const { return Defaults.MinClamp; }
	bool HasMaximum() const { return Defaults.bCappedHigh; }
	float GetMaximum() const { return Defaults.MaxClamp; }
	void GetModifiers(TArray<FCombatModifier>& OutMods) const { OutMods.Append(Modifiers); }

	void PostReplicatedAdd(const struct FCombatStatArray& InArraySerializer);
	void PostReplicatedChange(const struct FCombatStatArray& InArraySerializer);

private:

	float Value = 0.0f;
	void RecalculateValue();
	UPROPERTY()
	FStatInfo Defaults;
	UPROPERTY()
	TArray<FCombatModifier> Modifiers;
	UPROPERTY()
	int32 LastPredictionID = 0;
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
	TMultiMap<FGameplayTag, FStatCallback> PendingSubscriptions;

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
