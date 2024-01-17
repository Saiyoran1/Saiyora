#include "SaiyoraCombatInterface.h"
#include "AbilityComponent.h"
#include "BuffHandler.h"
#include "CombatStatusComponent.h"
#include "CrowdControlHandler.h"
#include "DamageHandler.h"
#include "ResourceHandler.h"
#include "SaiyoraMovementComponent.h"
#include "StatHandler.h"
#include "ThreatHandler.h"

UCombatStatusComponent* ISaiyoraCombatInterface::GetCombatStatusComponent_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetCombatStatusComponent for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedCombatStatusComponent)
	{
		return CachedCombatStatusComponent;
	}
	bCachedCombatStatusComponent = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedCombatStatusComponent = ImplementingActor->FindComponentByClass<UCombatStatusComponent>();
	}
	return CachedCombatStatusComponent;
}

UDamageHandler* ISaiyoraCombatInterface::GetDamageHandler_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetDamageHandler for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedDamageHandler)
	{
		return CachedDamageHandler;
	}
	bCachedDamageHandler = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedDamageHandler = ImplementingActor->FindComponentByClass<UDamageHandler>();
	}
	return CachedDamageHandler;
}

UBuffHandler* ISaiyoraCombatInterface::GetBuffHandler_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetBuffHandler for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedBuffHandler)
	{
		return CachedBuffHandler;
	}
	bCachedBuffHandler = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedBuffHandler = ImplementingActor->FindComponentByClass<UBuffHandler>();
	}
	return CachedBuffHandler;
}

UAbilityComponent* ISaiyoraCombatInterface::GetAbilityComponent_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetAbilityComponent for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedAbilityComponent)
	{
		return CachedAbilityComponent;
	}
	bCachedAbilityComponent = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedAbilityComponent = ImplementingActor->FindComponentByClass<UAbilityComponent>();
	}
	return CachedAbilityComponent;
}

UCrowdControlHandler* ISaiyoraCombatInterface::GetCrowdControlHandler_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetCrowdControlHandler for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedCrowdControlHandler)
	{
		return CachedCrowdControlHandler;
	}
	bCachedCrowdControlHandler = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedCrowdControlHandler = ImplementingActor->FindComponentByClass<UCrowdControlHandler>();
	}
	return CachedCrowdControlHandler;
}

UResourceHandler* ISaiyoraCombatInterface::GetResourceHandler_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetResourceHandler for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedResourceHandler)
	{
		return CachedResourceHandler;
	}
	bCachedResourceHandler = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedResourceHandler = ImplementingActor->FindComponentByClass<UResourceHandler>();
	}
	return CachedResourceHandler;
}

UStatHandler* ISaiyoraCombatInterface::GetStatHandler_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetStatHandler for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedStatHandler)
	{
		return CachedStatHandler;
	}
	bCachedStatHandler = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedStatHandler = ImplementingActor->FindComponentByClass<UStatHandler>();
	}
	return CachedStatHandler;
}

UThreatHandler* ISaiyoraCombatInterface::GetThreatHandler_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetThreatHandler for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedThreatHandler)
	{
		return CachedThreatHandler;
	}
	bCachedThreatHandler = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedThreatHandler = ImplementingActor->FindComponentByClass<UThreatHandler>();
	}
	return CachedThreatHandler;
}

USaiyoraMovementComponent* ISaiyoraCombatInterface::GetCustomMovementComponent_Implementation() const
{
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of GetCustomMovementComponent for object %s. Please override this to return a pointer."), *_getUObject()->GetName());
	if (bCachedMovementComponent)
	{
		return CachedMovementComponent;
	}
	bCachedMovementComponent = true;
	const AActor* ImplementingActor = Cast<AActor>(_getUObject());
	if (IsValid(ImplementingActor))
	{
		CachedMovementComponent = ImplementingActor->FindComponentByClass<USaiyoraMovementComponent>();
	}
	return CachedMovementComponent;
}