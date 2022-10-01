#pragma once
#include "CoreMinimal.h"
#include "SpecializationStructs.generated.h"

class UCombatAbility;
class UAncientTalent;
class UAncientSpecialization;

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

USTRUCT()
struct FAncientTalentSet : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(NotReplicated)
	UAncientSpecialization* OwningSpecialization = nullptr;
	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Talents"))
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAncientSpecChangeNotification, UAncientSpecialization*, PreviousSpec, UAncientSpecialization*, NewSpec);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTalentChangeNotification, TSubclassOf<UCombatAbility>, BaseAbility, TSubclassOf<UAncientTalent>, PreviousTalent, TSubclassOf<UAncientTalent>, NewTalent);