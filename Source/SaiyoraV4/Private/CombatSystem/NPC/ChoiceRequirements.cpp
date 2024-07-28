#include "ChoiceRequirements.h"

#include "Buff.h"
#include "BuffHandler.h"
#include "SaiyoraCombatInterface.h"
#include "TargetContexts.h"
#include "ThreatHandler.h"
#include "Kismet/KismetMathLibrary.h"

#pragma region Choice Requirement

void FNPCChoiceRequirement::Init(AActor* NPC)
{
	OwningNPC = NPC;
	SetupRequirement();
}

#pragma endregion 
#pragma region Time In Combat

bool FNPCCR_TimeInCombat::IsMet() const
{
	if (!IsValid(ThreatHandler))
	{
		return false;
	}
	const float CombatTime = ThreatHandler->GetCombatTime();
	return bMetAfterTimeReached ? CombatTime > TimeRequirement : CombatTime < TimeRequirement;
}

void FNPCCR_TimeInCombat::SetupRequirement()
{
	const AActor* Owner = GetOwningNPC();
	if (!IsValid(Owner) || !Owner->Implements<USaiyoraCombatInterface>())
	{
		return;
	}
	ThreatHandler = ISaiyoraCombatInterface::Execute_GetThreatHandler(Owner);
}

#pragma endregion
#pragma region Range Of Target

bool FNPCCR_RangeOfTarget::IsMet() const
{
	const FNPCTargetContext* Context = TargetContext.GetPtr<FNPCTargetContext>();
	if (!Context)
	{
		return false;
	}
	const AActor* Target = Context->GetBestTarget(GetOwningNPC());
	if (!IsValid(Target))
	{
		return false;
	}
	const float DistSqToTarget = bIncludeZDistance ? FVector::DistSquared(Target->GetActorLocation(), GetOwningNPC()->GetActorLocation())
		: FVector::DistSquared2D(Target->GetActorLocation(), GetOwningNPC()->GetActorLocation());
	const float ClampedRangeSq = FMath::Square(FMath::Max(0.0f, Range));
	return bWithinRange ? DistSqToTarget < ClampedRangeSq : DistSqToTarget >= ClampedRangeSq;
}

#pragma endregion
#pragma region Rotation To Target

bool FNPCCR_RotationToTarget::IsMet() const
{
	const FNPCTargetContext* Context = TargetContext.GetPtr<FNPCTargetContext>();
	if (!Context)
	{
		return false;
	}
	const AActor* Target = Context->GetBestTarget(GetOwningNPC());
	if (!IsValid(Target))
	{
		return false;
	}
	const FVector OwnerVector = bIncludeZAngle ? GetOwningNPC()->GetActorRotation().Vector().GetSafeNormal() : GetOwningNPC()->GetActorRotation().Vector().GetSafeNormal2D();
	const FVector TowardTarget = bIncludeZAngle ? (Target->GetActorLocation() - GetOwningNPC()->GetActorLocation()).GetSafeNormal() : (Target->GetActorLocation() - GetOwningNPC()->GetActorLocation()).GetSafeNormal2D();
	const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct(OwnerVector, TowardTarget));
	DrawDebugString(GetOwningNPC()->GetWorld(), GetOwningNPC()->GetActorLocation() + FVector::UpVector * 150.0f, FString::SanitizeFloat(Angle),0,  FColor::White, .2f);
	return bWithinAngle ? Angle < AngleThreshold : Angle >= AngleThreshold;
}

#pragma endregion
#pragma region Buff

bool FNPCCR_Buff::IsMet() const
{
	if (!IsValid(BuffClass) || BuffClass == UBuff::StaticClass())
	{
		return !bRequireBuff;
	}
	if (!GetOwningNPC()->Implements<USaiyoraCombatInterface>())
	{
		return !bRequireBuff;
	}
	const UBuffHandler* OwnerBuff = ISaiyoraCombatInterface::Execute_GetBuffHandler(GetOwningNPC());
	if (!IsValid(OwnerBuff))
	{
		return !bRequireBuff;
	}
	TArray<UBuff*> AppliedBuffs;
	OwnerBuff->GetBuffsOfClass(BuffClass, AppliedBuffs);
	return bRequireBuff ? AppliedBuffs.Num() > 0 : AppliedBuffs.Num() == 0;
}

#pragma endregion 