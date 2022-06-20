#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CombatStructs.h"
#include "StatStructs.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStatCallback, FGameplayTag const&, StatTag, float const, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatNotification, FGameplayTag const&, StatTag, float const, NewValue);

USTRUCT()
struct FStatInfo : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Stat", meta = (Categories = "Stat"))
    FGameplayTag StatTag;
    UPROPERTY(EditAnywhere, Category = "Stat", meta = (ClampMin = "0"))
    float DefaultValue = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Stat")
    bool bModifiable = false;
    UPROPERTY(EditAnywhere, Category = "Stat")
    bool bCappedLow = false;
    UPROPERTY(EditAnywhere, Category = "Stat", meta = (ClampMin = "0"))
    float MinClamp = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Stat")
    bool bCappedHigh = false;
    UPROPERTY(EditAnywhere, Category = "Stat", meta = (ClampMin = "0"))
    float MaxClamp = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Stat")
    bool bShouldReplicate = false;
};

USTRUCT()
struct FCombatStat : public FFastArraySerializerItem
{
    GENERATED_BODY()

    FCombatStat(FStatInfo const& InitInfo);
    FCombatStat() {}
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

    void PostReplicatedAdd(const struct FCombatStatArray& InArraySerializer);
    void PostReplicatedChange(const struct FCombatStatArray& InArraySerializer);

private:

    UPROPERTY()
    float Value = 0.0f;
    void RecalculateValue();
    UPROPERTY()
    FStatInfo Defaults;
    TMap<UBuff*, FCombatModifier> Modifiers;
    bool bInitialized = false;
    FStatNotification OnStatChanged;
};

USTRUCT()
struct FCombatStatArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FCombatStat> Items;
    UPROPERTY(NotReplicated)
    class UStatHandler* Handler = nullptr;
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