#pragma once
#include "CoreMinimal.h"
#include "BuffStructs.h"
#include "CrowdControlStructs.h"
#include "DamageStructs.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CrowdControlHandler.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UCrowdControlHandler : public UActorComponent
{
	GENERATED_BODY()

public:
	static FGameplayTag GenericCrowdControlTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl")), false); }
	static FGameplayTag StunTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Stun")), false); }
	static FGameplayTag IncapacitateTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Incapacitate")), false); }
	static FGameplayTag RootTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Root")), false); }
	static FGameplayTag SilenceTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Silence")), false); }
	static FGameplayTag DisarmTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CrowdControl.Disarm")), false); }
	static FGameplayTag GenericCcImmunityTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CcImmunity")), false); }
	static FGameplayTag StunImmunityTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CcImmunity.Stun")), false); }
	static FGameplayTag IncapImmunityTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CcImmunity.Incap")), false); }
	static FGameplayTag RootImmunityTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CcImmunity.Root")), false); }
	static FGameplayTag SilenceImmunityTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CcImmunity.Silence")), false); }
	static FGameplayTag DisarmImmunityTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("CcImmunity.Disarm")), false); }
	static FGameplayTag CcTypeToTag(ECrowdControlType const CcType);
	static FGameplayTag CcTypeToImmunity(ECrowdControlType const CcType);
	static ECrowdControlType CcTagToType(FGameplayTag const& CcTag);
	static ECrowdControlType CcImmunityToType(FGameplayTag const& ImmunityTag);
	
	UCrowdControlHandler();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void SubscribeToCrowdControlChanged(FCrowdControlCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void UnsubscribeFromCrowdControlChanged(FCrowdControlCallback const& Callback);
	
private:
	UPROPERTY(EditAnywhere, Category = "Crowd Control", meta = (AllowPrivateAccess = true))
	TSet<ECrowdControlType> DefaultCrowdControlImmunities;
	TMap<ECrowdControlType, int32> CrowdControlImmunities;
	void AddImmunity(ECrowdControlType const CcType);
	void RemoveImmunity(ECrowdControlType const CcType);
	void PurgeCcOfType(ECrowdControlType const CcType);
	
	FCrowdControlStatus* GetCcStruct(ECrowdControlType const CcType);
	FCrowdControlNotification OnCrowdControlChanged;
	
	UPROPERTY(ReplicatedUsing=OnRep_StunStatus)
	FCrowdControlStatus StunStatus;
	UFUNCTION()
	void OnRep_StunStatus(FCrowdControlStatus const& Previous);
	UPROPERTY(ReplicatedUsing=OnRep_IncapStatus)
	FCrowdControlStatus IncapStatus;
	UFUNCTION()
	void OnRep_IncapStatus(FCrowdControlStatus const& Previous);
	UPROPERTY(ReplicatedUsing=OnRep_RootStatus)
	FCrowdControlStatus RootStatus;
	UFUNCTION()
	void OnRep_RootStatus(FCrowdControlStatus const& Previous);
	UPROPERTY(ReplicatedUsing=OnRep_SilenceStatus)
	FCrowdControlStatus SilenceStatus;
	UFUNCTION()
	void OnRep_SilenceStatus(FCrowdControlStatus const& Previous);
	UPROPERTY(ReplicatedUsing=OnRep_DisarmStatus)
	FCrowdControlStatus DisarmStatus;
	UFUNCTION()
	void OnRep_DisarmStatus(FCrowdControlStatus const& Previous);
	
	UPROPERTY()
	class UBuffHandler* BuffHandler;
	UFUNCTION()
	bool CheckBuffRestrictedByCcImmunity(FBuffApplyEvent const& BuffEvent);
	FBuffEventCondition BuffCcRestriction;
	UFUNCTION()
	void CheckAppliedBuffForCcOrImmunity(FBuffApplyEvent const& BuffEvent);
	FBuffEventCallback OnBuffApplied;
	UFUNCTION()
	void CheckRemovedBuffForCcOrImmunity(FBuffRemoveEvent const& RemoveEvent);
	FBuffRemoveCallback OnBuffRemoved;
	UPROPERTY()
	class UDamageHandler* DamageHandler;
	UFUNCTION()
	void RemoveIncapacitatesOnDamageTaken(FDamagingEvent const& DamageEvent);
	FDamageEventCallback OnDamageTaken;
};