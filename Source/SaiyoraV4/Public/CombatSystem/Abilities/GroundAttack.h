#pragma once
#include "CoreMinimal.h"
#include "CombatEnums.h"
#include "Engine/DecalActor.h"
#include "GroundAttack.generated.h"

class AGroundAttack;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGroundDetonation, AGroundAttack*, Attack, const TArray<AActor*>&, HitActors);

UCLASS()
class SAIYORAV4_API AGroundAttack : public ADecalActor
{
	GENERATED_BODY()

public:

	AGroundAttack(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void Initialize(const FVector& InExtent, const float InConeAngle, const float InInnerRingPercent, const EFaction InHostility, const float InDetonationTime, const bool bInDestroyOnDetonation,
		const FLinearColor& InIndicatorColor, UTexture2D* InIndicatorTexture, const float InIntensity);
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Decal")
	void SetDecalExtent(const FVector NewExtent);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Decal")
	void SetConeAngle(const float NewConeAngle);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Decal")
	void SetInnerRingPercent(const float NewInnerRingPercent);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Decal")
	void SetHostility(const EFaction NewHostility);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Decal")
	void SetIndicatorColor(const FLinearColor NewIndicatorColor);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Decal")
	void SetIndicatorTexture(UTexture2D* NewIndicatorTexture);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Decal")
	void SetIntensity(const float NewIntensity);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Detonation")
	void SetDetonationTime(const float NewDetonationTime, const bool bInDestroyOnDetonate);
	UPROPERTY(BlueprintAssignable, Category = "Detonation")
	FOnGroundDetonation OnDetonation;

	UFUNCTION(BlueprintPure, Category = "Detonation")
	void GetActorsInDetonation(TArray<AActor*>& HitActors) const;

private:

	UPROPERTY(ReplicatedUsing=OnRep_DecalExtent)
	FVector DecalExtent = FVector(100.0f);
	UFUNCTION()
	void OnRep_DecalExtent();
	UPROPERTY(ReplicatedUsing=OnRep_ConeAngle)
	float ConeAngle = 360.0f;
	UFUNCTION()
	void OnRep_ConeAngle();
	UPROPERTY(ReplicatedUsing=OnRep_InnerRingPercent)
	float InnerRingPercent = 0.0f;
	UFUNCTION()
	void OnRep_InnerRingPercent();
	UPROPERTY(ReplicatedUsing=OnRep_Hostility)
	EFaction Hostility = EFaction::Neutral;
	UFUNCTION()
	void OnRep_Hostility();
	UPROPERTY(ReplicatedUsing=OnRep_IndicatorColor)
	FLinearColor IndicatorColor = FLinearColor::Red;
	UFUNCTION()
	void OnRep_IndicatorColor();
	UPROPERTY(ReplicatedUsing=OnRep_IndicatorTexture)
	UTexture2D* IndicatorTexture = nullptr;
	UFUNCTION()
	void OnRep_IndicatorTexture();
	UPROPERTY(ReplicatedUsing=OnRep_Intensity)
	float Intensity = 1.0f;
	UFUNCTION()
	void OnRep_Intensity();

	UPROPERTY(ReplicatedUsing=OnRep_DetonationTime)
	float DetonationTime = 0.0f;
	UFUNCTION()
	void OnRep_DetonationTime();
	float StartTime = 0.0f;
	UPROPERTY(Replicated)
	bool bDestroyOnDetonate = false;

	UPROPERTY()
	UMaterialInstanceDynamic* DecalMaterial;
	UPROPERTY()
	UBoxComponent* OuterBox;

	void DoDetonation();
	float GetAngleToActor(AActor* Actor) const;
	float GetRadiusAtAngle(const float Angle) const;
};