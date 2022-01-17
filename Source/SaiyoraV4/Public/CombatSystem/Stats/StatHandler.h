#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatStructs.h"
#include "BuffStructs.h"
#include "StatHandler.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UStatHandler : public UActorComponent
{
	GENERATED_BODY()
	
public:
	
	static FGameplayTag GenericStatTag() { return FGameplayTag::RequestGameplayTag(FName(TEXT("Stat")), false); }
	
	UStatHandler();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;

//Stats

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool IsStatValid(FGameplayTag const StatTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	float GetStatValue(FGameplayTag const StatTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	bool IsStatModifiable(FGameplayTag const StatTag) const;
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void SubscribeToStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback);
	UFUNCTION(BlueprintCallable, Category = "Stat", meta = (GameplayTagFilter = "Stat"))
	void UnsubscribeFromStatChanged(FGameplayTag const StatTag, FStatCallback const& Callback);
	void AddStatModifier(FGameplayTag const StatTag, FCombatModifier const& Modifier);
	void RemoveStatModifier(FGameplayTag const StatTag, UBuff* Source);	
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Stat")
	UDataTable* InitialStats;
	TMap<FGameplayTag, FStatInfo> StatDefaults;
	UPROPERTY(Replicated)
	FCombatStatArray ReplicatedStats;
	TMap<FGameplayTag, FCombatStat> NonReplicatedStats;
	FGameplayTagContainer StaticStats;
};
