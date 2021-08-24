// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatStat.h"
#include "StatHandler.h"

void UCombatStat::Init(FStatInfo const& InitInfo, UStatHandler* Handler)
{
	if (!IsValid(Handler) || !InitInfo.StatTag.MatchesTag(UStatHandler::GenericStatTag()))
	{
		return;
	}
	this->Handler = Handler;
	StatTag = InitInfo.StatTag;
	bShouldReplicate = InitInfo.bShouldReplicate;
	UModifiableFloatValue::Init(InitInfo.DefaultValue, InitInfo.bModifiable, InitInfo.bCappedLow, InitInfo.MinClamp, InitInfo.bCappedHigh, InitInfo.MaxClamp);
}
