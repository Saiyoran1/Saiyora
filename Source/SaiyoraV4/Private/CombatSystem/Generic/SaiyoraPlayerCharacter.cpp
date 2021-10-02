// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraPlayerCharacter.h"

#include "SaiyoraMovementComponent.h"

ASaiyoraPlayerCharacter::ASaiyoraPlayerCharacter(const class FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<USaiyoraMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASaiyoraPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASaiyoraPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASaiyoraPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

