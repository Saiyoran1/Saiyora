#pragma once
#include "CoreMinimal.h"
#include "SpecializationStructs.generated.h"

class UCombatAbility;
class UAncientTalent;
class UAncientSpecialization;
class UModernSpecialization;

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

//Smaller struct used for RPCs and changing talents.
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

//Full struct detailing both base/possible talents, as well as the current state of talent selection.
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

//Set of talent choices for a spec, for fast array replication.
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

//Layout used by the UI as a combo of talent selections and keybind selections for a spec.
USTRUCT(BlueprintType)
struct FAncientSpecLayout
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UAncientSpecialization> Spec;
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FAncientTalentChoice> Talents;
};

//Enum to describe what kind of abilities can be selected for a slot in a modern spec.
UENUM(BlueprintType)
enum class EModernSlotType : uint8
{
	None,
	Weapon,
	Spec,
	Flex
};

//The slot information for a given selection in a modern spec.
USTRUCT(BlueprintType)
struct FModernTalentChoice : public FFastArraySerializerItem
{
	GENERATED_BODY()

	void PostReplicatedAdd(const struct FModernTalentSet& InArraySerializer);
	void PostReplicatedChange(const struct FModernTalentSet& InArraySerializer);

	UPROPERTY(BlueprintReadOnly)
	int32 SlotNumber = 0;
	UPROPERTY(BlueprintReadOnly)
	EModernSlotType SlotType = EModernSlotType::Flex;
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UCombatAbility> Selection;

	FModernTalentChoice() {}
	FModernTalentChoice(const int32 Slot, const EModernSlotType InSlotType, const TSubclassOf<UCombatAbility> AbilityClass = nullptr) : SlotNumber(Slot), SlotType(InSlotType), Selection(AbilityClass) {}
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

USTRUCT(BlueprintType)
struct FModernSpecLayout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UModernSpecialization> Spec;
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FModernTalentChoice> Talents;
};

UCLASS(Blueprintable)
class SAIYORAV4_API UPlayerAbilityData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	TArray<TSubclassOf<UAncientSpecialization>> AncientSpecializations;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	int32 NumAncientAbilities = 6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	TArray<TSubclassOf<UModernSpecialization>> ModernSpecializations;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	int32 NumModernSpecAbilities = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	int32 NumModernFlexAbilities = 4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Specialization")
	TArray<TSubclassOf<UCombatAbility>> ModernAbilityPool;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAncientSpecChangeNotification, UAncientSpecialization*, PreviousSpec, UAncientSpecialization*, NewSpec);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAncientTalentChangeNotification, TSubclassOf<UCombatAbility>, BaseAbility, TSubclassOf<UAncientTalent>, PreviousTalent, TSubclassOf<UAncientTalent>, NewTalent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FModernSpecChangeNotification, UModernSpecialization*, PreviousSpec, UModernSpecialization*, NewSpec);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FModernTalentChangeNotification, const int32, SlotNumber, TSubclassOf<UCombatAbility>, PreviousAbility, TSubclassOf<UCombatAbility>, NewAbility);