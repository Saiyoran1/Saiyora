#pragma once
#include "CoreMinimal.h"
#include "SpecializationStructs.generated.h"

class UCombatAbility;
class UAncientTalent;
class UAncientSpecialization;
class UModernSpecialization;

USTRUCT(BlueprintType)
struct FAncientTalentSelection
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UCombatAbility> BaseAbility;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UAncientTalent> Selection;
};

USTRUCT(BlueprintType)
struct FAncientTalentChoice : public FFastArraySerializerItem
{
	GENERATED_BODY()

	void PostReplicatedAdd(const struct FAncientTalentSet& InArraySerializer);
	void PostReplicatedChange(const struct FAncientTalentSet& InArraySerializer);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization")
	TSubclassOf<UCombatAbility> BaseAbility;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Specialization")
	TArray<TSubclassOf<UAncientTalent>> Talents;
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UAncientTalent> CurrentSelection = nullptr;
	UPROPERTY(NotReplicated)
	UAncientTalent* ActiveTalent = nullptr;
};

USTRUCT(BlueprintType)
struct FAncientTalentSet : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(NotReplicated)
	UAncientSpecialization* OwningSpecialization = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Talents"))
	TArray<FAncientTalentChoice> Items;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FAncientTalentChoice, FAncientTalentSet>( Items, DeltaParms, *this );
	}
};

template<>
struct TStructOpsTypeTraits<FAncientTalentSet> : public TStructOpsTypeTraitsBase2<FAncientTalentSet>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};

USTRUCT(BlueprintType)
struct FModernTalentChoice : public FFastArraySerializerItem
{
	GENERATED_BODY()

	void PostReplicatedAdd(const struct FModernTalentSet& InArraySerializer);
	void PostReplicatedChange(const struct FModernTalentSet& InArraySerializer);

	UPROPERTY(BlueprintReadWrite)
	int32 SlotNumber = 0;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UCombatAbility> Selection;

	FModernTalentChoice() {}
	FModernTalentChoice(const int32 Slot, const TSubclassOf<UCombatAbility> AbilityClass) : SlotNumber(Slot), Selection(AbilityClass) {}
};

USTRUCT(BlueprintType)
struct FModernTalentSet : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(NotReplicated)
	UModernSpecialization* OwningSpecialization = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Talents"))
	TArray<FModernTalentChoice> Items;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FModernTalentChoice, FModernTalentSet>( Items, DeltaParms, *this );
	}
};

template<>
struct TStructOpsTypeTraits<FModernTalentSet> : public TStructOpsTypeTraitsBase2<FModernTalentSet>
{
	enum 
	{
		WithNetDeltaSerializer = true,
	};
};

UCLASS(Blueprintable)
class SAIYORAV4_API UAbilityPool : public UDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	TArray<TSubclassOf<UCombatAbility>> AbilityClasses;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAncientSpecChangeNotification, UAncientSpecialization*, PreviousSpec, UAncientSpecialization*, NewSpec);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAncientTalentChangeNotification, TSubclassOf<UCombatAbility>, BaseAbility, TSubclassOf<UAncientTalent>, PreviousTalent, TSubclassOf<UAncientTalent>, NewTalent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FModernSpecChangeNotification, UModernSpecialization*, PreviousSpec, UModernSpecialization*, NewSpec);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FModernTalentChangeNotification, const int32, SlotNumber, TSubclassOf<UCombatAbility>, PreviousAbility, TSubclassOf<UCombatAbility>, NewAbility);