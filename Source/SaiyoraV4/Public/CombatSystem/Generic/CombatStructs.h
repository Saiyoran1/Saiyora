#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "CombatEnums.h"
#include "CombatStructs.generated.h"

class UBuff;

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

    FGameplayTag Behavior_Combat;

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

        Behavior_Combat = Manager.AddNativeGameplayTag(TEXT("Behavior.CombatTree"));
    }

private:

    static FSaiyoraCombatTags SaiyoraCombatTags;
};

USTRUCT(BlueprintType)
struct FCombatModifier
{
    GENERATED_BODY()

    static float ApplyModifiers(const TArray<FCombatModifier>& ModArray, const float BaseValue);
    static int32 ApplyModifiers(const TArray<FCombatModifier>& ModArray, const int32 BaseValue);
    FCombatModifier() {}
    FCombatModifier(const float BaseValue, const EModifierType ModifierType, UBuff* SourceBuff = nullptr, const bool Stackable = false);
    UPROPERTY(BlueprintReadWrite)
    EModifierType Type = EModifierType::Invalid;
    UPROPERTY(BlueprintReadWrite)
    float Value = 0.0f;
    UPROPERTY(BlueprintReadWrite)
    bool bStackable = true;
    UPROPERTY(BlueprintReadWrite)
    UBuff* BuffSource = nullptr;

    FORCEINLINE bool operator==(const FCombatModifier& Other) const { return Other.Type == Type && Other.BuffSource == BuffSource && Other.Value == Value; }
};

USTRUCT(BlueprintType)
struct FCombatParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    ECombatParamType ParamType = ECombatParamType::None;
    UPROPERTY(BlueprintReadWrite)
    FString ParamName;
    UPROPERTY(BlueprintReadWrite)
    bool BoolParam = false;
    UPROPERTY(BlueprintReadWrite)
    int32 IntParam = 0;
    UPROPERTY(BlueprintReadWrite)
    float FloatParam = 0.0f;
    UPROPERTY(BlueprintReadWrite)
    UObject* ObjectParam = nullptr;
    UPROPERTY(BlueprintReadWrite)
    TSubclassOf<UObject> ClassParam;
    UPROPERTY(BlueprintReadWrite)
    FVector VectorParam = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite)
    FRotator RotatorParam = FRotator::ZeroRotator;

    friend FArchive& operator<<(FArchive& Ar, FCombatParameter& Parameter)
    {
        Ar << Parameter.ParamType;
        Ar << Parameter.ParamName;
        switch (Parameter.ParamType)
        {
            case ECombatParamType::None :
                break;
            case ECombatParamType::String :
                break;
            case ECombatParamType::Bool :
                Ar << Parameter.BoolParam;
                break;
            case ECombatParamType::Int :
                Ar << Parameter.IntParam;
                break;
            case ECombatParamType::Float :
                Ar << Parameter.FloatParam;
                break;
            case ECombatParamType::Object :
                Ar << Parameter.ObjectParam;
                break;
            case ECombatParamType::Class :
                Ar << Parameter.ClassParam;
                break;
            case ECombatParamType::Vector :
                Ar << Parameter.VectorParam;
                break;
            case ECombatParamType::Rotator :
                Ar << Parameter.RotatorParam;
                break;
            default :
                break;
        }
        return Ar;
    }
};

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

USTRUCT(BlueprintType)
struct FCombatModifierHandle
{
    GENERATED_BODY()

    FCombatModifierHandle() {}
    FCombatModifierHandle(const FCombatModifierHandle& Other) : ModifierID(Other.ModifierID) {}
    static FCombatModifierHandle Invalid;
    static FCombatModifierHandle MakeHandle() { return FCombatModifierHandle(GetNextModifierID()); }
    
    bool IsValid() const { return ModifierID > 0; }

    FORCEINLINE bool operator==(const FCombatModifierHandle& Other) const { return Other.ModifierID == ModifierID; }

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

USTRUCT()
struct FModifiableFloat
{
    GENERATED_BODY()

public:

    FCombatModifierHandle AddModifier(const FCombatModifier& Modifier);
    void RemoveModifier(const FCombatModifierHandle& ModifierHandle);
    void UpdateModifier(const FCombatModifierHandle& ModifierHandle, const FCombatModifier& NewModifier);
    void SetUpdatedCallback(const FModifiableFloatCallback& Callback);

    float GetCurrentValue() const { return bInitialized ? CurrentValue : DefaultValue; }
    float GetDefaultValue() const { return DefaultValue; }
    bool IsModifiable() const { return bIsModifiable; }

    void SetDefaultValue(const float NewDefault) { DefaultValue = NewDefault; Recalculate(); }
    void SetIsModifiable(const bool bNewModifiable) { bIsModifiable = bNewModifiable; Recalculate(); }
    void SetMinClamp(const bool bClamp, const float NewClamp) { bClampMin = bClamp; MinClamp = bClampMin ? NewClamp : 0.0f; Recalculate(); }
    void SetMaxClamp(const bool bClamp, const float NewClamp) { bClampMax = bClamp; MaxClamp = bClampMax ? NewClamp : 0.0f; Recalculate(); }

    FModifiableFloat() { Recalculate(); }
    //This constructor gets called when making the in-game struct from the editor struct I think.
    //This allows a recalculation to happen immediately, instead of needing to manually initialize the struct.
    FModifiableFloat(const FModifiableFloat& Other)
    {
        *this = Other;
        Recalculate();
    }
    FModifiableFloat(const float Default, const bool bModifiable, const bool bClampLow = false, const float LowClamp = 0.0f, const bool bClampHigh = false, const float HighClamp = 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Default value = %f"), Default);
        DefaultValue = Default;
        bIsModifiable = bModifiable;
        bClampMin = bClampLow;
        MinClamp = bClampMin ? LowClamp : 0.0f;
        bClampMax = bClampHigh;
        MaxClamp = bClampMax ? HighClamp : 0.0f;
        Recalculate();
    }
    
private:

    UPROPERTY(EditAnywhere, NotReplicated)
    float DefaultValue = 0.0f;
    UPROPERTY(EditAnywhere, NotReplicated)
    bool bIsModifiable = true;

    UPROPERTY(EditAnywhere, NotReplicated)
    bool bClampMin = false;
    UPROPERTY(EditAnywhere, NotReplicated)
    float MinClamp = 0;

    UPROPERTY(EditAnywhere, NotReplicated)
    bool bClampMax = false;
    UPROPERTY(EditAnywhere, NotReplicated)
    float MaxClamp = 0;

    void Recalculate();
    UPROPERTY()
    float CurrentValue = 0.0f;
    FModifiableFloatCallback OnUpdated;
    
    TMap<FCombatModifierHandle, FCombatModifier> Modifiers;

    bool bInitialized = false;
};

USTRUCT()
struct FModifiableInt
{
    GENERATED_BODY()

public:

    FCombatModifierHandle AddModifier(const FCombatModifier& Modifier);
    void RemoveModifier(const FCombatModifierHandle& ModifierHandle);
    void UpdateModifier(const FCombatModifierHandle& ModifierHandle, const FCombatModifier& NewModifier);
    void SetUpdatedCallback(const FModifiableIntCallback& Callback);

    int32 GetCurrentValue() const { return bInitialized ? CurrentValue : DefaultValue; }
    int32 GetDefaultValue() const { return DefaultValue; }
    bool IsModifiable() const { return bIsModifiable; }

    void SetDefaultValue(const int32 NewDefault) { DefaultValue = NewDefault; Recalculate(); }
    void SetIsModifiable(const bool bNewModifiable) { bIsModifiable = bNewModifiable; Recalculate(); }
    void SetMinClamp(const bool bClamp, const int32 NewClamp) { bClampMin = bClamp; MinClamp = bClampMin ? NewClamp : 0; Recalculate(); }
    void SetMaxClamp(const bool bClamp, const int32 NewClamp) { bClampMax = bClamp; MaxClamp = bClampMax ? NewClamp : 0; Recalculate(); }

    FModifiableInt() { Recalculate(); }
    //This constructor gets called when making the in-game struct from the editor struct I think.
    //This allows a recalculation to happen immediately, instead of needing to manually initialize the struct.
    FModifiableInt(const FModifiableInt& Other)
    {
        *this = Other;
        Recalculate();
    }
    FModifiableInt(const int32 Default, const bool bModifiable, const bool bClampLow = false, const int32 LowClamp = 0, const bool bClampHigh = false, const int32 HighClamp = 0)
    {
        DefaultValue = Default;
        bIsModifiable = bModifiable;
        bClampMin = bClampLow;
        MinClamp = bClampMin ? LowClamp : 0;
        bClampMax = bClampHigh;
        MaxClamp = bClampMax ? HighClamp : 0;
        Recalculate();
    }
    
private:

    UPROPERTY(EditAnywhere)
    int32 DefaultValue = 0;
    UPROPERTY(EditAnywhere)
    bool bIsModifiable = true;

    bool bClampMin = false;
    int32 MinClamp = 0;

    bool bClampMax = false;
    int32 MaxClamp = 0;

    void Recalculate();
    UPROPERTY()
    int32 CurrentValue = 0;
    FModifiableIntCallback OnUpdated;
    
    TMap<FCombatModifierHandle, FCombatModifier> Modifiers;

    bool bInitialized = false;
};