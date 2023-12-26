#pragma once
#include "CoreMinimal.h"
#include "CombatAbility.h"
#include "SpecializationStructs.generated.h"

class UAncientTalent;
class UAncientSpecialization;
class UModernSpecialization;

//Data asset for global variables related to player abilities.
UCLASS(Blueprintable)
class SAIYORAV4_API UPlayerAbilityData : public UDataAsset
{
	GENERATED_BODY()

public:

	//Ancient specializations the player can choose from.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	TArray<TSubclassOf<UAncientSpecialization>> AncientSpecializations;
	//How many abilities an ancient spec has access to at once.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	int32 NumAncientAbilities = 6;
	//Modern specializations the player can choose from.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	TArray<TSubclassOf<UModernSpecialization>> ModernSpecializations;
	//How many spec-specific abilities a modern spec has access to at once.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	int32 NumModernSpecAbilities = 1;
	//How many abilities from the generic modern ability pool a modern spec has access to at once.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	int32 NumModernFlexAbilities = 4;
	//Abilities available for any modern spec to use in their flex slots.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	TArray<TSubclassOf<UCombatAbility>> ModernAbilityPool;
};

/*
 *
 * Ancient Spec Structs
 * 
 */

//Struct that represents a single talent row, with no state information.
USTRUCT(BlueprintType)
struct FAncientTalentRow
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UCombatAbility> BaseAbilityClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<UAncientTalent>> Talents;
};

//Smaller struct used for RPCing changes in talent selections.
USTRUCT(BlueprintType)
struct FAncientTalentSelection
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UCombatAbility> BaseAbility;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UAncientTalent> Selection;

	FAncientTalentSelection() {}
	FAncientTalentSelection(const TSubclassOf<UCombatAbility> InBase, const TSubclassOf<UAncientTalent> InSelection) :
		BaseAbility(InBase), Selection(InSelection) {}
};

//Full struct including the talent row information and the current selection.
USTRUCT(BlueprintType)
struct FAncientTalentChoice : public FFastArraySerializerItem
{
	GENERATED_BODY()

	void PostReplicatedAdd(const struct FAncientTalentSet& InArraySerializer);
	void PostReplicatedChange(const struct FAncientTalentSet& InArraySerializer);

	UPROPERTY(BlueprintReadOnly)
	FAncientTalentRow TalentRow;
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UAncientTalent> CurrentSelection = nullptr;
	UPROPERTY(NotReplicated)
	UAncientTalent* ActiveTalent = nullptr;

	FAncientTalentChoice() {}
	FAncientTalentChoice(const FAncientTalentRow& InRow) : TalentRow(InRow) {}
};

//Set of talent choices for a spec, for fast array replication in the spec object.
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

//Layout used by the UI to map keybinds to talent rows for a given spec.
USTRUCT(BlueprintType)
struct FAncientSpecLayout
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UAncientSpecialization> Spec;
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FAncientTalentChoice> Talents;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAncientSpecChangeNotification, UAncientSpecialization*, PreviousSpec, UAncientSpecialization*, NewSpec);

/*
 *
 * Modern Spec Structs
 * 
 */

//Enum to describe what kind of abilities can be selected for a slot in a modern spec loadout.
UENUM(BlueprintType)
enum class EModernSlotType : uint8
{
	None,
	//Weapon slots have one set ability class for the spec that can't be changed.
	Weapon,
	//Spec slots can only hold abilities that are from a spec-specific pool.
	Spec,
	//Flex slots can hold abilities from the general modern ability pool, available to all specs.
	Flex
};

//Struct for a modern talent selection, including the slot type.
USTRUCT(BlueprintType)
struct FModernTalentChoice
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	EModernSlotType SlotType = EModernSlotType::Flex;
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UCombatAbility> Selection;

	FModernTalentChoice() {}
	FModernTalentChoice(const EModernSlotType InSlotType, const TSubclassOf<UCombatAbility> AbilityClass = nullptr)
		: SlotType(InSlotType), Selection(AbilityClass) {}

	FORCEINLINE bool operator==(const FModernTalentChoice& Other) const { return Other.SlotType == SlotType && Other.Selection == Selection; }
};

//Struct for the UI that maps keybinds to modern ability slots.
USTRUCT(BlueprintType)
struct FModernSpecLayout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UModernSpecialization> Spec;
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FModernTalentChoice> Talents;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FModernSpecChangeNotification, UModernSpecialization*, PreviousSpec, UModernSpecialization*, NewSpec);