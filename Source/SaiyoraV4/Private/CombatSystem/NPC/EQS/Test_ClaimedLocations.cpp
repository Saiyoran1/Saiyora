#include "Test_ClaimedLocations.h"
#include "NPCSubsystem.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

UTest_ClaimedLocations::UTest_ClaimedLocations()
{
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	Cost = EEnvTestCost::Low;
	TestPurpose = EEnvTestPurpose::Score;
}

void UTest_ClaimedLocations::RunTest(FEnvQueryInstance& QueryInstance) const
{
	const UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!IsValid(QueryOwner))
	{
		return;
	}
	const UActorComponent* OwnerAsComponent = Cast<UActorComponent>(QueryOwner);
	if (IsValid(OwnerAsComponent))
	{
		QueryOwner = OwnerAsComponent->GetOwner();
	}
	const AActor* OwnerActor = Cast<AActor>(QueryOwner);
	if (!IsValid(OwnerActor))
	{
		return;
	}
	const UNPCSubsystem* NPCSubsystem = QueryOwner->GetWorld()->GetSubsystem<UNPCSubsystem>();
	if (!IsValid(NPCSubsystem))
	{
		return;
	}

	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
	const float MinThresholdValue = FloatValueMin.GetValue();

	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	const float MaxThresholdValue = FloatValueMax.GetValue();
	
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector Location = GetItemLocation(QueryInstance, It.GetIndex());
		const float Score = NPCSubsystem->GetScorePenaltyForLocation(OwnerActor, Location);
		It.SetScore(TestPurpose, FilterType, Score, MinThresholdValue, MaxThresholdValue);
	}
}