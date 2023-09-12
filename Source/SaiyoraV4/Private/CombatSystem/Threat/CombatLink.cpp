#include "CombatLink.h"
#include "CombatGroup.h"
#include "SaiyoraCombatInterface.h"
#include "ThreatHandler.h"

ACombatLink::ACombatLink()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* SceneComp = CreateDefaultSubobject<USceneComponent>(FName(TEXT("Scene")));
	RootComponent = SceneComp;
#if WITH_EDITORONLY_DATA
	Sphere = CreateEditorOnlyDefaultSubobject<USphereComponent>(FName(TEXT("Sphere")));
	Sphere->InitSphereRadius(50.0f);
	Sphere->SetupAttachment(RootComponent);
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sphere->SetCanEverAffectNavigation(false);
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
		if (LinkedActor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UThreatHandler* ActorThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(LinkedActor);
			if (IsValid(ActorThreat))
			{
				ThreatHandlers.Add(ActorThreat, false);
				ActorThreat->OnCombatChanged.AddDynamic(this, &ACombatLink::OnActorCombat);
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
