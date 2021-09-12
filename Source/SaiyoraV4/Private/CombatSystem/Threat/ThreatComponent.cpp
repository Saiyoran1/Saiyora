// Fill out your copyright notice in the Description page of Project Settings.

#include "Threat/ThreatComponent.h"

UThreatComponent::UThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UThreatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UThreatComponent::InitializeComponent()
{
	
}


