// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SaiyoraPlayerCharacter.generated.h"

UCLASS()
class SAIYORAV4_API ASaiyoraPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASaiyoraPlayerCharacter(const class FObjectInitializer& ObjectInitializer);
protected:
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
