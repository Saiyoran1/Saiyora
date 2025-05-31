#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SaiyoraUIDataAsset.generated.h"

class UEscapeMenu;
class UDeathOverlay;
class UPlayerHUD;
enum class EElementalSchool : uint8;
class USaiyoraErrorMessage;

UCLASS()
class SAIYORAV4_API USaiyoraUIDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	
	USaiyoraUIDataAsset();

	//The main combat HUD for the player.
	UPROPERTY(EditAnywhere, Category = "Widget Classes")
	TSubclassOf<UPlayerHUD> PlayerHUDClass;
	//A widget that pops up to tell a player that an error has occured with some action he took.
	UPROPERTY(EditAnywhere, Category = "Widget Classes")
	TSubclassOf<USaiyoraErrorMessage> ErrorMessageWidgetClass;
	//The overlay that appears when a player is dead to offer options about resurrection and respawning.
	UPROPERTY(EditAnywhere, Category = "Widget Classes")
	TSubclassOf<UDeathOverlay> DeathOverlayClass;
	//The overlay that pops up when a player presses escape.
	UPROPERTY(EditDefaultsOnly, Category = "Widget Classes")
	TSubclassOf<UEscapeMenu> EscapeMenuClass;

	//Colors that represent each spell school in a variety of contexts.
	UPROPERTY(EditAnywhere, Category = "Combat")
	TMap<EElementalSchool, FLinearColor> SchoolColors;
	FLinearColor GetSchoolColor(const EElementalSchool School) const { return SchoolColors.FindRef(School); }

	//If a health bar is NOT using class colors, this is the default health color.
	UPROPERTY(EditAnywhere, Category = "Health Bars")
	FLinearColor DefaultPlayerHealthColor = FLinearColor::Red;
	//The health bar color for dead players.
	UPROPERTY(EditAnywhere, Category = "Health Bars")
	FLinearColor PlayerDeadColor = FLinearColor::Black;

	//The default health bar color for enemy nameplates.
	UPROPERTY(EditAnywhere, Category = "Health Bars")
	FLinearColor EnemyHealthColor = FLinearColor::Red;

	//Outline color for floating damage text.
	UPROPERTY(EditAnywhere, Category = "Floating Combat Text")
	FLinearColor DamageOutlineColor = FLinearColor::Black;
	//Outline color for floating healing text.
	UPROPERTY(EditAnywhere, Category = "Floating Combat Text")
	FLinearColor HealingOutlineColor = FLinearColor::Green;
	//Outline color for floating absorb text.
	UPROPERTY(EditAnywhere, Category = "Floating Combat Text")
	FLinearColor AbsorbOutlineColor = FLinearColor::Blue;

	//When a cast is uninterruptible, the cast bar is this color.
	UPROPERTY(EditAnywhere, Category = "Cast Bars")
	FLinearColor UninterruptibleCastColor = FLinearColor::Gray;
	//When a cast is interrupted, the cast bar briefly flashes this color.
	UPROPERTY(EditAnywhere, Category = "Cast Bars")
	FLinearColor InterruptedCastColor = FLinearColor::Red;
	//When a cast is cancelled, the cast bar briefly flashes this color.
	UPROPERTY(EditAnywhere, Category = "Cast Bars")
	FLinearColor CancelledCastColor = FLinearColor::Gray;
	//When a cast is uninterruptible, this icon appears on one side of the cast bar.
	UPROPERTY(EditAnywhere, Category = "Cast Bars")
	UTexture2D* UninterruptibleCastIcon = nullptr;

	//Default color for text in the dungeon progress display.
	UPROPERTY(EditAnywhere, Category = "Dungeon Progress")
	FLinearColor DungeonProgressDefaultTextColor = FLinearColor::White;
	//Generic "success" color for text in the dungeon progress display.
	UPROPERTY(EditAnywhere, Category = "Dungeon Progress")
	FLinearColor DungeonProgressSuccessTextColor = FLinearColor::Green;
	//Generic "failure" color for text in the dungeon progress display.
	UPROPERTY(EditAnywhere, Category = "Dungeon Progress")
	FLinearColor DungeonProgressFailureTextColor = FLinearColor::Red;
	//Start color for the progress bars for kill count and time remaining. Color lerps between this and DungeonProgressEndBarColor.
	UPROPERTY(EditAnywhere, Category = "Dungeon Progress")
	FLinearColor DungeonProgressStartBarColor = FLinearColor::Yellow;
	//End color for the progress bars for kill count and time remaining. Color lerps between this and DungeonProgressEndBarColor.
	UPROPERTY(EditAnywhere, Category = "Dungeon Progress")
	FLinearColor DungeonProgressEndBarColor = FLinearColor::Red;
	//Progress bar color for the kill count and time remaining when the objective is achieved successfully.
	UPROPERTY(EditAnywhere, Category = "Dungeon Progress")
	FLinearColor SuccessProgressColor = FLinearColor::Green;
	//Progress bar color for the time remaining once the dungeon has been depleted.
	UPROPERTY(EditAnywhere, Category = "Dungeon Progress")
	FLinearColor FailureProgressColor = FLinearColor::Gray;

	//The player resource bar is an arc formed on part of a circle. This is the radius of that circle.
	UPROPERTY(EditAnywhere, Category = "Resources")
	float HUDResourceBarRadius = 240.0f;
	//The player resource bar is an arc formed on part of a circle. This is how much of the circle we use for the arc.
	UPROPERTY(EditAnywhere, Category = "Resources")
	float HUDResourceBarAngleExtent = 110.0f;
	//The color of resource icons in the player's resource bar that are not active.
	UPROPERTY(EditAnywhere, Category = "Resources")
	FLinearColor InactiveDiscreteResourceIconColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.1f);

	//The icon used for an ability slot that has no active ability.
	UPROPERTY(EditAnywhere, Category = "Abilities")
	UTexture2D* InvalidAbilityTexture;
	//The color used for an ability slot that has no active ability.
	UPROPERTY(EditAnywhere, Category = "Abilities")
	FLinearColor InvalidAbilityTint = FLinearColor::Transparent;
};
