#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "CombatEnums.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "CombatStructs.generated.h"

class UBuff;

#pragma region Definitions

struct SAIYORAV4_API FSaiyoraCollision
{
    //Object and Trace Channels
    static constexpr ECollisionChannel O_WorldAncient = ECC_GameTraceChannel12; 
    static constexpr ECollisionChannel O_WorldModern = ECC_GameTraceChannel13;
    static constexpr ECollisionChannel O_PlayerHitbox = ECC_GameTraceChannel10; 
    static constexpr ECollisionChannel O_NPCHitbox = ECC_GameTraceChannel11;
    static constexpr ECollisionChannel O_ProjectileHitbox = ECC_GameTraceChannel9;
    static constexpr ECollisionChannel O_ProjectileCollision = ECC_GameTraceChannel2;
    static constexpr ECollisionChannel T_Combat = ECC_GameTraceChannel1;
    static constexpr ECollisionChannel O_NPCDetection = ECC_GameTraceChannel3;
    static constexpr ECollisionChannel O_PlayerDetection = ECC_GameTraceChannel4;

    //Object Profiles
    static const FName P_NoCollision;
    static const FName P_OverlapAll;
    static const FName P_BlockAll;
    static const FName P_Pawn;
    static const FName P_AncientPawn;
    static const FName P_ModernPawn;
    static const FName P_PlayerHitbox;
    static const FName P_NPCHitbox;
    static const FName P_ProjectileHitboxAll;
    static const FName P_ProjectileHitboxPlayers;
    static const FName P_ProjectileHitboxNPCs;
    static const FName P_ProjectileCollisionAll;
    static const FName P_ProjectileCollisionAncient;
    static const FName P_ProjectileCollisionModern;
    static const FName P_NPCAggro;
    static const FName P_PlayerAggro;

    //Combat Trace Profiles
    static const FName CT_All;
    static const FName CT_Players;
    static const FName CT_NPCs;
    static const FName CT_OverlapAll;
    static const FName CT_OverlapPlayers;
    static const FName CT_OverlapNPCs;
    static const FName CT_AncientAll;
    static const FName CT_AncientPlayers;
    static const FName CT_AncientNPCs;
    static const FName CT_AncientOverlapAll;
    static const FName CT_AncientOverlapPlayers;
    static const FName CT_AncientOverlapNPCs;
    static const FName CT_ModernAll;
    static const FName CT_ModernPlayers;
    static const FName CT_ModernNPCs;
    static const FName CT_ModernOverlapAll;
    static const FName CT_ModernOverlapPlayers;
    static const FName CT_ModernOverlapNPCs;
    static const FName CT_GeometryBothPlanes;
    static const FName CT_GeometryAncient;
    static const FName CT_GeometryModern;
    static const FName CT_GeometryNone;
};

struct SAIYORAV4_API FSaiyoraCombatTags : public FGameplayTagNativeAdder
{
    FGameplayTag DungeonBoss;
    
    FGameplayTag AbilityRestriction;
    FGameplayTag AbilityClassRestriction;
    FGameplayTag AbilityFireRateRestriction;
    FGameplayTag AbilityDoubleReloadRestriction;
    FGameplayTag AbilityFullReloadRestriction;

    FGameplayTag FireWeaponAbility;
    FGameplayTag ReloadAbility;
    FGameplayTag StopFireAbility;

    FGameplayTag CrowdControl;
    FGameplayTag Cc_Stun;
    FGameplayTag Cc_Incapacitate;
    FGameplayTag Cc_Root;
    FGameplayTag Cc_Silence;
    FGameplayTag Cc_Disarm;

    FGameplayTag Damage;
    FGameplayTag Healing;

    FGameplayTag ExternalMovement;
    FGameplayTag MovementRestriction;

    FGameplayTag Stat;
    FGameplayTag Stat_DamageDone;
    FGameplayTag Stat_DamageTaken;
    FGameplayTag Stat_HealingDone;
    FGameplayTag Stat_HealingTaken;
    FGameplayTag Stat_AbsorbDone;
    FGameplayTag Stat_AbsorbTaken;
    FGameplayTag Stat_MaxHealth;

    FGameplayTag Stat_GlobalCooldownLength;
    FGameplayTag Stat_CastLength;
    FGameplayTag Stat_CooldownLength;

    FGameplayTag Stat_MaxWalkSpeed;
    FGameplayTag Stat_MaxCrouchSpeed;
    FGameplayTag Stat_GroundFriction; 
    FGameplayTag Stat_BrakingDeceleration; 
    FGameplayTag Stat_MaxAcceleration;
    FGameplayTag Stat_GravityScale;
    FGameplayTag Stat_JumpZVelocity;
    FGameplayTag Stat_AirControl;

    FGameplayTag Stat_AggroRadius;

    FGameplayTag Threat;
    FGameplayTag Threat_Fixate;
    FGameplayTag Threat_Blind;
    FGameplayTag Threat_Fade;
    FGameplayTag Threat_Misdirect;

    FGameplayTag Resurrection;

    FORCEINLINE static const FSaiyoraCombatTags& Get() { return SaiyoraCombatTags; }

    virtual ~FSaiyoraCombatTags() {} /* This stops the "polymorphic struct with non-virtual destructor" intellisense error, even though it shouldn't matter in this case. */

protected:

    virtual void AddTags() override
    {
        UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

        DungeonBoss = Manager.AddNativeGameplayTag(TEXT("Boss"));
        
        AbilityRestriction = Manager.AddNativeGameplayTag(TEXT("Ability.Restriction"));
        AbilityClassRestriction = Manager.AddNativeGameplayTag(TEXT("Ability.Restriction.Class"));
        AbilityFireRateRestriction = Manager.AddNativeGameplayTag(TEXT("Ability.Restriction.FireRate"));
        AbilityDoubleReloadRestriction = Manager.AddNativeGameplayTag(TEXT("Ability.Restriction.DoubleReload"));
        AbilityFullReloadRestriction = Manager.AddNativeGameplayTag(TEXT("Ability.Restriction.FullReload"));

        FireWeaponAbility = Manager.AddNativeGameplayTag(TEXT("Ability.FireWeapon"));
        ReloadAbility = Manager.AddNativeGameplayTag(TEXT("Ability.ReloadWeapon"));
        StopFireAbility = Manager.AddNativeGameplayTag(TEXT("Ability.StopFireWeapon"));

        CrowdControl = Manager.AddNativeGameplayTag(TEXT("CrowdControl"));
        Cc_Stun = Manager.AddNativeGameplayTag(TEXT("CrowdControl.Stun"));
        Cc_Incapacitate = Manager.AddNativeGameplayTag(TEXT("CrowdControl.Incapacitate"));
        Cc_Root = Manager.AddNativeGameplayTag(TEXT("CrowdControl.Root"));
        Cc_Silence = Manager.AddNativeGameplayTag(TEXT("CrowdControl.Silence"));
        Cc_Disarm = Manager.AddNativeGameplayTag(TEXT("CrowdControl.Disarm"));

        Damage = Manager.AddNativeGameplayTag(TEXT("Damage"));
        Healing = Manager.AddNativeGameplayTag(TEXT("Healing"));

        ExternalMovement = Manager.AddNativeGameplayTag(TEXT("Movement"));
        MovementRestriction = Manager.AddNativeGameplayTag(TEXT("Movement.Restriction"));

        Stat = Manager.AddNativeGameplayTag(TEXT("Stat"));
        Stat_DamageDone = Manager.AddNativeGameplayTag(TEXT("Stat.DamageDone"));
        Stat_DamageTaken = Manager.AddNativeGameplayTag(TEXT("Stat.DamageTaken"));
        Stat_HealingDone = Manager.AddNativeGameplayTag(TEXT("Stat.HealingDone"));
        Stat_HealingTaken = Manager.AddNativeGameplayTag(TEXT("Stat.HealingTaken"));
        Stat_MaxHealth = Manager.AddNativeGameplayTag(TEXT("Stat.MaxHealth"));
        Stat_AggroRadius = Manager.AddNativeGameplayTag(TEXT("Stat.AggroRadius"));
        Stat_GlobalCooldownLength = Manager.AddNativeGameplayTag(TEXT("Stat.GlobalCooldownLength"));
        Stat_CastLength = Manager.AddNativeGameplayTag(TEXT("Stat.CastLength"));
        Stat_CooldownLength = Manager.AddNativeGameplayTag(TEXT("Stat.CooldownLength"));
        Stat_MaxWalkSpeed = Manager.AddNativeGameplayTag(TEXT("Stat.MaxWalkSpeed"));
        Stat_MaxCrouchSpeed = Manager.AddNativeGameplayTag(TEXT("Stat.MaxCrouchSpeed"));
        Stat_GroundFriction = Manager.AddNativeGameplayTag(TEXT("Stat.GroundFriction"));
        Stat_BrakingDeceleration = Manager.AddNativeGameplayTag(TEXT("Stat.BrakingDeceleration"));
        Stat_MaxAcceleration = Manager.AddNativeGameplayTag(TEXT("Stat.MaxAcceleration"));
        Stat_GravityScale = Manager.AddNativeGameplayTag(TEXT("Stat.GravityScale"));
        Stat_JumpZVelocity = Manager.AddNativeGameplayTag(TEXT("Stat.JumpZVelocity"));
        Stat_AirControl = Manager.AddNativeGameplayTag(TEXT("Stat.AirControl"));

        Threat = Manager.AddNativeGameplayTag(TEXT("Threat"));
        Threat_Fixate = Manager.AddNativeGameplayTag(TEXT("Threat.Fixate"));
        Threat_Blind = Manager.AddNativeGameplayTag(TEXT("Threat.Blind"));
        Threat_Fade = Manager.AddNativeGameplayTag(TEXT("Threat.Fade"));
        Threat_Misdirect = Manager.AddNativeGameplayTag(TEXT("Threat.Misdirect"));

        Resurrection = Manager.AddNativeGameplayTag(TEXT("Buff.Resurrection"));
    }

private:

    static FSaiyoraCombatTags SaiyoraCombatTags;
};

#pragma endregion 
#pragma region CombatParameters

/*
 *Combat parameters are extremely generic structs that are intended to be used as FInstancedStructs.
 *These can contain basically any data that might want to be accessed by an ability or buff, identified by string.
 */

//Base parameter struct, all the parameter types inherit from this.
USTRUCT(BlueprintType)
struct FCombatParameter
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ParamName;

    FCombatParameter() {}
    FCombatParameter(const FString& Name) : ParamName(Name) {}
};

USTRUCT(BlueprintType)
struct FCombatParameterBool : public FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool BoolParam = false;

    FCombatParameterBool() {}
    FCombatParameterBool(const FString& Name, const bool Value) : FCombatParameter(Name), BoolParam(Value) {}
};

USTRUCT(BlueprintType)
struct FCombatParameterInt : public FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 IntParam = 0;

    FCombatParameterInt() {}
    FCombatParameterInt(const FString& Name, const int32 Value) : FCombatParameter(Name), IntParam(Value) {}
};

USTRUCT(BlueprintType)
struct FCombatParameterFloat : public FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float FloatParam = 0.0f;

    FCombatParameterFloat() {}
    FCombatParameterFloat(const FString& Name, const float Value) : FCombatParameter(Name), FloatParam(Value) {}
};

USTRUCT(BlueprintType)
struct FCombatParameterString : public FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString StringParam;

    FCombatParameterString() {}
    FCombatParameterString(const FString& Name, const FString& Value) : FCombatParameter(Name), StringParam(Value) {}
};

USTRUCT(BlueprintType)
struct FCombatParameterVector : public FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector VectorParam = FVector::ZeroVector;

    FCombatParameterVector() {}
    FCombatParameterVector(const FString& Name, const FVector& Value) : FCombatParameter(Name), VectorParam(Value) {}
};

USTRUCT(BlueprintType)
struct FCombatParameterRotator : public FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FRotator RotatorParam = FRotator::ZeroRotator;

    FCombatParameterRotator() {}
    FCombatParameterRotator(const FString& Name, const FRotator& Value) : FCombatParameter(Name), RotatorParam(Value) {}
};

USTRUCT(BlueprintType)
struct FCombatParameterObject : public FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UObject* ObjectParam = nullptr;

    FCombatParameterObject() {}
    FCombatParameterObject(const FString& Name, UObject* Object) : FCombatParameter(Name), ObjectParam(Object) {}
};

USTRUCT(BlueprintType)
struct FCombatParameterClass : public FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<UObject> ClassParam;

    FCombatParameterClass() {}
    FCombatParameterClass(const FString& Name, const TSubclassOf<UObject> Value) : FCombatParameter(Name), ClassParam(Value) {}
};

#pragma endregion
#pragma region Restrictions

//Templated class used for containing restrictions to a specific type of combat event.
//Makes it easy to add, remove, and check restrictions given the context of an event.
template <class T>
class TConditionalRestrictionList
{
public:

    template <typename... Args>
    bool IsRestricted(Args... Context)
    {
        for (const T& Restriction : Restrictions)
        {
            if (Restriction.IsBound() && Restriction.Execute(Context...))
            {
                return true;
            }
        }
        return false;
    }
    
    void Add(const T& Restriction)
    {
        if (Restriction.IsBound())
        {
            Restrictions.Add(Restriction);
        }
    }
    
    int32 Remove(const T& Restriction)
    {
        return Restrictions.Remove(Restriction);
    }

private:
    
    TSet<T> Restrictions;
};

#pragma endregion
#pragma region Modifiers

//Struct that defines a modifier to a value in the context of a combat event or persistent value (like stats or resource costs).
USTRUCT(BlueprintType)
struct FCombatModifier
{
    GENERATED_BODY()

    //How to calculate this modifier's effect on a value. Invalid means the modifier will not be used in this calculation.
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EModifierType Type = EModifierType::Invalid;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Value = 0.0f;
    //For modifiers that are sourced from buffs, they can optionally stack with the buff's stacks.
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bStackable = true;
    //Optional buff source. Modifiers without a valid buff source can not stack.
    UPROPERTY(BlueprintReadWrite)
    UBuff* BuffSource = nullptr;

    FCombatModifier() {}
    FCombatModifier(const float BaseValue, const EModifierType ModifierType, UBuff* SourceBuff = nullptr, const bool Stackable = false);
    FORCEINLINE bool operator==(const FCombatModifier& Other) const { return Other.Type == Type && Other.BuffSource == BuffSource && Other.Value == Value; }
    
    //Helper function for applying an array of modifiers to a base value.
    static float ApplyModifiers(const TArray<FCombatModifier>& ModArray, const float BaseValue);
    //Helper function for applying an array of modifiers to a base value, then truncating it to an int.
    static int32 ApplyModifiers(const TArray<FCombatModifier>& ModArray, const int32 BaseValue);
};

//Templated class used for holding a list of conditional modifier functions that each return an FCombatModifier when provided combat event context.
//Useful for adding, removing, and retrieving the list of modifiers for a given event.
template <class T>
class TConditionalModifierList
{
public:

    template <typename... Args>
    void GetModifiers(TArray<FCombatModifier>& OutModifiers, Args... Context) const
    {
        for (const T& Mod : Modifiers)
        {
            if (Mod.IsBound())
            {
                OutModifiers.Add(Mod.Execute(Context...));
            }
        }
    }

    void Add(const T& Modifier)
    {
        if (Modifier.IsBound())
        {
            Modifiers.Add(Modifier);
        }
    }

    void Remove(const T& Modifier)
    {
        Modifiers.Remove(Modifier);
    }

private:

    TSet<T> Modifiers;
};

//Container for a unique ID for a given combat modifier that can be used to remove or modify it later.
USTRUCT(BlueprintType)
struct FCombatModifierHandle
{
    GENERATED_BODY()

    //Whether this modifier handle is valid.
    bool IsValid() const { return ModifierID > 0; }

    FCombatModifierHandle() {}
    FCombatModifierHandle(const FCombatModifierHandle& Other) : ModifierID(Other.ModifierID) {}
    FORCEINLINE bool operator==(const FCombatModifierHandle& Other) const { return Other.ModifierID == ModifierID; }

    //Static definition of an invalid modifier handle with ID -1. Used when applying a modifier fails.
    static FCombatModifierHandle Invalid;
    static FCombatModifierHandle MakeHandle() { return FCombatModifierHandle(GetNextModifierID()); }

private:

    int32 ModifierID = -1;
    
    static int32 NextModifier;
    static int32 GetNextModifierID() { NextModifier++; return NextModifier; }

    FCombatModifierHandle(const int32 ID) : ModifierID(ID) {}

    friend uint32 GetTypeHash(const FCombatModifierHandle& Handle);
};

FORCEINLINE uint32 GetTypeHash(const FCombatModifierHandle& Handle)
{
    return GetTypeHash(Handle.ModifierID);
}

DECLARE_DELEGATE_TwoParams(FModifiableFloatCallback, const float, const float);
DECLARE_DELEGATE_TwoParams(FModifiableIntCallback, const int32, const int32);

//Wrapper for a float value and the list of modifiers currently applied to it, along with some parameters to determine clamping.
//Comes with a callback delegate to update whatever contains this value when the calculated value changes.
//Derived structs can be used in FastArrays for per-item callbacks.
USTRUCT()
struct FModifiableFloat : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:

    void Init() { Recalculate(); }

    FCombatModifierHandle AddModifier(const FCombatModifier& Modifier);
    void RemoveModifier(const FCombatModifierHandle& ModifierHandle);
    void UpdateModifier(const FCombatModifierHandle& ModifierHandle, const FCombatModifier& NewModifier);
    void SetUpdatedCallback(const FModifiableFloatCallback& Callback);

    float GetCurrentValue() const { return CurrentValue; }
    float GetDefaultValue() const { return DefaultValue; }
    bool IsModifiable() const { return bIsModifiable; }

    void SetDefaultValue(const float NewDefault) { DefaultValue = NewDefault; Recalculate(); }
    void SetIsModifiable(const bool bNewModifiable) { bIsModifiable = bNewModifiable; Recalculate(); }
    void SetMinClamp(const bool bClamp, const float NewClamp) { bClampMin = bClamp; Minimum = bClampMin ? NewClamp : 0.0f; Recalculate(); }
    void SetMaxClamp(const bool bClamp, const float NewClamp) { bClampMax = bClamp; Maximum = bClampMax ? NewClamp : 0.0f; Recalculate(); }

    FModifiableFloat() {}
    FModifiableFloat(const float Default, const bool bModifiable, const bool bClampLow = false, const float LowClamp = 0.0f, const bool bClampHigh = false, const float HighClamp = 0.0f)
        : DefaultValue(Default), bIsModifiable(bModifiable), bClampMin(bClampLow), Minimum(LowClamp), bClampMax(bClampHigh), Maximum(HighClamp), CurrentValue(Default) {}
    
private:

    UPROPERTY(EditAnywhere, NotReplicated)
    float DefaultValue = 0.0f;
    UPROPERTY(EditAnywhere, NotReplicated)
    bool bIsModifiable = true;

    UPROPERTY(EditAnywhere, NotReplicated)
    bool bClampMin = false;
    UPROPERTY(EditAnywhere, NotReplicated, meta = (EditCondition = "bClampMin"))
    float Minimum = 0;

    UPROPERTY(EditAnywhere, NotReplicated)
    bool bClampMax = false;
    UPROPERTY(EditAnywhere, NotReplicated, meta = (EditCondition = "bClampMax"))
    float Maximum = 0;

    void Recalculate();
    UPROPERTY()
    float CurrentValue = 0.0f;
    FModifiableFloatCallback OnUpdated;
    
    TMap<FCombatModifierHandle, FCombatModifier> Modifiers;
};

//Wrapper for an int value and the list of modifiers currently applied to it, along with some parameters to determine clamping.
//Comes with a callback delegate to update whatever contains this value when the calculated value changes.
//Derived structs can be used in FastArrays for per-item callbacks.
USTRUCT()
struct FModifiableInt : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:

    void Init() { Recalculate(); }

    FCombatModifierHandle AddModifier(const FCombatModifier& Modifier);
    void RemoveModifier(const FCombatModifierHandle& ModifierHandle);
    void UpdateModifier(const FCombatModifierHandle& ModifierHandle, const FCombatModifier& NewModifier);
    void SetUpdatedCallback(const FModifiableIntCallback& Callback);

    int32 GetCurrentValue() const { return CurrentValue; }
    int32 GetDefaultValue() const { return DefaultValue; }
    bool IsModifiable() const { return bIsModifiable; }

    void SetDefaultValue(const int32 NewDefault) { DefaultValue = NewDefault; Recalculate(); }
    void SetIsModifiable(const bool bNewModifiable) { bIsModifiable = bNewModifiable; Recalculate(); }
    void SetMinClamp(const bool bClamp, const int32 NewClamp) { bClampMin = bClamp; Minimum = bClampMin ? NewClamp : 0; Recalculate(); }
    void SetMaxClamp(const bool bClamp, const int32 NewClamp) { bClampMax = bClamp; Maximum = bClampMax ? NewClamp : 0; Recalculate(); }

    FModifiableInt() {}
    FModifiableInt(const int32 Default, const bool bModifiable, const bool bClampLow = false, const int32 LowClamp = 0, const bool bClampHigh = false, const int32 HighClamp = 0)
        : DefaultValue(Default), bIsModifiable(bModifiable), bClampMin(bClampLow), Minimum(LowClamp), bClampMax(bClampHigh), Maximum(HighClamp), CurrentValue(Default) {}
    
private:

    UPROPERTY(EditAnywhere, NotReplicated)
    int32 DefaultValue = 0;
    UPROPERTY(EditAnywhere, NotReplicated)
    bool bIsModifiable = true;

    UPROPERTY(EditAnywhere, NotReplicated)
    bool bClampMin = false;
    UPROPERTY(EditAnywhere, NotReplicated, meta = (EditCondition = "bClampMin"))
    int32 Minimum = 0;

    UPROPERTY(EditAnywhere, NotReplicated)
    bool bClampMax = false;
    UPROPERTY(EditAnywhere, NotReplicated, meta = (EditCondition = "bClampMax"))
    int32 Maximum = 0;

    void Recalculate();
    UPROPERTY()
    int32 CurrentValue = 0;
    FModifiableIntCallback OnUpdated;
    
    TMap<FCombatModifierHandle, FCombatModifier> Modifiers;
};

#pragma endregion 