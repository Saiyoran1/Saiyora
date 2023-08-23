#include "DungeonPostProcess.h"

#include "Materials/MaterialInstanceConstant.h"

ADungeonPostProcess::ADungeonPostProcess()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UMaterialInstanceConstant> OutlinePostProcess(TEXT("MaterialInstanceConstant'/Game/Saiyora/UI/PostProcess/Outlines/MI_CombatOutlines.MI_CombatOutlines'"));
	if (IsValid(OutlinePostProcess.Object))
	{
		AddOrUpdateBlendable(OutlinePostProcess.Object);
	}
	bUnbound = true;
}
