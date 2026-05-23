#include "quirks.h"

#include <stdint.h>
#include <string.h>

#include "combat_defs.h"
#include "config.h"
#include "critter.h"
#include "item.h"
#include "object.h"
#include "party_member.h"
#include "platform_compat.h"
#include "sfall_config.h"
#include "skill_defs.h"
#include "stat.h"
#include "stat_defs.h"

namespace fallout {

// External objects from the engine we read but never mutate.
extern Object* gDude;

namespace {

    uint32_t gActiveQuirks = 0;
    bool gFirstHitPending = false;

    struct QuirkDef {
        const char* configName; // matched case-insensitively from ddraw.ini
        const char* displayName; // shown in logs / future UI
    };

    constexpr QuirkDef kQuirkDefs[QUIRK_COUNT] = {
        { "GlassCannon", "Glass Cannon" },
        { "Headhunter", "Headhunter" },
        { "Sociopath", "Sociopath" },
        { "Berserker", "Berserker" },
        { "Gunslinger", "Gunslinger" },
        { "CultOfTheMushroomCloud", "Cult of the Mushroom Cloud" },
        { "LoneWanderer", "Lone Wanderer" },
        { "QuickLearner", "Quick Learner" },
        { "DeathWish", "Death Wish" },
        { "Sturdy", "Sturdy" },
    };

    void setActive(int quirk)
    {
        if (quirk >= 0 && quirk < QUIRK_COUNT) {
            gActiveQuirks |= (1u << quirk);
        }
    }

    int lookupByName(const char* name)
    {
        for (int i = 0; i < QUIRK_COUNT; i++) {
            if (compat_stricmp(name, kQuirkDefs[i].configName) == 0) {
                return i;
            }
        }
        return QUIRK_NONE;
    }

    // Parse "GlassCannon, Headhunter" — trim whitespace, allow at most 2 active.
    void parseQuirksString(const char* csv)
    {
        if (csv == nullptr || *csv == '\0') {
            return;
        }

        int activeCount = 0;
        char buf[64];
        size_t bufLen = 0;

        auto flush = [&]() {
            if (bufLen == 0) return;
            buf[bufLen] = '\0';

            // Trim leading whitespace.
            char* start = buf;
            while (*start == ' ' || *start == '\t')
                start++;

            // Trim trailing whitespace.
            size_t len = strlen(start);
            while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t')) {
                start[--len] = '\0';
            }

            if (*start != '\0') {
                int q = lookupByName(start);
                if (q != QUIRK_NONE && activeCount < 2) {
                    setActive(q);
                    activeCount++;
                }
            }
            bufLen = 0;
        };

        for (const char* p = csv; *p != '\0'; p++) {
            if (*p == ',') {
                flush();
            } else if (bufLen < sizeof(buf) - 1) {
                buf[bufLen++] = *p;
            }
        }
        flush();
    }

    bool isPistolWeapon(Object* attacker, int hitMode)
    {
        Object* weapon = critterGetWeaponForHitMode(attacker, hitMode);
        if (weapon == nullptr) return false;
        if (weaponGetSkillForHitMode(weapon, hitMode) != SKILL_SMALL_GUNS) return false;
        return weaponIsTwoHanded(weapon) == 0;
    }

    bool isRifleOrBigGunWeapon(Object* attacker, int hitMode)
    {
        Object* weapon = critterGetWeaponForHitMode(attacker, hitMode);
        if (weapon == nullptr) return false;
        int skill = weaponGetSkillForHitMode(weapon, hitMode);
        if (skill == SKILL_BIG_GUNS) return true;
        if (skill == SKILL_SMALL_GUNS && weaponIsTwoHanded(weapon)) return true;
        return false;
    }

    bool hasPartyCompanion()
    {
        // _getPartyMemberCount() includes the dude itself in the count, so a
        // value of 1 means "only the dude". > 1 means at least one companion.
        return _getPartyMemberCount() > 1;
    }

} // namespace

void quirksInit()
{
    gActiveQuirks = 0;
    gFirstHitPending = false;

    char* csv = nullptr;
    if (configGetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_QUIRKS_KEY, &csv)) {
        parseQuirksString(csv);
    }
}

bool dudeHasQuirk(int quirk)
{
    if (quirk < 0 || quirk >= QUIRK_COUNT) return false;
    return (gActiveQuirks & (1u << quirk)) != 0;
}

void quirksResetCombatState()
{
    gFirstHitPending = true;
}

double quirksGetDamageScale(int hitMode)
{
    double scale = 1.0;

    if (dudeHasQuirk(QUIRK_SOCIOPATH)) {
        scale *= 1.15;
    }

    if (dudeHasQuirk(QUIRK_BERSERKER)) {
        // Melee + unarmed only.
        if (hitMode == HIT_MODE_PUNCH || hitMode == HIT_MODE_KICK
            || hitMode == HIT_MODE_STRONG_PUNCH || hitMode == HIT_MODE_HAMMER_PUNCH
            || hitMode == HIT_MODE_HAYMAKER || hitMode == HIT_MODE_JAB
            || hitMode == HIT_MODE_PALM_STRIKE || hitMode == HIT_MODE_PIERCING_STRIKE
            || hitMode == HIT_MODE_STRONG_KICK || hitMode == HIT_MODE_SNAP_KICK
            || hitMode == HIT_MODE_POWER_KICK || hitMode == HIT_MODE_HIP_KICK
            || hitMode == HIT_MODE_HOOK_KICK || hitMode == HIT_MODE_PIERCING_KICK) {
            scale *= 1.20;
        }
    }

    if (dudeHasQuirk(QUIRK_GLASS_CANNON) && gFirstHitPending) {
        scale *= 1.50;
        gFirstHitPending = false;
    }

    return scale;
}

int quirksGetStatBonus(int stat, int currentValueBeforeBonus)
{
    int bonus = 0;

    // Lone Wanderer: +2 to all SPECIAL stats with no companions.
    if (dudeHasQuirk(QUIRK_LONE_WANDERER) && !hasPartyCompanion()) {
        if (stat >= STAT_STRENGTH && stat <= STAT_LUCK) {
            bonus += 2;
        }
    }

    // Glass Cannon: halve max HP.
    if (dudeHasQuirk(QUIRK_GLASS_CANNON) && stat == STAT_MAXIMUM_HIT_POINTS) {
        bonus -= currentValueBeforeBonus / 2;
    }

    // Sturdy: +3 Damage Threshold (Normal).
    if (dudeHasQuirk(QUIRK_STURDY) && stat == STAT_DAMAGE_THRESHOLD) {
        bonus += 3;
    }

    // Cult of the Mushroom Cloud: +50 rad resist, -20 to laser/plasma/electrical DT.
    if (dudeHasQuirk(QUIRK_CULT_OF_THE_MUSHROOM_CLOUD)) {
        if (stat == STAT_RADIATION_RESISTANCE) bonus += 50;
        if (stat == STAT_DAMAGE_RESISTANCE_LASER) bonus -= 20;
        if (stat == STAT_DAMAGE_RESISTANCE_PLASMA) bonus -= 20;
        if (stat == STAT_DAMAGE_RESISTANCE_ELECTRICAL) bonus -= 20;
    }

    // Death Wish: +3 max AP when current HP is below 25% of max.
    if (dudeHasQuirk(QUIRK_DEATH_WISH) && stat == STAT_MAXIMUM_ACTION_POINTS) {
        if (gDude != nullptr) {
            int curHp = gDude->data.critter.hp;
            int maxHpBase = critterGetBaseStatWithTraitModifier(gDude, STAT_MAXIMUM_HIT_POINTS)
                + critterGetBonusStat(gDude, STAT_MAXIMUM_HIT_POINTS);
            if (maxHpBase > 0 && curHp * 4 < maxHpBase) {
                bonus += 3;
            }
        }
    }

    return bonus;
}

int quirksGetToHitBonus(int hitMode)
{
    if (!dudeHasQuirk(QUIRK_GUNSLINGER) || gDude == nullptr) {
        return 0;
    }

    if (isPistolWeapon(gDude, hitMode)) {
        return 10;
    }
    if (isRifleOrBigGunWeapon(gDude, hitMode)) {
        return -10;
    }
    return 0;
}

int quirksGetHitLocationPenalty(int hitLocation, int basePenalty)
{
    if (!dudeHasQuirk(QUIRK_HEADHUNTER)) {
        return basePenalty;
    }

    // Penalties are stored as positive ints subtracted from to-hit. Halving
    // the head/eye penalties improves accuracy; introducing torso penalty
    // is the cost.
    switch (hitLocation) {
    case HIT_LOCATION_HEAD:
        return basePenalty / 2; // -40 -> -20
    case HIT_LOCATION_EYES:
        return (basePenalty * 2) / 3; // -60 -> -40
    case HIT_LOCATION_TORSO:
        return basePenalty + 20; // 0 -> +20 (penalty)
    default:
        return basePenalty;
    }
}

double quirksGetXpScale()
{
    return dudeHasQuirk(QUIRK_QUICK_LEARNER) ? 1.20 : 1.0;
}

bool quirksDisablesCalledShots()
{
    return dudeHasQuirk(QUIRK_BERSERKER);
}

const char* quirksGetName(int quirk)
{
    if (quirk < 0 || quirk >= QUIRK_COUNT) return nullptr;
    return kQuirkDefs[quirk].displayName;
}

} // namespace fallout
