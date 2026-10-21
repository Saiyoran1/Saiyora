#pragma once
#include "CoreMinimal.h"
#include "DeveloperSettings.h"
#include "SaiyoraDevSettings.generated.h"

class UMapsDataAsset;
class USaiyoraUIDataAsset;
class UCombatDebugOptions;

#define CombatDebug_DA USaiyoraDevSettings::GetCombatDebugDataAsset()
#define UI_DA USaiyoraDevSettings::GetUIDataAsset()
#define Maps_DA USaiyoraDevSettings::GetMapsDataAsset()

#pragma region Macros

#if WITH_EDITOR
//For the editor, we have a fallback function that can be used to lazy load at edit time.
#define SAIYORA_DEV_ASSET_EDITOR_DECLARE(Type, Name) \
	static Type* Editor_LazyLoad##Name();
#define SAIYORA_DEV_ASSET_EDITOR_FALLBACK(Name) \
	if (!Static_##Name) { return Editor_LazyLoad##Name(); }
#else
//Outside of the editor, these compile out
#define SAIYORA_DEV_ASSET_EDITOR_FALLBACK(Name)
#define SAIYORA_DEV_ASSET_EDITOR_DECLARE(Type, Name)
#endif

//This macro is for adding a new soft asset pointer to the Dev Settings.
//It includes the declarations for a getter and the static pointer that is used after loading the asset.
//Also, in editor, includes a lazy load function for use at edit time.
#define SAIYORA_DEV_ASSET(Type, Name) \
public: \
	static FORCEINLINE Type* Get##Name() \
	{ \
		SAIYORA_DEV_ASSET_EDITOR_FALLBACK(Name) \
		return Static_##Name; \
	} \
private: \
	static Type* Static_##Name; \
	SAIYORA_DEV_ASSET_EDITOR_DECLARE(Type, Name);

#pragma endregion 

UCLASS(Config = Game, DefaultConfig)
class SAIYORAV4_API USaiyoraDevSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	static void InitForRuntime();
	static void ClearDataAssetRefs();

private:

	//Loads a given asset. bDuplicate is used at runtime so that we create a copy of the asset.
	//At edit time, we do not duplicate the asset, as we want changes made to persist.
	template <typename T>
	static T* LoadDevAsset(const TSoftObjectPtr<T>& Asset, const bool bDuplicate)
	{
		T* Loaded = Asset.LoadSynchronous();
		if (Loaded && bDuplicate)
		{
			Loaded = DuplicateObject(Loaded, nullptr);
		}
		if (IsValid(Loaded))
		{
			Loaded->AddToRoot();
		}
		return Loaded;
	}
#if WITH_EDITOR
	//We only lazy-load assets at edit time. Otherwise, they should never be null after GameInstance init.
	static bool CanLazyLoadAsset();
#endif

	UPROPERTY(Config, EditAnywhere, Category = "DataAssets")
	TSoftObjectPtr<UCombatDebugOptions> CombatDebugDataAsset;
	SAIYORA_DEV_ASSET(UCombatDebugOptions, CombatDebugDataAsset)

	UPROPERTY(Config, EditAnywhere, Category = "DataAssets")
	TSoftObjectPtr<USaiyoraUIDataAsset> UIDataAsset;
	SAIYORA_DEV_ASSET(USaiyoraUIDataAsset, UIDataAsset)

	UPROPERTY(Config, EditAnywhere, Category = "DataAssets")
	TSoftObjectPtr<UMapsDataAsset> MapsDataAsset;
	SAIYORA_DEV_ASSET(UMapsDataAsset, MapsDataAsset)
};
