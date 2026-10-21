#include "SaiyoraGameInstance.h"
#include "SaiyoraDevSettings.h"

void USaiyoraGameInstance::Init()
{
	USaiyoraDevSettings::InitForRuntime();
	Super::Init();
}

void USaiyoraGameInstance::Shutdown()
{
	Super::Shutdown();
	USaiyoraDevSettings::ClearDataAssetRefs();
}