#pragma once
#include "BuffStructs.h"
#include "DamageStructs.h"
#include "NPCEnums.h"
#include "Components/ActorComponent.h"
#include "BuffHandler.generated.h"

class UNPCAbilityComponent;
class USaiyoraMovementComponent;
class UThreatHandler;
class UCrowdControlHandler;
class UStatHandler;
class UDamageHandler;
class UCombatStatusComponent;

//Component that handles applying and removing buffs to and from the owning actor.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UBuffHandler : public UActorComponent
{
	GENERATED_BODY()

#pragma region Init

public:	
	
	UBuffHandler();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	UPROPERTY()
	UNPCAbilityComponent* NPCComponentRef;

#pragma endregion 
#pragma region Incoming Buffs

public:

	//Main function for applying a buff to a target. Has parameters for overriding stacking/refreshing/duplication behavior per buff, and can accept custom parameters for the buff to use.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs", meta = (AutoCreateRefTerm = "BuffParams", BaseStruct = "/Script/SaiyoraV4.CombatParameter"))
	FBuffApplyEvent ApplyBuff(const TSubclassOf<UBuff> BuffClass, AActor* AppliedBy, UObject* Source,
		const bool DuplicateOverride, const EBuffApplicationOverrideType StackOverrideType, const int32 OverrideStacks,
		const EBuffApplicationOverrideType RefreshOverrideType, const float OverrideDuration, const bool IgnoreRestrictions,
		const TArray<FInstancedStruct>& BuffParams);
	//Main function for removing an existing buff from a target.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buffs")
	FBuffRemoveEvent RemoveBuff(UBuff* Buff, const EBuffExpireReason ExpireReason);
	//Whether the target can ever receive any buffs. This value does not change during gameplay.
	//Actors may want to have outgoing buffs and restrictions without being buffed themselves.
	UFUNCTION(BlueprintPure, Category = "Buffs")
	bool CanEverReceiveBuffs() const { return bCanEverReceiveBuffs; }
	//Get buffs applied to this actor (not debuffs or hidden buffs).
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffs(TArray<UBuff*>& OutBuffs) const { OutBuffs = Buffs; }
	//Get debuffs applied to this actor.
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetDebuffs(TArray<UBuff*>& OutDebuffs) const { OutDebuffs = Debuffs; }
	//Get hidden buffs applied to this actor.
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetHiddenBuffs(TArray<UBuff*>& OutHiddenBuffs) const { OutHiddenBuffs = HiddenBuffs; }
	//Get buffs applied to this actor of a specific class.
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffsOfClass(const TSubclassOf<UBuff> BuffClass, TArray<UBuff*>& OutBuffs) const;
	//Get buffs applied to this actor by another specific actor.
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffsAppliedByActor(const AActor* Actor, TArray<UBuff*>& OutBuffs) const;
	//Get an existing buff of a given class, optionally from a specific owner.
	UFUNCTION(BlueprintPure, Category = "Buff")
	UBuff* FindExistingBuff(const TSubclassOf<UBuff> BuffClass, const bool bSpecificOwner = false, const AActor* BuffOwner = nullptr) const;

	//Called when any buff is applied to this actor.
	UPROPERTY(BlueprintAssignable)
	FBuffEventNotification OnIncomingBuffApplied;
	//Called when any buff is removed from this actor.
	UPROPERTY(BlueprintAssignable)
	FBuffRemoveNotification OnIncomingBuffRemoved;

	//Adds a restriction on buffs applied to this actor.
	void AddIncomingBuffRestriction(const FBuffRestriction& Restriction) { IncomingBuffRestrictions.Add(Restriction); }
	//Removes a restriction on buffs applied to this actor.
	void RemoveIncomingBuffRestriction(const FBuffRestriction& Restriction) { IncomingBuffRestrictions.Remove(Restriction); }
	//Check during the buff application process for whether a specific buff can be applied to this actor.
	bool CheckIncomingBuffRestricted(const FBuffApplyEvent& BuffEvent, TArray<EBuffApplyFailReason>& OutFailReasons);

	//Called by new buff instances applied to this actor after they are initialized to notify the handler to add them to the correct array and fire off delegates.
	void NotifyOfNewIncomingBuff(const FBuffApplyEvent& ApplicationEvent);
	//Called by buffs applied to this actor when they are removed, to notify the handler to remove them from the correct array and fire off delegates.
	void NotifyOfIncomingBuffRemoval(const FBuffRemoveEvent& RemoveEvent);

private:

	//Blanket restriction that prevents all incoming buffs for this actor.
	//Does not change during gameplay.
	UPROPERTY(EditAnywhere, Category = "Buffs")
	bool bCanEverReceiveBuffs = true;
	//One of three arrays of active buffs applied to this actor.
	//These are typically "positive" effects.
	UPROPERTY()
	TArray<UBuff*> Buffs;
	//One of three arrays of active buffs applied to this actor.
	//These are typically "negative" effects.
	UPROPERTY()
	TArray<UBuff*> Debuffs;
	//One of three arrays of active buffs applied to this actor.
	//These effects are hidden from player UI.
	UPROPERTY()
	TArray<UBuff*> HiddenBuffs;
	//Called after removing a buff on the server to stop replicating it and drop the pointer.
	UFUNCTION()
	void PostRemoveCleanup(UBuff* Buff);
	//Array of recently removed buffs on the server that we still want to replicate for a short time.
	UPROPERTY()
	TArray<UBuff*> RecentlyRemoved;
	//Restrictions to incoming buffs.
	TConditionalRestrictionList<FBuffRestriction> IncomingBuffRestrictions;
	//Callback from DamageHandler's OnLifeStatusChanged
	//When the owning actor's life status changes from Alive, we want to remove all buffs that don't persist through death.
	UFUNCTION()
	void RemoveBuffsOnOwnerDeath(AActor* Actor, const ELifeStatus PreviousStatus, const ELifeStatus NewStatus);
	//Callback from NPCAbilityComponent's OnCombatBehaviorChanged
	//If our owner is an NPC and leaves combat, we remove all buffs that don't persist outside of combat.
	UFUNCTION()
	void RemoveBuffsOnCombatEnd(const ENPCCombatBehavior PreviousBehavior, const ENPCCombatBehavior NewBehavior);

#pragma endregion 
#pragma region Outgoing Buffs

public:

	//Get all buffs applied by this actor.
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetOutgoingBuffs(TArray<UBuff*>& OutBuffs) const { OutBuffs = OutgoingBuffs; }
	//Get all buffs of a specific class applied by this actor to other actors.
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetOutgoingBuffsOfClass(const TSubclassOf<UBuff> BuffClass, TArray<UBuff*>& OutBuffs) const;
	//Get all buffs this actor has applied to a specific target.
	UFUNCTION(BlueprintPure, Category = "Buff")
	void GetBuffsAppliedToActor(const AActor* Target, TArray<UBuff*>& OutBuffs) const;
	//Find a buff of a given class that this actor has applied, optionally to a specific target.
	UFUNCTION(BlueprintPure, Category = "Buff")
	UBuff* FindExistingOutgoingBuff(const TSubclassOf<UBuff> BuffClass, const bool bSpecificTarget = false, const AActor* BuffTarget = nullptr) const;

	//Delegate called any time this actor applies a buff to another actor.
	UPROPERTY(BlueprintAssignable)
	FBuffEventNotification OnOutgoingBuffApplied;
	//Delegate called when a buff this actor applied to someone is removed.
	UPROPERTY(BlueprintAssignable)
	FBuffRemoveNotification OnOutgoingBuffRemoved;

	//Add a restriction to buffs applied by this actor.
	void AddOutgoingBuffRestriction(const FBuffRestriction& Restriction) { OutgoingBuffRestrictions.Add(Restriction); }
	//Remove a restriction to buffs applied by this actor.
	void RemoveOutgoingBuffRestriction(const FBuffRestriction& Restriction) { OutgoingBuffRestrictions.Remove(Restriction); }
	//During the buff application process, this function checks whether we are allowed to apply a given buff to another actor.
	bool CheckOutgoingBuffRestricted(const FBuffApplyEvent& BuffEvent, TArray<EBuffApplyFailReason>& OutFailReasons);

	//Called by new buff instances after initialization to add them to our array and fire off delegates.
	void NotifyOfNewOutgoingBuff(const FBuffApplyEvent& ApplicationEvent);
	//Called when an outgoing buff is removed so that we can remove it from our array and fire off delegates.
	void NotifyOfOutgoingBuffRemoval(const FBuffRemoveEvent& RemoveEvent);
	
private:

	//All buffs this actor has applied to other actors.
	UPROPERTY()
	TArray<UBuff*> OutgoingBuffs;
	//Restrictions on outgoing buffs applied by this actor.
	TConditionalRestrictionList<FBuffRestriction> OutgoingBuffRestrictions;

#pragma endregion 
};
