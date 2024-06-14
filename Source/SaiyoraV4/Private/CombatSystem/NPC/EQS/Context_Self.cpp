#include "Context_Self.h"

#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

void UContext_Self::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
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
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, OwnerActor);
}