// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "SaiyoraEnums.h"
#include "Hitbox.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class SAIYORAV4_API UHitbox : public UBoxComponent
{
	GENERATED_BODY()
public:
	UHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
private:
	void UpdateFactionCollision(EFaction const NewFaction);
};
