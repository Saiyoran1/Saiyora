// Fill out your copyright notice in the Description page of Project Settings.


#include "SaiyoraGameState.h"

void ASaiyoraGameState::UpdateClientWorldTime(float const ServerTime)
{
    if (GetLocalRole() != ROLE_Authority)
    {
        WorldTime = ServerTime;
    }
}

ASaiyoraGameState::ASaiyoraGameState()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

float ASaiyoraGameState::GetServerWorldTimeSeconds() const
{
    return GetWorldTime();
}

void ASaiyoraGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    WorldTime += DeltaSeconds;
}
