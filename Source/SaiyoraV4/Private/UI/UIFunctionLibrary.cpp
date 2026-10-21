#include "UIFunctionLibrary.h"
#include "DamageEnums.h"
#include "SaiyoraDevSettings.h"
#include "SaiyoraUIDataAsset.h"

#pragma region Formatting Strings

FString UUIFunctionLibrary::GetTimeDisplayString(const float Seconds)
{
	const int IntSeconds = FMath::CeilToInt(Seconds);
	const int Minutes = IntSeconds / 60;
	const int RemainingSeconds = IntSeconds - (Minutes * 60);
	FString TimeString = FString::Printf(L"%s%i:%s%i",
		*FString(Minutes < 10 ? "0" : ""), Minutes,
		*FString(RemainingSeconds < 10 ? "0" : ""), RemainingSeconds);
	return TimeString;
}

FString UUIFunctionLibrary::GetShortKeybind(const FKey& Key)
{
	if (Key == EKeys::LeftMouseButton)
	{
		return "LMB";
	}
	if (Key == EKeys::RightMouseButton)
	{
		return "RMB";
	}
	if (Key == EKeys::MiddleMouseButton)
	{
		return "MMB";
	}
	if (Key == EKeys::MouseScrollUp)
	{
		return "SU";
	}
	if (Key == EKeys::MouseScrollDown)
	{
		return "SD";
	}
	if (Key == EKeys::CapsLock)
	{
		return "Caps";
	}
	if (Key == EKeys::Escape)
	{
		return "Esc";
	}
	if (Key == EKeys::SpaceBar)
	{
		return "Space";
	}
	
	return Key.GetDisplayName(false).ToString();
}

FString UUIFunctionLibrary::GetInputChordString(const FInputChord& InputChord)
{
	FString ModifierString;
	if (InputChord.bAlt)
	{
		ModifierString += "A+";
	}
	if (InputChord.bCmd)
	{
		ModifierString += "Cmd+";
	}
	if (InputChord.bCtrl)
	{
		ModifierString += "Ctrl+";
	}
	if (InputChord.bShift)
	{
		ModifierString += "S+";
	}
	return ModifierString + GetShortKeybind(InputChord.Key);
}

#pragma endregion
#pragma region Colors

FLinearColor UUIFunctionLibrary::GetElementalSchoolColor(const EElementalSchool School)
{
	return UI_DA->SchoolColors.FindRef(School);
}

FLinearColor UUIFunctionLibrary::GetHealthEventOutlineColor(const EHealthEventType EventType)
{
	switch (EventType)
	{
	case EHealthEventType::Damage :
		return UI_DA->DamageOutlineColor;
	case EHealthEventType::Healing :
		return UI_DA->HealingOutlineColor;
	case EHealthEventType::Absorb :
		return UI_DA->AbsorbOutlineColor;
	default :
		return FLinearColor::Transparent;
	}
}

#pragma endregion 