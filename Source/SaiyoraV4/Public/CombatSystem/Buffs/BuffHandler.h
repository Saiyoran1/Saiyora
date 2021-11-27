#pragma once
#include "BuffStructs.h"
#include "Components/ActorComponent.h"
#include "BuffHandler.generated.h"

class UPlaneComponent;

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
	UPlaneComponent* PlaneComponent;

//Incoming Buffs

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs", meta = (AutoCreateRefTerm = "BuffParams"))
	FBuffApplyEvent ApplyBuff(TSubclassOf<UBuff> const BuffClass, AActor* const AppliedBy, UObject* const Source,
		bool const DuplicateOverride, EBuffApplicationOverrideType const StackOverrideType, int32 const OverrideStacks,
		EBuffApplicationOverrideType const RefreshOverrideType, float const OverrideDuration, bool const IgnoreRestrictions,
		TArray<FCombatParameter> const& BuffParams);
	void RemoveBuff(FBuffRemoveEvent& RemoveEvent);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buffs")
	bool CanEverReceiveBuffs() const { return bCanEverReceiveBuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetBuffs() const { return Buffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetDebuffs() const { return Debuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetHiddenBuffs() const { return HiddenBuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetBuffsOfClass(TSubclassOf<UBuff> const BuffClass) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UBuff* FindExistingBuff(TSubclassOf<UBuff> const BuffClass, AActor* Owner) const;

	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToIncomingBuff(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromIncomingBuff(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToIncomingBuffRemove(FBuffRemoveCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromIncomingBuffRemove(FBuffRemoveCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff")
	void AddIncomingBuffRestriction(FBuffRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff")
	void RemoveIncomingBuffRestriction(FBuffRestriction const& Restriction);
	bool CheckIncomingBuffRestricted(FBuffApplyEvent const& BuffEvent);
	
	void NotifyOfReplicatedIncomingBuffApply(FBuffApplyEvent const& ReplicatedEvent);
	void NotifyOfReplicatedIncomingBuffRemove(FBuffRemoveEvent const& ReplicatedEvent);

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
	TArray<FBuffRestriction> IncomingBuffRestrictions;

//Outgoing Buffs

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetOutgoingBuffs() const { return OutgoingBuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetOutgoingBuffsOfClass(TSubclassOf<UBuff> const BuffClass) const;

	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void SubscribeToOutgoingBuff(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void UnsubscribeFromOutgoingBuff(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void SubscribeToOutgoingBuffRemove(FBuffRemoveCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void UnsubscribeFromOutgoingBuffRemove(FBuffRemoveCallback const& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
	void AddOutgoingBuffRestriction(FBuffRestriction const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
	void RemoveOutgoingBuffRestriction(FBuffRestriction const& Restriction);
	bool CheckOutgoingBuffRestricted(FBuffApplyEvent const& BuffEvent);

	void NotifyOfOutgoingBuffApplication(FBuffApplyEvent const& BuffEvent);
	void SuccessfulOutgoingBuffRemoval(FBuffRemoveEvent const& RemoveEvent);
	void NotifyOfReplicatedOutgoingBuffApply(FBuffApplyEvent const& ReplicatedEvent);
	void NotifyOfReplicatedOutgoingBuffRemove(FBuffRemoveEvent const& ReplicatedEvent);

private:

	UPROPERTY()
	TArray<UBuff*> OutgoingBuffs;
	FBuffEventNotification OnOutgoingBuffApplied;
	FBuffRemoveNotification OnOutgoingBuffRemoved;
	TArray<FBuffRestriction> OutgoingBuffRestrictions;
};
