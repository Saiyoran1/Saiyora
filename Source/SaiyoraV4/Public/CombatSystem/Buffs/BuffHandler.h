// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BuffStructs.h"
#include "Components/ActorComponent.h"
#include "BuffHandler.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UBuffHandler : public UActorComponent
{
	GENERATED_BODY()

public:	

	//Engine functions.
	
	UBuffHandler();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	//Server functions called by the BuffLibrary during application and removal.

	bool CheckIncomingBuffRestricted(FBuffApplyEvent const& BuffEvent);
	bool CheckOutgoingBuffRestricted(FBuffApplyEvent const& BuffEvent);
	void ApplyBuff(FBuffApplyEvent& BuffEvent);
	void RemoveBuff(FBuffRemoveEvent& RemoveEvent);
	void SuccessfulOutgoingBuffApplication(FBuffApplyEvent const& BuffEvent);
	void SuccessfulOutgoingBuffRemoval(FBuffRemoveEvent const& RemoveEvent);

private:

	//Internal function for actually instantiating a buff object during application.
	void CreateNewBuff(FBuffApplyEvent& BuffEvent);
	//This function exists to allow buff removal events to replicate properly.
	UFUNCTION()
    void PostRemoveCleanup(UBuff* Buff);

public:

	//Functions called on the client side by buff objects after replication.
	
	void NotifyOfReplicatedIncomingBuffApply(FBuffApplyEvent const& ReplicatedEvent);
	void NotifyOfReplicatedIncomingBuffRemove(FBuffRemoveEvent const& ReplicatedEvent);
	void NotifyOfReplicatedOutgoingBuffApply(FBuffApplyEvent const& ReplicatedEvent);
	void NotifyOfReplicatedOutgoingBuffRemove(FBuffRemoveEvent const& ReplicatedEvent);
	
	//Blueprint callable functions for functionality that only involves this BuffHandler.

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetBuffs() const { return Buffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetDebuffs() const { return Debuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetHiddenBuffs() const { return HiddenBuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetBuffsOfClass(TSubclassOf<UBuff> const BuffClass) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetOutgoingBuffs() const { return OutgoingBuffs; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	TArray<UBuff*> GetOutgoingBuffsOfClass(TSubclassOf<UBuff> const BuffClass) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Buff")
	UBuff* FindExistingBuff(TSubclassOf<UBuff> const BuffClass, AActor* Owner) const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff")
	void AddIncomingBuffRestriction(FBuffEventCondition const& Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff")
	void RemoveIncomingBuffRestriction(FBuffEventCondition const& Restriction);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToIncomingBuffSuccess(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromIncomingBuffSuccess(FBuffEventCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void SubscribeToIncomingBuffRemove(FBuffRemoveCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnsubscribeFromIncomingBuffRemove(FBuffRemoveCallback const& Callback);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
    void AddOutgoingBuffRestriction(FBuffEventCondition const & Restriction);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
    void RemoveOutgoingBuffRestriction(FBuffEventCondition const & Restriction);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
    void SubscribeToOutgoingBuffSuccess(FBuffEventCallback const & Callback);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
    void UnsubscribeFromOutgoingBuffSuccess(FBuffEventCallback const & Callback);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
    void SubscribeToOutgoingBuffRemove(FBuffRemoveCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Buffs")
    void UnsubscribeFromOutgoingBuffRemove(FBuffRemoveCallback const& Callback);

private:
	
	UPROPERTY()
	TArray<UBuff*> Buffs;
	UPROPERTY()
	TArray<UBuff*> Debuffs;
	UPROPERTY()
	TArray<UBuff*> HiddenBuffs;
	UPROPERTY()
	TArray<UBuff*> RecentlyRemoved;
	UPROPERTY()
	TArray<UBuff*> OutgoingBuffs;

	FBuffEventNotification OnIncomingBuffApplied;
	FBuffRemoveNotification OnIncomingBuffRemoved;
	TArray<FBuffEventCondition> IncomingBuffConditions;
	
	FBuffEventNotification OnOutgoingBuffApplied;
	FBuffRemoveNotification OnOutgoingBuffRemoved;
	TArray<FBuffEventCondition> OutgoingBuffConditions;
};
