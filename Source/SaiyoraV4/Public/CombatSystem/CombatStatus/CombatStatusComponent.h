#pragma once
#include "CoreMinimal.h"
#include "CombatStatusStructs.h"
#include "CombatStructs.h"
#include "WidgetComponent.h"
#include "Camera/CameraComponent.h"
#include "CombatStatusComponent.generated.h"

class UFloatingName;
class ASaiyoraGameState;
class ASaiyoraPlayerCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UCombatStatusComponent : public UWidgetComponent
{
	GENERATED_BODY()

//Setup

public:
	
	UCombatStatusComponent();
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	UFUNCTION()
	void OnPlayerAdded(const ASaiyoraPlayerCharacter* NewPlayer);
	
	//Name
	
public:
	
	UFUNCTION(BlueprintPure, Category = "Name")
	FName GetCombatName() const { return CombatName; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Name")
	void SetCombatName(const FName NewName);
	UPROPERTY(BlueprintAssignable, Category = "Name")
	FOnCombatNameChanged OnNameChanged;
	
private:
	
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_CombatName, Category = "Name")
	FName CombatName;
	UPROPERTY(EditDefaultsOnly, Category = "Name")
	TSubclassOf<UFloatingName> NameWidgetClass;
	UFUNCTION()
	void OnRep_CombatName(const FName PreviousName) const { OnNameChanged.Broadcast(PreviousName, CombatName); }
	void SetupNameWidget(const ASaiyoraPlayerCharacter* LocalPlayer);

	UPROPERTY()
	UCameraComponent* LocalPlayerCamera;

//Plane

public:

	UFUNCTION(BlueprintPure, Category = "Plane")
	ESaiyoraPlane GetCurrentPlane() const { return PlaneStatus.CurrentPlane; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Plane")
	ESaiyoraPlane PlaneSwap(const bool bIgnoreRestrictions, UObject* Source, const bool bToSpecificPlane, const ESaiyoraPlane TargetPlane = ESaiyoraPlane::None);

	UFUNCTION(BlueprintCallable, Category = "Plane")
	void AddPlaneSwapRestriction(UObject* Source);
	UFUNCTION(BlueprintCallable, Category = "Plane")
	void RemovePlaneSwapRestriction(UObject* Source);
	UFUNCTION(BlueprintPure, Category = "Plane")
	bool IsPlaneSwapRestricted() { return bPlaneSwapRestricted; }

	UPROPERTY(BlueprintAssignable)
	FPlaneSwapNotification OnPlaneSwapped;
	UPROPERTY(BlueprintAssignable)
	FPlaneSwapRestrictionNotification OnPlaneSwapRestrictionChanged;
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Plane")
	ESaiyoraPlane DefaultPlane = ESaiyoraPlane::None;
	UPROPERTY(EditAnywhere, Category = "Plane")
	bool bCanEverPlaneSwap = false;
	UPROPERTY(ReplicatedUsing = OnRep_PlaneStatus)
	FPlaneStatus PlaneStatus;
	UFUNCTION()
	void OnRep_PlaneStatus(const FPlaneStatus& PreviousStatus);
	
	UPROPERTY()
	TSet<UObject*> PlaneSwapRestrictions;
	UPROPERTY(ReplicatedUsing=OnRep_PlaneSwapRestricted)
	bool bPlaneSwapRestricted = false;
	UFUNCTION()
	void OnRep_PlaneSwapRestricted() { OnPlaneSwapRestrictionChanged.Broadcast(bPlaneSwapRestricted); }

//Faction

public:
	
	UFUNCTION(BlueprintPure, Category = "Faction")
	EFaction GetCurrentFaction() const { return DefaultFaction; }
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Faction")
	EFaction DefaultFaction = EFaction::None;

//Rendering

public:

	void RefreshRendering() { UpdateOwnerCustomRendering(); }

private:
	
	UPROPERTY()
	UCombatStatusComponent* LocalPlayerStatusComponent;
	
	static constexpr int32 DefaultEnemySamePlane = 0;
	static constexpr int32 EnemySamePlaneStart = 1;
	static constexpr int32 EnemySamePlaneEnd = 49;
	static constexpr int32 DefaultEnemyXPlane = 50;
	static constexpr int32 EnemyXPlaneStart = 51;
	static constexpr int32 EnemyXPlaneEnd = 99;
	static constexpr int32 DefaultNeutralSamePlane = 100;
	static constexpr int32 NeutralSamePlaneStart = 101;
	static constexpr int32 NeutralSamePlaneEnd = 149;
	static constexpr int32 DefaultNeutralXPlane = 150;
	static constexpr int32 NeutralXPlaneStart = 151;
	static constexpr int32 NeutralXPlaneEnd = 199;
	static constexpr int32 DefaultStencil = 200;
	static constexpr int32 DefaultFriendlySamePlane = 201;
	static constexpr int32 FriendlySamePlaneStart = 202;
	static constexpr int32 FriendlySamePlaneEnd = 227;
	static constexpr int32 DefaultFriendlyXPlane = 228;
	static constexpr int32 FriendlyXPlaneStart = 229;
	static constexpr int32 FriendlyXPlaneEnd = 255;

	void UpdateOwnerCustomRendering();
	void UpdateStencilValue();
	static TMap<int32, UCombatStatusComponent*> StencilValues;
	int32 StencilValue = 200;
	bool bUseCustomDepth = false;
	bool bUsingDefaultID = true;
	
	UFUNCTION()
	void OnLocalPlayerPlaneSwap(const ESaiyoraPlane Previous, const ESaiyoraPlane New, UObject* Source) { UpdateOwnerCustomRendering(); }

//Collision

private:

	void UpdateOwnerPlaneCollision();
};