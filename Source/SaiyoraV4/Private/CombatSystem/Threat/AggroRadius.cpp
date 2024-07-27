#include "AggroRadius.h"

#include <CombatGroup.h>

#include "AbilityComponent.h"
#include "CombatStatusComponent.h"
#include "NPCAbilityComponent.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraCombatLibrary.h"
#include "StatHandler.h"
#include "ThreatHandler.h"
#include "Kismet/KismetSystemLibrary.h"

static TAutoConsoleVariable<int32> DrawAggroSpheres(
		TEXT("game.DrawAggroSpheres"),
		0,
		TEXT("Determines whether Aggro Radius spheres should be visible."),
		ECVF_Default);

UAggroRadius::UAggroRadius()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UAggroRadius::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (DrawAggroSpheres.GetValueOnGameThread() > 0)
	{
		if (GetCollisionProfileName() == FSaiyoraCollision::P_NPCAggro)
		{
			DrawDebugSphere(GetWorld(), GetComponentLocation(), GetScaledSphereRadius(), 32, FColor::Red);
		}
		else if (GetCollisionProfileName() == FSaiyoraCollision::P_PlayerAggro)
		{
			DrawDebugSphere(GetWorld(), GetComponentLocation(), GetScaledSphereRadius(), 32, FColor::Blue);
		}
	}
}

void UAggroRadius::InitializeComponent()
{
	Super::InitializeComponent();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
	OnComponentBeginOverlap.AddDynamic(this, &UAggroRadius::OnOverlap);
}

void UAggroRadius::Initialize(UThreatHandler* ThreatHandler, const float DefaultRadius)
{
	if (!IsValid(ThreatHandler) || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	DefaultAggroRadius = FMath::Max(0.0f, DefaultRadius);
	ThreatHandlerRef = ThreatHandler;
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
		SetCollisionProfileName(FSaiyoraCollision::P_NPCAggro);
		break;
	case ENPCCombatBehavior::Patrolling :
		SetCollisionProfileName(FSaiyoraCollision::P_NPCAggro);
		break;
	default :
		SetCollisionProfileName(FSaiyoraCollision::P_NoCollision);
	}
}

void UAggroRadius::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UAggroRadius* OverlappedAggro = Cast<UAggroRadius>(OtherComp);
	if (IsValid(ThreatHandlerRef) && IsValid(OverlappedAggro) && OwnerFaction == EFaction::Enemy)
	{
		if (OverlappedAggro->GetOwnerFaction() == EFaction::Friendly && !ThreatHandlerRef->IsActorInThreatTable(OtherActor))
		{
			FHitResult SightResult;
			UKismetSystemLibrary::LineTraceSingle(this, GetComponentLocation(), OverlappedAggro->GetComponentLocation(),
				UEngineTypes::ConvertToTraceType(ECC_Visibility), false,TArray<AActor*>(),
				EDrawDebugTrace::None, SightResult, true);
			if (!SightResult.bBlockingHit)
			{
				ThreatHandlerRef->AddThreat(EThreatType::Absolute, 1.0f, OtherActor, nullptr, false, false, FThreatModCondition());
			}
		}
		else if (OverlappedAggro->GetOwnerFaction() == EFaction::Enemy && ThreatHandlerRef->IsInCombat() && IsValid(ThreatHandlerRef->GetCombatGroup())
			&& OtherActor->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()))
		{
			UThreatHandler* CombatantThreat = ISaiyoraCombatInterface::Execute_GetThreatHandler(OverlappedAggro->GetOwner());
			if (IsValid(CombatantThreat) && CombatantThreat->HasThreatTable())
			{
				if (CombatantThreat->IsInCombat() && IsValid(CombatantThreat->GetCombatGroup()))
				{
					if (CombatantThreat->GetCombatGroup() != ThreatHandlerRef->GetCombatGroup())
					{
						ThreatHandlerRef->GetCombatGroup()->MergeWith(CombatantThreat->GetCombatGroup());
					}
				}
				else
				{
					ThreatHandlerRef->GetCombatGroup()->AddCombatant(CombatantThreat);
				}
			}
		}
	}
}

