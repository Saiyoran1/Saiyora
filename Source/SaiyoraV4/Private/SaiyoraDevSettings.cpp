#include "SaiyoraDevSettings.h"
#include "CombatDebugOptions.h"
#include "MapsDataAsset.h"
#include "SaiyoraUIDataAsset.h"
#include "Engine/World.h"

//A macro that lets us pass other macros into it to perform on every asset in this list.
#define SAIYORA_DEV_ASSET_LIST(Action) \
	Action(UCombatDebugOptions, CombatDebugDataAsset) \
	Action(USaiyoraUIDataAsset, UIDataAsset) \
	Action(UMapsDataAsset, MapsDataAsset)

#pragma region Macros

#if WITH_EDITOR
//With the editor, this is the definition of Editor_LazyLoad(), which is the function that loads the asset at edit time if needed.
//Pass false into LoadDevAsset because we don't duplicate assets at edit time, only at runtime (so that changes made during gameplay aren't saved).
#define SAIYORA_DEV_ASSET_EDITOR_DEFINE(Type, Name) \
	Type* USaiyoraDevSettings::Editor_LazyLoad##Name() \
	{ \
		if (!IsValid(Static_##Name) && CanLazyLoadAsset()) \
		{ \
			Static_##Name = LoadDevAsset(GetDefault<USaiyoraDevSettings>()->Name, false); \
		} \
		return Static_##Name; \
	}	
#else
//Compile out the lazy load function if no editor
#define SAIYORA_DEV_ASSET_EDITOR_DEFINE(Type, Name)
#endif

//This macro sets the default value of the static asset pointer to null and (if compiled with editor) defines the lazy load function above.
#define SAIYORA_DEV_ASSET_DEFINE(Type, Name) \
	Type* USaiyoraDevSettings::Static_##Name = nullptr; \
	SAIYORA_DEV_ASSET_EDITOR_DEFINE(Type, Name)

//This calls the define macro (directly above) for everything in the SAIYORA_DEV_ASSET_LIST at the top of this file.
SAIYORA_DEV_ASSET_LIST(SAIYORA_DEV_ASSET_DEFINE)

//Macro that initializes an asset during InitForRuntime, loading and duplicating it and setting the static asset pointer.
#define SAIYORA_DEV_ASSET_INIT(Type, Name) \
	{ \
		const TSoftObjectPtr<Type>& Asset = GetDefault<USaiyoraDevSettings>()->Name; \
		checkf(!Asset.IsNull(), TEXT("%s in the developer settings was null!"), TEXT(#Name)); \
		Static_##Name = LoadDevAsset(Asset, true); \
		checkf(IsValid(Static_##Name), TEXT("Duplicating or Loading %s failed!"), TEXT(#Name)); \
	}

//Clears the static asset pointer before init and after shutdown, so we don't have dangling pointers between edit time and runtime.
#define SAIYORA_DEV_ASSET_CLEAR(Type, Name) \
	if (IsValid(Static_##Name)) \
	{ \
		Static_##Name->RemoveFromRoot(); \
	} \
	Static_##Name = nullptr;

#pragma endregion 

void USaiyoraDevSettings::InitForRuntime()
{
	ClearDataAssetRefs();
	SAIYORA_DEV_ASSET_LIST(SAIYORA_DEV_ASSET_INIT)
}

void USaiyoraDevSettings::ClearDataAssetRefs()
{
	SAIYORA_DEV_ASSET_LIST(SAIYORA_DEV_ASSET_CLEAR)
}

#if WITH_EDITOR
bool USaiyoraDevSettings::CanLazyLoadAsset()
{
	return !GWorld || (GWorld->IsEditorWorld() && !GWorld->IsGameWorld());
}
#endif