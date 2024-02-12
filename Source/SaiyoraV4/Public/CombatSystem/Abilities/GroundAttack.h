#pragma once
#include "CoreMinimal.h"
#include "AbilityStructs.h"
#include "CombatEnums.h"
#include "Engine/DecalActor.h"
#include "GroundAttack.generated.h"

class UAbilityComponent;
class AGroundAttack;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FGroundDetonationCallback, AGroundAttack*, Attack, const TArray<AActor*>&, HitActors);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGroundDetonation, AGroundAttack*, Attack, const TArray<AActor*>&, HitActors);

//Detonation params used when creating a ground attack on the server.
USTRUCT(BlueprintType)
struct FGroundAttackDetonationParams
{
	GENERATED_BODY()

	//The faction of the actor performing this attack, for use in filtering targets.
	UPROPERTY()
	EFaction AttackerFaction = EFaction::Neutral;
	//Faction filter for actors hit by the detonation. Relative to the actor who summoned this ground attack.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESaiyoraFactionFilter FactionFilter = ESaiyoraFactionFilter::None;
	//The plane of the actor performing this attack, for use in filtering targets.
	UPROPERTY()
	ESaiyoraPlane AttackerPlane = ESaiyoraPlane::Both;
	//Plane filter for actors hit by the detonation. Relative to the actor who summoned this ground attack.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESaiyoraPlaneFilter PlaneFilter = ESaiyoraPlaneFilter::None;
	//Whether to tie the detonation of this ground attack to the end of the summoner's current cast or use the provided delay instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBindToCastEnd = false;
	//If not tieing the detonation to the end of a cast, this is the time it takes for the attack to detonate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bBindToCastEnd"))
	float DetonationDelay = 1.0f;
	//Whether to loop the detonation multiple times before destroying the attack. Not available if tied to a cast.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bBindToCastEnd"))
	bool bLoopDetonation = false;
	//When looping the detonation, whether to infinitely loop until the attack actor is manually stopped or not.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bBindToCastEnd && bLoopDetonation"))
	bool bInfiniteLoop = false;
	//If not infinitely looping, how many times after the initial detonation to loop before destroying the attack actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bBindToCastEnd && bLoopDetonation && !bInfiniteLoop"))
	int32 NumAdditionalLoops = 1;
	//Optional callback fired off when the detonation completes, providing the attack actor and all hit actors.
	//This delegate can also be manually bound to later, but providing it while initializing can be convenient.
	UPROPERTY(BlueprintReadWrite)
	FGroundDetonationCallback OnDetonationCallback = FGroundDetonationCallback();
};

//Struct for replicating detonation params to clients so they can display visuals for the detonation behavior.
//Bind to casts, filtering targets, and executing callbacks are all server only, so it makes more sense to have a separate struct that just represents timing information for clients.
USTRUCT()
struct FReplicatedDetonationParams
{
	GENERATED_BODY()

	//Tiemstamp for when the first detonation should happen.
	UPROPERTY()
	float FirstDetonationTime = 0.0f;
	//Whether multiple detonations will occur.
	UPROPERTY()
	bool bLooping = false;
	//How far apart each detonation will occur after the first.
	UPROPERTY()
	float DetonationInterval = 0.0f;
	//Whether these detonations should continue until manually stopped.
	UPROPERTY()
	bool bInfiniteLooping = false;
	//If not infinitely looping, how many additional detonations should occur.
	UPROPERTY()
	int32 AdditionalLoops = 1;

	//Flag for valid params, used to identify on clients when to initialize.
	UPROPERTY()
	bool bValid = false;
};

//Struct for basic parameters of a ground attack. Note that things like DecalExtent, ConeAngle, and InnerRingPercent WILL affect the hitbox of the attack.
USTRUCT(BlueprintType)
struct FGroundAttackVisualParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector DecalExtent = FVector(100.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ConeAngle = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InnerRingPercent = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor IndicatorColor = FLinearColor::Red;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float IndicatorIntensity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* IndicatorTexture = nullptr;

	//Flag for valid params, used to identify on clients when to initialize.
	UPROPERTY()
	bool bValid = false;
};

UCLASS()
class SAIYORAV4_API AGroundAttack : public ADecalActor
{
	GENERATED_BODY()

#pragma region Core

public:

	AGroundAttack(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	void ServerInit(const FGroundAttackVisualParams& InAttackParams, const FGroundAttackDetonationParams& InDetonationParams);
	virtual void PostNetReceive() override;

private:

	bool bLocallyInitialized = false;
	
#pragma endregion
#pragma region Visuals

private:

	UPROPERTY(Replicated)
	FGroundAttackVisualParams VisualParams;

	//Called during initialization to setup decal size and material parameters.
	void SetupVisuals();

	UPROPERTY()
	UMaterialInstanceDynamic* DecalMaterial;

#pragma endregion 
#pragma region Detonation
	
	UPROPERTY(BlueprintAssignable, Category = "Detonation")
	FOnGroundDetonation OnDetonation;
	UFUNCTION(BlueprintPure, Category = "Detonation")
	void GetActorsInDetonation(TArray<AActor*>& HitActors) const;

private:

	FGroundAttackDetonationParams DetonationParams;
	UPROPERTY(Replicated)
	FReplicatedDetonationParams ReplicatedDetonationParams;
	float StartTime = 0.0f;
	float LocalDetonationTime = 0.0f;
	int32 CompletedDetonationCounter = 0;

	void SetupDetonation();
	void DoDetonation();
	void DestroyGroundAttack();
	UFUNCTION()
	void DelayedDestroy() { Destroy(); }
	UPROPERTY(ReplicatedUsing = OnRep_Destroyed)
	bool bDestroyed = false;
	UFUNCTION()
	void OnRep_Destroyed() { Destroy(); }

	UPROPERTY()
	UAbilityComponent* OwnerAbilityCompRef = nullptr;
	UFUNCTION()
	void DetonateOnCastFinalTick(const FAbilityEvent& Event) { DestroyGroundAttack(); }
	UFUNCTION()
	void CancelDetonationFromCastCancel(const FCancelEvent& Event) { DestroyGroundAttack(); }
	UFUNCTION()
	void CancelDetonationFromCastInterrupt(const FInterruptEvent& Event) { DestroyGroundAttack(); }
	
	UPROPERTY()
	UBoxComponent* OuterBox;
	
	float GetAngleToActor(AActor* Actor) const;
	float GetRadiusAtAngle(const float Angle) const;

#pragma endregion
};