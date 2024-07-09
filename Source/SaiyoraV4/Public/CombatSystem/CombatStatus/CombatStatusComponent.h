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
class UThreatHandler;

//Combat component that handles Plane and Faction of its owner. Derives from UWidgetComponent because it also handles the floating name widget above an NPC/player's head outside of combat.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SAIYORAV4_API UCombatStatusComponent : public UWidgetComponent
{
	GENERATED_BODY()

#pragma region Setup

public:
	
	UCombatStatusComponent();
	virtual void GetLifetimeReplicatedProps(::TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UPROPERTY()
	ASaiyoraGameState* GameStateRef;
	//Called when a new player is added by the GameState. This is only called until a local player is found, after which it is unbound.
	//This is used because this component has a name widget that faces the local player's camera, and rendering of the outline material and plane material depend on the local player's faction and plane status.
	UFUNCTION()
	void OnPlayerAdded(const ASaiyoraPlayerCharacter* NewPlayer);

#pragma endregion
#pragma region Names
	
public:

	//Get the display name for this combatant.
	UFUNCTION(BlueprintPure, Category = "Name")
	FName GetCombatName() const { return CombatName; }
	//Change the name of this combatant at runtime.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Name")
	void SetCombatName(const FName NewName);
	//Delegate fired when a combatant's name changes.
	UPROPERTY(BlueprintAssignable, Category = "Name")
	FOnCombatNameChanged OnNameChanged;
	
private:

	//Cosmetic name used for floating name display and unit frames.
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_CombatName, Category = "Name")
	FName CombatName;
	//Widget class for the floating name displayed above the combatant when not in combat.
	UPROPERTY(EditDefaultsOnly, Category = "Name")
	TSubclassOf<UFloatingName> NameWidgetClass;
	UFUNCTION()
	void OnRep_CombatName(const FName PreviousName) const { OnNameChanged.Broadcast(PreviousName, CombatName); }
	//Called when we have a valid reference to a local player, to spawn the name widget and set it up to face the player's camera.
	//This can happen on BeginPlay or in a callback from the GameState adding a new player.
	void SetupNameWidget(const ASaiyoraPlayerCharacter* LocalPlayer);

	//Local player's camera, so the name widget can be faced toward it.
	UPROPERTY()
	UCameraComponent* LocalPlayerCamera = nullptr;
	//Threat handler of the owner, used to bind and unbind to combat change events to hide and unhide the name widget.
	UPROPERTY()
	UThreatHandler* ThreatHandlerRef = nullptr;
	//Callback from the ThreatHandler, hides the name widget when entering combat and shows it again when leaving.
	UFUNCTION()
	void OnCombatChanged(UThreatHandler* Combatant, const bool bNewCombat);

#pragma endregion 
#pragma region Plane

public:

	UFUNCTION(BlueprintPure, Category = "Plane")
	ESaiyoraPlane GetCurrentPlane() const { return PlaneStatus.CurrentPlane; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Plane")
	ESaiyoraPlane PlaneSwap(const bool bIgnoreRestrictions, UObject* Source, const bool bToSpecificPlane, const ESaiyoraPlane TargetPlane = ESaiyoraPlane::Both);

	UFUNCTION(BlueprintCallable, Category = "Plane")
	void AddPlaneSwapRestriction(UObject* Source);
	UFUNCTION(BlueprintCallable, Category = "Plane")
	void RemovePlaneSwapRestriction(UObject* Source);
	UFUNCTION(BlueprintPure, Category = "Plane")
	bool IsPlaneSwapRestricted() const { return bPlaneSwapRestricted; }

	UPROPERTY(BlueprintAssignable)
	FPlaneSwapNotification OnPlaneSwapped;
	//Delegate fired when plane swapping becomes restricted or unrestricted. Specifically useful for the UI to display to players when they can and can't swap planes.
	UPROPERTY(BlueprintAssignable)
	FPlaneSwapRestrictionNotification OnPlaneSwapRestrictionChanged;
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Plane")
	ESaiyoraPlane DefaultPlane = ESaiyoraPlane::Both;
	UPROPERTY(EditAnywhere, Category = "Plane")
	bool bCanEverPlaneSwap = false;
	//Contains the current plane and the source of the last swap.
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
	//Updates the owner's collision profile to reflect his new plane status. This allows him to pass through x-plane geometry and respond correctly to traces and projectiles that are plane-specific.
	void UpdateOwnerPlaneCollision();

#pragma endregion
#pragma region Faction

public:
	
	UFUNCTION(BlueprintPure, Category = "Faction")
	EFaction GetCurrentFaction() const { return DefaultFaction; }
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Faction")
	EFaction DefaultFaction = EFaction::Neutral;

#pragma endregion 
#pragma region Rendering

public:

	//Called when attaching an actor to this component's owner.
	//This just force refreshes rendering so the newly attached actor gets the correct outline and plane materials.
	void RefreshRendering() { UpdateOwnerCustomRendering(); }

private:

	//Plane status component of the local player, used to determine plane relationship to this component's owner.
	UPROPERTY()
	UCombatStatusComponent* LocalPlayerStatusComponent;

	//These are stencil value ranges that are used in the outline and plane materials to determine the correct colors to use.
	
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

	//Updates the outline and plane materials of the owner when it or the local player changes plane relationship.
	void UpdateOwnerCustomRendering();
	//Called in UpdateOwnerCustomRendering. Handles getting a unique stencil value from the correct range, and freeing up the old value this component was using.
	void UpdateStencilValue();
	//Static map of all stencil values in use, to make sure that outlines show up against other actors of the same plane/faction relationship.
	static TMap<int32, UCombatStatusComponent*> StencilValues;
	int32 StencilValue = 200;
	bool bUseCustomDepth = false;
	bool bUsingDefaultID = true;

	//Called when the local player plane swaps, to update the owner's outline and plane materials.
	UFUNCTION()
	void OnLocalPlayerPlaneSwap(const ESaiyoraPlane Previous, const ESaiyoraPlane New, UObject* Source) { UpdateOwnerCustomRendering(); }

#pragma endregion 
};