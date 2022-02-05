#include "AbilityFunctionLibrary.h"
#include "SaiyoraCombatInterface.h"
#include "SaiyoraGameMode.h"
#include "Kismet/KismetSystemLibrary.h"

FAbilityOrigin UAbilityFunctionLibrary::MakeAbilityOrigin(FVector const& AimLocation, FVector const& AimDirection, FVector const& Origin)
{
	FAbilityOrigin Target;
	Target.Origin = Origin;
	Target.AimLocation = AimLocation;
	Target.AimDirection = AimDirection;
	return Target;
}

bool UAbilityFunctionLibrary::PredictLineTrace(AActor* Shooter, float const TraceLength,
	TEnumAsByte<ETraceTypeQuery> const TraceChannel, TArray<AActor*> const& ActorsToIgnore, int32 const TargetSetID,
	FHitResult& Result, FAbilityOrigin& OutOrigin, FAbilityTargetSet& OutTargetSet)
{
	if (!IsValid(Shooter) || !Shooter->GetClass()->ImplementsInterface(USaiyoraCombatInterface::StaticClass()) || TraceLength <= 0.0f)
	{
		return false;
	}
	APawn* ShooterAsPawn = Cast<APawn>(Shooter);
	if (!IsValid(ShooterAsPawn) || !ShooterAsPawn->IsLocallyControlled())
	{
		return false;
	}
	FName AimSocket;
	USceneComponent* AimComponent = ISaiyoraCombatInterface::Execute_GetAimSocket(Shooter, AimSocket);
	FName OriginSocket;
	USceneComponent* OriginComponent = ISaiyoraCombatInterface::Execute_GetAbilityOriginSocket(Shooter, OriginSocket);
	
	if (IsValid(AimComponent))
	{
		OutOrigin.AimLocation = AimComponent->GetSocketLocation(AimSocket);
		OutOrigin.AimDirection = AimComponent->GetForwardVector();
		OutOrigin.Origin = IsValid(OriginComponent) ? OriginComponent->GetSocketLocation(OriginSocket) : OutOrigin.AimLocation;
	}
	else if (IsValid(OriginComponent))
	{
		OutOrigin.Origin = OriginComponent->GetSocketLocation(OriginSocket);
		OutOrigin.AimLocation = OutOrigin.Origin;
		OutOrigin.AimDirection = OriginComponent->GetForwardVector();
	}
	else
	{
		OutOrigin.Origin = Shooter->GetActorLocation();
		OutOrigin.AimLocation = Shooter->GetActorLocation();
		OutOrigin.AimDirection = Shooter->GetActorForwardVector();
	}

	FVector const CamTraceEnd = OutOrigin.AimLocation + (ASaiyoraGameMode::CamTraceLength * OutOrigin.AimDirection);
	FHitResult CamTraceResult;
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.AimLocation, CamTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, CamTraceResult, true);

	//If we hit something and it was in front of the origin, use that as our trace target, otherwise use the end of the trace.
	FVector const OriginTraceEnd = OutOrigin.Origin + TraceLength *
			(((CamTraceResult.bBlockingHit && FVector::DotProduct((CamTraceResult.ImpactPoint - OutOrigin.Origin), OutOrigin.AimDirection) > 0.0f)
				? CamTraceResult.ImpactPoint : CamTraceResult.TraceEnd) - OutOrigin.Origin).GetSafeNormal();
	UKismetSystemLibrary::LineTraceSingle(Shooter, OutOrigin.Origin, OriginTraceEnd, TraceChannel, false,
		ActorsToIgnore, EDrawDebugTrace::None, Result, true);

	OutTargetSet.SetID = TargetSetID;
	if (IsValid(Result.GetActor()))
	{
		OutTargetSet.Targets.Add(Result.GetActor());
	}
	
	return Result.bBlockingHit;
}