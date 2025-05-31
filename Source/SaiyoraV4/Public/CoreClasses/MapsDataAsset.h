#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MapsDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FMapInformation
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSoftObjectPtr<UWorld> Level;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FName DisplayName;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FString Description;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UTexture2D* Image = nullptr;
};

UCLASS(Blueprintable)
class UMapsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	TSoftObjectPtr<UWorld> MainMenuLevel;
	//TODO: This doesn't need to be blueprint-exposed once the main menu is moved to C++.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
	TArray<FMapInformation> DungeonPool;
};
