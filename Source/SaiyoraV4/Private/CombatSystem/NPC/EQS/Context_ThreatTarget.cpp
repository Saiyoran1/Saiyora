#include "Context_ThreatTarget.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

void UContext_ThreatTarget::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!IsValid(QueryOwner) || !QueryOwner->Implements<USaiyoraCombatInterface>())
	{
		return;
	}
	const UThreatHandler* ThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(QueryOwner);
	if (!IsValid(ThreatHandler))
	{
		return;
	}
	if (IsValid(ThreatHandler->GetCurrentTarget()))
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, ThreatHandler->GetCurrentTarget());
	}
}