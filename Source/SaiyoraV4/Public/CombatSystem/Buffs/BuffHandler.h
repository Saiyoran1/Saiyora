#pragma once
#include "BuffStructs.h"
#include "DamageStructs.h"
#include "Components/ActorComponent.h"
#include "BuffHandler.generated.h"

class USaiyoraMovementComponent;
class UThreatHandler;
class UCrowdControlHandler;
class UStatHandler;
class UDamageHandler;
class UCombatStatusComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UBuffHandler : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UBuffHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

private:

	UPROPERTY()
	UCombatStatusComponent* CombatStatusComponentRef;
	UPROPERTY()
	UDamageHandler* DamageHandlerRef;
	UPROPERTY()
	UStatHandler* StatHandlerRef;
	UPROPERTY()
	UCrowdControlHandler* CcHandlerRef;
	UPROPERTY()
	UThreatHandler* ThreatHandlerRef;
	UPROPERTY()
	USaiyoraMovementComponent* MovementComponentRef;
	
//Incoming Buffs

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs", meta = (AutoCreateRefTerm = "BuffParams"))
	FBuffApplyEvent ApplyBuff(const TSubclassOf<UBuff> BuffClass, AActor* AppliedBy, UObject* Source,
		const bool DuplicateOverride, const EBuffApplicationOverrideType StackOverrideType, const int32 OverrideStacks,
		const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration, const bool IgnoreRestrictions,
		const TArray<FCombatParameter>& BuffParams);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
	FBuffRemoveEvent RemoveBuff(UBuff* Buff, const EBuffExpireReason ExpireReason);
	UFUNCTION(BlueprintPure, Category = "Buffs")
	bool CanEverReceiveBuffs() const { return bCanEverReceiveBuffs; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffs(TArray<UBuff*>& OutBuffs) const { OutBuffs = Buffs; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetDebuffs(TArray<UBuff*>& OutDebuffs) const { OutDebuffs = Debuffs; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetHiddenBuffs(TArray<UBuff*>& OutHiddenBuffs) const { OutHiddenBuffs = HiddenBuffs; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffsOfClass(const TSubclassOf<UBuff> BuffClass, TArray<UBuff*>& OutBuffs) const;
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffsAppliedByActor(const AActor* Actor, TArray<UBuff*>& OutBuffs) const;
	UFUNCTION(BlueprintPure, Category = "Buff")
	UBuff* FindExistingBuff(const TSubclassOf<UBuff> BuffClass, const bool bSpecificOwner = false, const AActor* BuffOwner = nullptr) const;

	UPROPERTY(BlueprintAssignable)
	FBuffEventNotification OnIncomingBuffApplied;
	UPROPERTY(BlueprintAssignable)
	FBuffRemoveNotification OnIncomingBuffRemoved;

	void AddIncomingBuffRestriction(const FBuffRestriction& Restriction) { IncomingBuffRestrictions.Add(Restriction); }
	void RemoveIncomingBuffRestriction(const FBuffRestriction& Restriction) { IncomingBuffRestrictions.Add(Restriction); }
	bool CheckIncomingBuffRestricted(const FBuffApplyEvent& BuffEvent);
	
	void NotifyOfNewIncomingBuff(const FBuffApplyEvent& ApplicationEvent);
	void NotifyOfIncomingBuffRemoval(const FBuffRemoveEvent& RemoveEvent);

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
	TConditionalRestrictionList<FBuffRestriction> IncomingBuffRestrictions;
	UFUNCTION()
	void RemoveBuffsOnOwnerDeath(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);

//Outgoing Buffs

public:

	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetOutgoingBuffs(TArray<UBuff*>& OutBuffs) const { OutBuffs = OutgoingBuffs; }
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetOutgoingBuffsOfClass(const TSubclassOf<UBuff> BuffClass, TArray<UBuff*>& OutBuffs) const;
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffsAppliedToActor(const AActor* Target, TArray<UBuff*>& OutBuffs) const;
	UFUNCTION(BlueprintPure, Category = "Buff")
	UBuff* FindExistingOutgoingBuff(const TSubclassOf<UBuff> BuffClass, const bool bSpecificTarget = false, const AActor* BuffTarget = nullptr) const;

	UPROPERTY(BlueprintAssignable)
	FBuffEventNotification OnOutgoingBuffApplied;
	UPROPERTY(BlueprintAssignable)
	FBuffRemoveNotification OnOutgoingBuffRemoved;

	void AddOutgoingBuffRestriction(const FBuffRestriction& Restriction) { OutgoingBuffRestrictions.Add(Restriction); }
	void RemoveOutgoingBuffRestriction(const FBuffRestriction& Restriction) { OutgoingBuffRestrictions.Remove(Restriction); }
	bool CheckOutgoingBuffRestricted(const FBuffApplyEvent& BuffEvent);

	void NotifyOfNewOutgoingBuff(const FBuffApplyEvent& ApplicationEvent);
	void NotifyOfOutgoingBuffRemoval(const FBuffRemoveEvent& RemoveEvent);
	
private:

	UPROPERTY()
	TArray<UBuff*> OutgoingBuffs;
	TConditionalRestrictionList<FBuffRestriction> OutgoingBuffRestrictions;
};
