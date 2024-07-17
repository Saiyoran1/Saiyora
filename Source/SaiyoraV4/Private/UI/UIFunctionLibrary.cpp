﻿#include "UIFunctionLibrary.h"

#include "DamageEnums.h"
#include "SaiyoraGameInstance.h"
#include "SaiyoraUIDataAsset.h"

#pragma region Setup

USaiyoraUIDataAsset* UUIFunctionLibrary::UIDataAsset = nullptr;

USaiyoraUIDataAsset* UUIFunctionLibrary::GetUIDataAsset(UObject* WorldContext)
{
	if (!IsValid(UIDataAsset))
	{
		if (IsValid(WorldContext) && IsValid(WorldContext->GetWorld()))
		{
			const USaiyoraGameInstance* GameInstance = Cast<USaiyoraGameInstance>(WorldContext->GetWorld()->GetGameInstance());
			if (IsValid(GameInstance))
			{
				UIDataAsset = GameInstance->UIDataAsset;
			}
		}
	}
	return UIDataAsset;
}

#pragma endregion
#pragma region Formatting Strings

FString UUIFunctionLibrary::GetTimeDisplayString(const float Seconds)
{
	const int IntSeconds = FMath::FloorToInt(Seconds);
	const int Minutes = IntSeconds / 60;
	const int RemainingSeconds = IntSeconds - (Minutes * 60);
	FString TimeString = FString::Printf(L"%s%i:%s%i",
		*FString(Minutes < 10 ? "0" : ""), Minutes,
		*FString(RemainingSeconds < 10 ? "0" : ""), RemainingSeconds);
	return TimeString;
}

#pragma endregion
#pragma region Colors

FLinearColor UUIFunctionLibrary::GetElementalSchoolColor(UObject* WorldContext, const EElementalSchool School)
{
	const USaiyoraUIDataAsset* UIDataAsset = GetUIDataAsset(WorldContext);
	if (IsValid(UIDataAsset))
	{
		return UIDataAsset->SchoolColors.FindRef(School);
	}
	return FLinearColor::White;
}

FLinearColor UUIFunctionLibrary::GetPlayerHealthColor(UObject* WorldContext)
{
	const USaiyoraUIDataAsset* UIDataAsset = GetUIDataAsset(WorldContext);
	if (IsValid(UIDataAsset))
	{
		return UIDataAsset->PlayerHealthColor;
	}
	return FLinearColor::Red;
}

FLinearColor UUIFunctionLibrary::GetPlayerDeadColor(UObject* WorldContext)
{
	const USaiyoraUIDataAsset* UIDataAsset = GetUIDataAsset(WorldContext);
	if (IsValid(UIDataAsset))
	{
		return UIDataAsset->PlayerDeadColor;
	}
	return FLinearColor::Black;
}

FLinearColor UUIFunctionLibrary::GetHealthEventOutlineColor(UObject* WorldContext, const EHealthEventType EventType)
{
	const USaiyoraUIDataAsset* UIDataAsset = GetUIDataAsset(WorldContext);
	if (IsValid(UIDataAsset))
	{
		switch (EventType)
		{
		case EHealthEventType::Damage :
			return UIDataAsset->DamageOutlineColor;
		case EHealthEventType::Healing :
			return UIDataAsset->HealingOutlineColor;
		case EHealthEventType::Absorb :
			return UIDataAsset->AbsorbOutlineColor;
		default :
			return FLinearColor::Transparent;
		}
	}
	return FLinearColor::Transparent;
}

FLinearColor UUIFunctionLibrary::GetUninterruptibleCastColor(UObject* WorldContext)
{
	const USaiyoraUIDataAsset* UIDataAsset = GetUIDataAsset(WorldContext);
	if (IsValid(UIDataAsset))
	{
		return UIDataAsset->UninterruptibleCastColor;
	}
	return FLinearColor::Gray;
}

FLinearColor UUIFunctionLibrary::GetInterruptedCastColor(UObject* WorldContext)
{
	const USaiyoraUIDataAsset* UIDataAsset = GetUIDataAsset(WorldContext);
	if (IsValid(UIDataAsset))
	{
		return UIDataAsset->InterruptedCastColor;
	}
	return FLinearColor::Red;
}

FLinearColor UUIFunctionLibrary::GetCancelledCastColor(UObject* WorldContext)
{
	const USaiyoraUIDataAsset* UIDataAsset = GetUIDataAsset(WorldContext);
	if (IsValid(UIDataAsset))
	{
		return UIDataAsset->CancelledCastColor;
	}
	return FLinearColor::Gray;
}

#pragma endregion 