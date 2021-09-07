// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatSystem/Abilities/PlayerSpecialization.h"

void UPlayerSpecialization::InitializeSpecObject(UPlayerAbilityHandler* NewHandler)
{
	if (!IsValid(NewHandler))
	{
		return;
	}
	Handler = NewHandler;
	OnSpecInitialize();
}

void UPlayerSpecialization::UnlearnSpecObject()
{
	OnSpecUnlearned();
}