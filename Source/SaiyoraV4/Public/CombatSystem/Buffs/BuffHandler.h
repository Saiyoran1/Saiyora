#pragma once
#include "BuffStructs.h"
#include "DamageStructs.h"
#include "Components/ActorComponent.h"
#include "BuffHandler.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UBuffHandler : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UBuffHandler();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

private:

	UPROPERTY()
	class UPlaneComponent* PlaneComponentRef;
	UPROPERTY()
	class UDamageHandler* DamageHandlerRef;
	UPROPERTY()
	class UStatHandler* StatHandlerRef;
	UPROPERTY()
	class UCrowdControlHandler* CcHandlerRef;
	UPROPERTY()
	class UThreatHandler* ThreatHandlerRef;
	UPROPERTY()
	class USaiyoraMovementComponent* MovementComponentRef;
	
//Incoming Buffs

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs", meta = (AutoCreateRefTerm = "BuffParams"))
	FBuffApplyEvent ApplyBuff(TSubclassOf<UBuff> const BuffClass, AActor* const AppliedBy, UObject* const Source,
		bool const DuplicateOverride, EBuffApplicationOverrideType const StackOverrideType, int32 const OverrideStacks,
		EBuffApplicationOverrideType const RefreshOverrideType, float const OverrideDuration, bool const IgnoreRestrictions,
		TArray<FCombatParameter> const& BuffParams);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
	FBuffRemoveEvent RemoveBuff(UBuff* Buff, EBuffExpireReason const ExpireReason);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buffs")
	bool CanEverReceiveBuffs() const { return bCanEverReceiveBuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetBuffs(TArray<UBuff*>& OutBuffs) const { OutBuffs = Buffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetDebuffs(TArray<UBuff*>& OutDebuffs) const { OutDebuffs = Debuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetHiddenBuffs(TArray<UBuff*>& OutHiddenBuffs) const { OutHiddenBuffs = HiddenBuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetBuffsOfClass(TSubclassOf<UBuff> const BuffClass, TArray<UBuff*>& OutBuffs) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetBuffsAppliedByActor(AActor* Actor, TArray<UBuff*>& OutBuffs) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UBuff* FindExistingBuff(TSubclassOf<UBuff> const BuffClass, bool const bSpecificOwner = false, AActor* BuffOwner = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToIncomingBuff(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromIncomingBuff(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToIncomingBuffRemove(FBuffRemoveCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromIncomingBuffRemove(FBuffRemoveCallback const& Callback);

	void AddIncomingBuffRestriction(UBuff* Source, FBuffRestriction const& Restriction);
	void RemoveIncomingBuffRestriction(UBuff* Source);
	bool CheckIncomingBuffRestricted(FBuffApplyEvent const& BuffEvent);
	
	void NotifyOfNewIncomingBuff(FBuffApplyEvent const& ApplicationEvent);
	void NotifyOfIncomingBuffRemoval(FBuffRemoveEvent const& RemoveEvent);

private:

	UPROPERTY(EditAnywhere, Category = "Buffs")
	bool bCanEverReceiveBuffs = true;
	UPROPERTY()
	TArray<UBuff*> Buffs;
	UPROPERTY()
	TArray<UBuff*> Debuffs;
	UPROPERTY()
	TArray<UBuff*> HiddenBuffs;
	UFUNCTION()
	void PostRemoveCleanup(UBuff* Buff);
	UPROPERTY()
	TArray<UBuff*> RecentlyRemoved;
	FBuffEventNotification OnIncomingBuffApplied;
	FBuffRemoveNotification OnIncomingBuffRemoved;
	TMap<UBuff*, FBuffRestriction> IncomingBuffRestrictions;
	FLifeStatusCallback OnDeath;
	UFUNCTION()
	void RemoveBuffsOnOwnerDeath(AActor* Actor, ELifeStatus const PreviousStatus, ELifeStatus const NewStatus);

//Outgoing Buffs

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetOutgoingBuffs(TArray<UBuff*>& OutBuffs) const { OutBuffs = OutgoingBuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetOutgoingBuffsOfClass(TSubclassOf<UBuff> const BuffClass, TArray<UBuff*>& OutBuffs) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	void GetBuffsAppliedToActor(AActor* Target, TArray<UBuff*>& OutBuffs) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UBuff* FindExistingOutgoingBuff(TSubclassOf<UBuff> const BuffClass, bool const bSpecificTarget = false, AActor* BuffTarget = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToOutgoingBuff(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromOutgoingBuff(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToOutgoingBuffRemove(FBuffRemoveCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromOutgoingBuffRemove(FBuffRemoveCallback const& Callback);

	void AddOutgoingBuffRestriction(UBuff* Source, FBuffRestriction const& Restriction);
	void RemoveOutgoingBuffRestriction(UBuff* Source);
	bool CheckOutgoingBuffRestricted(FBuffApplyEvent const& BuffEvent);

	void NotifyOfNewOutgoingBuff(FBuffApplyEvent const& ApplicationEvent);
	void NotifyOfOutgoingBuffRemoval(FBuffRemoveEvent const& RemoveEvent);
	
private:

	UPROPERTY()
	TArray<UBuff*> OutgoingBuffs;
	FBuffEventNotification OnOutgoingBuffApplied;
	FBuffRemoveNotification OnOutgoingBuffRemoved;
	TMap<UBuff*, FBuffRestriction> OutgoingBuffRestrictions;
};
