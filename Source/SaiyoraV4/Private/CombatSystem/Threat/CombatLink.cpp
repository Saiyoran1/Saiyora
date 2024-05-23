#include "CombatLink.h"
#include "CombatGroup.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"
#include "Components/BillboardComponent.h"

ACombatLink::ACombatLink()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(FName(TEXT("Scene")));
#if WITH_EDITOR
	Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(FName(TEXT("Billboard")));
	if (IsValid(Billboard))
	{
		Billboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Billboard->SetupAttachment(RootComponent);
	}
#endif
	SetHidden(true);
}

void ACombatLink::BeginPlay()
{
	Super::BeginPlay();
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	for (const AActor* LinkedActor : LinkedActors)
	{
		if (IsValid(LinkedActor) && LinkedActor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UThreatHandler* ActorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(LinkedActor);
			if (IsValid(ActorThreat))
			{
				ThreatHandlers.Add(ActorThreat, false);
				ActorThreat->OnCombatChanged.AddDynamic(this, &ACombatLink::OnActorCombat);
			}
			UNPCAbilityComponent* ActorAbility = Cast<UNPCAbilityComponent>(ISaiyoraCombatInterface::Execute_GetAbilityComponent(LinkedActor));
			if (IsValid(ActorAbility))
			{
				CompletedPatrolSegment.Add(LinkedActor, false);
				ActorAbility->OnPatrolLocationReached.AddDynamic(this, &ACombatLink::OnActorPatrolSegmentFinished);
				ActorAbility->OnPatrolStateChanged.AddDynamic(this, &ACombatLink::OnActorPatrolStateChanged);
				ActorAbility->SetupGroupPatrol(this);
			}
		}
	}
	for (const TTuple<UThreatHandler*, bool>& Combatant : ThreatHandlers)
	{
		if (Combatant.Value)
		{
			bGroupInCombat = true;
			UCombatGroup* GroupToJoin = Combatant.Key->GetCombatGroup();
			TArray<UThreatHandler*> Handlers;
			ThreatHandlers.GenerateKeyArray(Handlers);
			for (UThreatHandler* Handler : Handlers)
			{
				if (Handler != Combatant.Key)
				{
					GroupToJoin->AddCombatant(Handler);
				}
			}
			break;
		}
	}
}

void ACombatLink::OnActorCombat(UThreatHandler* Handler, const bool bInCombat)
{
	ThreatHandlers.Add(Handler, bInCombat);
	if (bInCombat)
	{
		if (bLinkPatrolPaths && CompletedPatrolSegment.Contains(Handler->GetOwner()))
		{
			CompletedPatrolSegment.Add(Handler->GetOwner(), false);
		}
		if (bGroupInCombat)
		{
			return;
		}
		bGroupInCombat = true;
		UCombatGroup* Group = Handler->GetCombatGroup();
		if (IsValid(Group))
		{
			TArray<UThreatHandler*> HandlersToInitiateCombat;
			for (const TTuple<UThreatHandler*, bool>& Combatant : ThreatHandlers)
			{
				if (!Combatant.Value)
				{
					HandlersToInitiateCombat.Add(Combatant.Key);
				}
			}
			for (UThreatHandler* Combatant : HandlersToInitiateCombat)
			{
				Group->AddCombatant(Combatant);
			}
		}
	}
	else
	{
		TArray<bool> Statuses;
		ThreatHandlers.GenerateValueArray(Statuses);
		if (!Statuses.Contains(true))
		{
			bGroupInCombat = false;
		}
	}
}

void ACombatLink::CheckPatrolSegmentComplete()
{
	TArray<bool> Results;
	CompletedPatrolSegment.GenerateValueArray(Results);
	if (!Results.Contains(false))
	{
		for (TTuple<const AActor*, bool>& PatrolTuple : CompletedPatrolSegment)
		{
			PatrolTuple.Value = false;
		}
		OnPatrolSegmentCompleted.Broadcast();
	}
}

void ACombatLink::OnActorPatrolSegmentFinished(AActor* PatrollingActor, const FVector& Location)
{
	CompletedPatrolSegment.Add(PatrollingActor, true);
	CheckPatrolSegmentComplete();
}

void ACombatLink::OnActorPatrolStateChanged(AActor* PatrollingActor, const ENPCPatrolSubstate PatrolSubstate)
{
	if (CompletedPatrolSegment.Contains(PatrollingActor) && PatrolSubstate == ENPCPatrolSubstate::PatrolFinished)
	{
		CompletedPatrolSegment.Remove(PatrollingActor);
		CheckPatrolSegmentComplete();
	}
}