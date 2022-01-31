#include "SaiyoraCombatLibrary.h"

#include "AbilityComponent.h"
#include "AnimInstanceProxy.h"
#include "SaiyoraPlayerController.h"
#include "Buff.h"
#include "BuffFunction.h"
#include "Hitbox.h"
#include "Kismet/KismetSystemLibrary.h"

float USaiyoraCombatLibrary::GetActorPing(AActor const* Actor)
{
    APawn const* ActorAsPawn = Cast<APawn>(Actor);
    if (!IsValid(ActorAsPawn))
    {
        return 0.0f;
    }
    ASaiyoraPlayerController const* Controller = Cast<ASaiyoraPlayerController>(ActorAsPawn->GetController());
    if (!IsValid(Controller))
    {
        return 0.0f;
    }
    return Controller->GetPlayerPing();
}

FCombatModifier USaiyoraCombatLibrary::MakeCombatModifier(UBuff* Source, EModifierType const ModifierType,
    float const ModifierValue, bool const bStackable)
{
    return FCombatModifier(ModifierValue, ModifierType, Source, bStackable);
}

FCombatModifier USaiyoraCombatLibrary::MakeBuffFunctionCombatModifier(UBuffFunction* Source,
    EModifierType const ModifierType, float const ModifierValue, bool const bStackable)
{
    return FCombatModifier(ModifierValue, ModifierType, Source->GetOwningBuff(), bStackable);
}

bool USaiyoraCombatLibrary::ValidatePredictedLineTrace(FHitscanTraceServer const& ServerTraceInfo)
{
    if (!IsValid(ServerTraceInfo.Instigator) || !IsValid(ServerTraceInfo.PredictionInfo.HitTarget) ||
        ServerTraceInfo.Instigator == ServerTraceInfo.PredictionInfo.HitTarget ||
        ServerTraceInfo.ActorsToIgnore.Contains(ServerTraceInfo.PredictionInfo.HitTarget))
    {
        return false;
    }
    //TODO: Validate origin location.
    bool bDidHit = false;
    TArray<UBoxHitbox*> HitboxComponents;
    ServerTraceInfo.PredictionInfo.HitTarget->GetComponents<UBoxHitbox>(HitboxComponents);
    for (UBoxHitbox* Hitbox : HitboxComponents)
    {
        Hitbox->RewindByTime(GetActorPing(ServerTraceInfo.Instigator));
    }
    FVector const TraceEnd = ServerTraceInfo.PredictionInfo.AimLocation + (ServerTraceInfo.TraceLength * ServerTraceInfo.PredictionInfo.AimDirection);
    if (ServerTraceInfo.bMultiTrace)
    {
        TArray<FHitResult> Results;
        UKismetSystemLibrary::LineTraceMulti(ServerTraceInfo.Instigator, ServerTraceInfo.PredictionInfo.AimLocation, TraceEnd,
            ServerTraceInfo.TraceChannel, false, ServerTraceInfo.ActorsToIgnore, EDrawDebugTrace::ForDuration, Results, false, FLinearColor::Blue);
        for (FHitResult const& Hit : Results)
        {
            if (IsValid(Hit.GetActor()) && Hit.GetActor() == ServerTraceInfo.PredictionInfo.HitTarget)
            {
                bDidHit = true;
                break;
            }
        }
    }
    else
    {
        FHitResult Result;
        UKismetSystemLibrary::LineTraceSingle(ServerTraceInfo.Instigator, ServerTraceInfo.PredictionInfo.AimLocation, TraceEnd,
            ServerTraceInfo.TraceChannel, false, ServerTraceInfo.ActorsToIgnore, EDrawDebugTrace::ForDuration, Result, false, FLinearColor::Blue);
        if (IsValid(Result.GetActor()) && Result.GetActor() == ServerTraceInfo.PredictionInfo.HitTarget)
        {
            bDidHit = true;
        }
    }
    for (UBoxHitbox* Hitbox : HitboxComponents)
    {
        Hitbox->Unrewind();
    }
    return bDidHit;
}

APredictableProjectile* USaiyoraCombatLibrary::PredictProjectile(UCombatAbility* Source, TSubclassOf<APredictableProjectile> const ProjectileClass, FTransform const& SpawnTransform,
    FCombatParameter& OutLocation, FCombatParameter& OutRotation, FCombatParameter& OutClass)
{
    if (!IsValid(Source) || !IsValid(ProjectileClass) || Source->GetHandler()->GetOwnerRole() != ROLE_AutonomousProxy)
    {
        return nullptr;
    }
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Source->GetHandler()->GetOwner();
    SpawnParams.Instigator = Cast<APawn>(Source->GetHandler()->GetOwner());
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APredictableProjectile* NewProjectile = Source->GetWorld()->SpawnActor<APredictableProjectile>(ProjectileClass, SpawnTransform, SpawnParams);
    if (!IsValid(NewProjectile))
    {
        return nullptr;
    }
    int32 const NewID = NewProjectile->InitializeClient(Source);
    OutLocation.VectorParam = SpawnTransform.GetLocation();
    OutLocation.ParamType = ECombatParamType::Vector;
    OutLocation.ParamName = FString::Printf(TEXT("%iProjectileLocation"), NewID);
    OutRotation.RotatorParam = SpawnTransform.GetRotation().Rotator();
    OutRotation.ParamType = ECombatParamType::Rotator;
    OutRotation.ParamName = FString::Printf(TEXT("%iProjectileRotation"), NewID);
    OutClass.ClassParam = ProjectileClass;
    OutClass.ParamType = ECombatParamType::Class;
    OutClass.ParamName = FString::Printf(TEXT("%iProjectileClass"), NewID);
    return NewProjectile;
}

void USaiyoraCombatLibrary::HandlePredictedProjectiles(UCombatAbility* Source, FCombatParameters const& PredictionParams, TArray<APredictableProjectile*>& OutProjectiles)
{
    if (!IsValid(Source) || Source->GetHandler()->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    TMap<int32, FProjectileSpawnParams> ProjectileTransforms;
    for (FCombatParameter const& Param : PredictionParams.Parameters)
    {
        if (Param.ParamName.Contains("ProjectileLocation"))
        {
            FString ID;
            Param.ParamName.Split("ProjectileLocation", &ID, nullptr);
            int32 const IntID = FCString::Atoi(*ID);
            if (IntID)
            {
                ProjectileTransforms.FindOrAdd(IntID).SpawnTransform.SetLocation(Param.VectorParam);
            }
        }
        else if (Param.ParamName.Contains("ProjectileRotation"))
        {
            FString ID;
            Param.ParamName.Split("ProjectileRotation", &ID, nullptr);
            int32 const IntID = FCString::Atoi(*ID);
            if (IntID)
            {
                ProjectileTransforms.FindOrAdd(IntID).SpawnTransform.SetRotation(Param.RotatorParam.Quaternion());
            }
        }
        else if (Param.ParamName.Contains("ProjectileClass"))
        {
            FString ID;
            Param.ParamName.Split("ProjectileClass", &ID, nullptr);
            int32 const IntID = FCString::Atoi(*ID);
            if (IntID)
            {
                ProjectileTransforms.FindOrAdd(IntID).Class = Param.ClassParam;
            }
        }
    }
    for (TTuple<int32, FProjectileSpawnParams> const& ProjectileInfo : ProjectileTransforms)
    {
        //TODO: Validity checking.
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Source->GetHandler()->GetOwner();
        SpawnParams.Instigator = Cast<APawn>(Source->GetHandler()->GetOwner());
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        APredictableProjectile* Projectile = Source->GetWorld()->SpawnActor<APredictableProjectile>(ProjectileInfo.Value.Class, ProjectileInfo.Value.SpawnTransform, SpawnParams);
        if (IsValid(Projectile))
        {
            Projectile->InitializeServer(Source, ProjectileInfo.Key, GetActorPing(Source->GetHandler()->GetOwner()));
            OutProjectiles.Add(Projectile);
        }
    }
}

