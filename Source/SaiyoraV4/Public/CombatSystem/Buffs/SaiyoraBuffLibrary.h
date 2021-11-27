#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "SaiyoraStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaiyoraBuffLibrary.generated.h"

class UBuffHandler;

UCLASS()
class SAIYORAV4_API USaiyoraBuffLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//Attempt to remove a buff from an actor.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
	static FBuffRemoveEvent RemoveBuff(UBuff* Buff, EBuffExpireReason const ExpireReason);
};
