#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "SaiyoraEnums.h"
#include "SaiyoraStructs.generated.h"

struct SAIYORAV4_API FSaiyoraCombatTags : public FGameplayTagNativeAdder
{
    FGameplayTag DungeonBoss;
    
    FGameplayTag AbilityRestriction;
    FGameplayTag AbilityClassRestriction;

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

    FGameplayTag Threat;
    FGameplayTag Threat_Fixate;
    FGameplayTag Threat_Blind;
    FGameplayTag Threat_Fade;
    FGameplayTag Threat_Misdirect;

    FORCEINLINE static const FSaiyoraCombatTags& Get() { return SaiyoraCombatTags; }

protected:

    virtual void AddTags() override
    {
        UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

        DungeonBoss = Manager.AddNativeGameplayTag(TEXT("Boss"));
        
        AbilityRestriction = Manager.AddNativeGameplayTag(TEXT("Ability.Restriction"));
        AbilityClassRestriction = Manager.AddNativeGameplayTag(TEXT("Ability.Restriction.Class"));

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
    }

private:

    static FSaiyoraCombatTags SaiyoraCombatTags;
};

USTRUCT(BlueprintType)
struct FCombatModifier
{
    GENERATED_BODY()

    static float ApplyModifiers(TArray<FCombatModifier> const& ModArray, float const BaseValue);
    static int32 ApplyModifiers(TArray<FCombatModifier> const& ModArray, int32 const BaseValue);
    FCombatModifier() {}
    FCombatModifier(float const BaseValue, EModifierType const ModifierType, class UBuff* SourceBuff = nullptr, bool const Stackable = false);
    UPROPERTY()
    EModifierType Type = EModifierType::Invalid;
    UPROPERTY()
    float Value = 0.0f;
    UPROPERTY()
    bool bStackable = true;
    UPROPERTY(NotReplicated)
    UBuff* Source = nullptr;

    FORCEINLINE bool operator==(FCombatModifier const& Other) const { return Other.Type == Type && Other.Source == Source && Other.Value == Value; }
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