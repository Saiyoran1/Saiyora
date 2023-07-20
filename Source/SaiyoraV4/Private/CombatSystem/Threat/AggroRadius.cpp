#include "AggroRadius.h"
#include "AbilityComponent.h"
#include "CombatStatusComponent.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatInterface.h"
#include "StatHandler.h"

UAggroRadius::UAggroRadius()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UAggroRadius::InitializeComponent()
{
	SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
	OnComponentBeginOverlap.AddDynamic(this, &UAggroRadius::OnOverlap);
}

void UAggroRadius::Initialize(const float DefaultRadius)
{
	DefaultAggroRadius = DefaultRadius;
	SetSphereRadius(DefaultRadius);
	if (GetOwner()->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		const UCombatStatusComponent* CombatStatusComponentRef = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(GetOwner());
		if (IsValid(CombatStatusComponentRef))
		{
			OwnerFaction = CombatStatusComponentRef->GetCurrentFaction();
		}
		StatHandlerRef = ISaiyoraCombatInterface::Execute_GetStatHandler(GetOwner());
		if (IsValid(StatHandlerRef) && StatHandlerRef->IsStatValid(FSaiyoraCombatTags::Get().Stat_AggroRadius))
		{
			AggroRadiusCallback.BindDynamic(this, &UAggroRadius::OnAggroRadiusStatChanged);
			StatHandlerRef->SubscribeToStatChanged(FSaiyoraCombatTags::Get().Stat_AggroRadius, AggroRadiusCallback);
			OnAggroRadiusStatChanged(FSaiyoraCombatTags::Get().Stat_AggroRadius, StatHandlerRef->GetStatValue(FSaiyoraCombatTags::Get().Stat_AggroRadius));
		}
		if (OwnerFaction == EFaction::Enemy)
		{
			NPCComponentRef = Cast<UNPCAbilityComponent>(ISaiyoraCombatInterface::Execute_GetAbilityComponent(GetOwner()));
			if (IsValid(NPCComponentRef))
			{
				NPCComponentRef->OnCombatBehaviorChanged.AddDynamic(this, &UAggroRadius::OnCombatBehaviorChanged);
				OnCombatBehaviorChanged(ENPCCombatBehavior::None, NPCComponentRef->GetCombatBehavior());
			}
		}
		else if (OwnerFaction == EFaction::Friendly)
		{
			SetCollisionProfileName(FSaiyoraCollision::P_PlayerAggro);
		}
	}
}

void UAggroRadius::OnAggroRadiusStatChanged(const FGameplayTag StatTag, const float NewValue)
{
	SetSphereRadius(FMath::Max(0.0f, DefaultAggroRadius * NewValue));
}

void UAggroRadius::OnCombatBehaviorChanged(const ENPCCombatBehavior PreviousBehavior,
	const ENPCCombatBehavior NewBehavior)
{
	switch (NewBehavior)
	{
	case ENPCCombatBehavior::Combat :
		SetCollisionProfileName(FSaiyoraCollision::P_NPCCombatAggro);
		break;
	case ENPCCombatBehavior::Patrolling :
		SetCollisionProfileName(FSaiyoraCollision::P_NPCNonCombatAggro);
		break;
	default :
		SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
	}
}

void UAggroRadius::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsValid(OtherActor) && OtherActor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
	{
		UCombatStatusComponent* OtherCombatStatus = ISaiyoraCombatInterface::Execute_GetCombatStatusComponent(OtherActor);
		if (IsValid(OtherCombatStatus))
		{
			
		}
		UThreatHandler* OtherActorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(OtherActor);
	}
}

