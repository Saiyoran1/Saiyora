#pragma once
#include "CoreMinimal.h"
#include "BuffFunctionality.h"
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

UCLASS()
class SAIYORAV4_API UFadeFunction : public UBuffFunction
{
	GENERATED_BODY()

	UPROPERTY()
	UThreatHandler* TargetHandler;

	void SetFadeVars();

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void Fade(UBuff* Buff);
};

UCLASS()
class SAIYORAV4_API UFixateFunction : public UBuffFunction
{
	GENERATED_BODY()

	UPROPERTY()
	UThreatHandler* TargetHandler;
	UPROPERTY()
	AActor* FixateTarget;

	void SetFixateVars(AActor* Target);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void Fixate(UBuff* Buff, AActor* Target);
};

UCLASS()
class SAIYORAV4_API UBlindFunction : public UBuffFunction
{
	GENERATED_BODY()

	UPROPERTY()
	UThreatHandler* TargetHandler;
	UPROPERTY()
	AActor* BlindTarget;

	void SetBlindVars(AActor* Target);

	virtual void OnApply(const FBuffApplyEvent& ApplyEvent) override;
	virtual void OnRemove(const FBuffRemoveEvent& RemoveEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Buff Function", meta = (DefaultToSelf = "Buff", HidePin = "Buff"))
	static void Blind(UBuff* Buff, AActor* Target);
};