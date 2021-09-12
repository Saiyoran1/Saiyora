// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ThreatStructs.h"
#include "Components/ActorComponent.h"
#include "ThreatComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAIYORAV4_API UThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UThreatComponent();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

	void AddFixate(AActor* Target, UBuff* Source);
	void AddBlind(AActor* Target, UBuff* Source);
	void AddMisdirect(AActor* ThreatFrom, AActor* ThreatTo, UBuff* Source);

private:
	UPROPERTY()
	TSet<FThreatTarget> ThreatTable;
};
