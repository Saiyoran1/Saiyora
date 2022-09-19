#pragma once
#include "CoreMinimal.h"
#include "BuffFunction.h"
#include "ThreatStructs.h"
#include "ThreatBuffFunctions.generated.h"

class UThreatHandler;

UCLASS()
class SAIYORAV4_API UThreatModifierFunction : public UBuffFunction
{
	GENERATED_BODY()

	FThreatModCondition Mod;
    EThreatModifierType ModType;
    UPROPERTY()
    UThreatHandler* TargetHandler;
    
    void SetModifierVars(EThreatModifierType const ModifierType, FThreatModCondition const& Modifier);
    
    virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
    virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;
    
    UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
    static void ThreatModifier(UBuff* Buff, EThreatModifierType const ModifierType, FThreatModCondition const& Modifier);
};

UCLASS()
class SAIYORAV4_API UThreatRestrictionFunction : public UBuffFunction
{
	GENERATED_BODY()

	FThreatRestriction Restrict;
	EThreatModifierType RestrictType;
	UPROPERTY()
	UThreatHandler* TargetHandler;

	void SetRestrictionVars(EThreatModifierType const RestrictionType, FThreatRestriction const& Restriction);

	virtual void OnApply(FBuffApplyEvent const& ApplyEvent) override;
	virtual void OnRemove(FBuffRemoveEvent const& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void ThreatRestriction(UBuff* Buff, EThreatModifierType const RestrictionType, FThreatRestriction const& Restriction);
};