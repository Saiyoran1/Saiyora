<a name="top"></a>
**[Back to Root](/README.md)**

# Crowd Control

Crowd control describes a subset of status effects from buffs that remove capabilities from an actor. In Saiyora, there are six different types of crowd control.

- Stuns, which prevent essentially all actions, including movement and ability usage.
- Roots, which prevent movement and immediately stop an actor's momentum, but do not prevent turning or crouching.
- Incapacitates, which are similar to stuns but also remove damage-over-time debuffs from the target and can be broken by taking any damage.
- Silences, which prevent ability usage.
- Disarms, which prevent weapon usage.

Notably, though certain crowd controls are intended to generically prevent ability usage and others are not, in reality abilities themselves have flags that determine what crowd controls prevent them from being used. 

> For example, a "blink" ability that dashes a character forward would be marked to be unusable while rooted, even though roots aren't normally a crowd control that prevents ability usage. Similarly, abilities can be individually marked to be usable while stunned or silenced, despite these crowd controls normally being intended to prevent ability usage.

Crowd control is handled by the UCrowdControlHandler component, which monitors buffs applied to its owning actor's UBuffHandler, checking their tags for any gameplay tag starting with "CrowdControl." It maintains a structure representing the status of each of the six crowd control types, including whether they are active, which buffs are applying them, and which of the buffs applying the crowd control has the longest remaining duration (mostly for use in the UI, but also to reduce checking whether removed buffs should cause crowd control to be removed). 

Crowd control application is not a restrictable event. The Crowd Control Handler does have a list of crowd control tags that it is immune to, but it is not modifiable during gameplay. Currently, the Buff Handler actually manually checks during its application of a buff whether the Crowd Control Handler exists and if its list of immuned crowd controls contains any tags that an applied buff has, and will restrict buff application entirely. 

> To add an immunity to a certain crowd control during gameplay, you would just add an incoming buff restriction that references a function like this:
```
bool RestrictStunBuffs(const FBuffApplyEvent& BuffEvent)
{
    UBuff* DefaultBuff = BuffEvent.BuffClass.GetDefaultObject();
    if (IsValid(DefaultBuff))
    {
        FGameplayTagContainer BuffTags;
        DefaultBuff->GetBuffTags(BuffTags);
        return BuffTags.HasTag(FSaiyoraCombatTags::Get().Cc_Stun));
    }
    return false;
}
```
**[⬆ Back to Top](#top)**
