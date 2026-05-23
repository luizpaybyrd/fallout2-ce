#ifndef FALLOUT_QUIRKS_H_
#define FALLOUT_QUIRKS_H_

namespace fallout {

// Up to 2 quirks can be active, picked at game start from ddraw.ini:
//   [Misc]
//   Quirks=GlassCannon,Headhunter
//
// Each quirk is a passive trait with a meaningful upside and downside,
// inspired by Wasteland 3 and Fallout: New Vegas. They layer on top of
// F2's existing trait/perk system rather than replacing it.
enum Quirk {
    QUIRK_NONE = -1,
    QUIRK_GLASS_CANNON = 0,
    QUIRK_HEADHUNTER,
    QUIRK_SOCIOPATH,
    QUIRK_BERSERKER,
    QUIRK_GUNSLINGER,
    QUIRK_CULT_OF_THE_MUSHROOM_CLOUD,
    QUIRK_LONE_WANDERER,
    QUIRK_QUICK_LEARNER,
    QUIRK_DEATH_WISH,
    QUIRK_STURDY,
    QUIRK_COUNT,
};

// Parse the [Misc] Quirks=... key from sfall config and populate the
// active-quirks set. Safe to call repeatedly; later calls overwrite.
void quirksInit();

// Whether the given quirk is currently active for the player.
bool dudeHasQuirk(int quirk);

// Reset the "first hit this combat" tracker. Call from _combat_begin().
void quirksResetCombatState();

// Multiplier applied to outgoing damage from the player. Returns 1.0 if no
// damage-affecting quirk is active. Also flips the first-hit flag.
double quirksGetDamageScale(int hitMode);

// Bonus added to STAT_X for the player only. Returns 0 for stats no quirk
// affects, or for non-player critters. Apply at the END of critterGetStat,
// after the normal clamp.
int quirksGetStatBonus(int stat, int currentValueBeforeBonus);

// To-hit modifier from quirks (Gunslinger). Returns 0 if inactive.
int quirksGetToHitBonus(int hitMode);

// Returns the (possibly modified) hit-location penalty. Pass the default
// value; the function returns it unchanged unless a quirk overrides.
int quirksGetHitLocationPenalty(int hitLocation, int basePenalty);

// XP multiplier (Quick Learner). Returns 1.0 if inactive.
double quirksGetXpScale();

// True if a quirk prevents the called-shot dialog from appearing (Berserker).
bool quirksDisablesCalledShots();

// Human-readable display name for the quirk (or nullptr).
const char* quirksGetName(int quirk);

} // namespace fallout

#endif /* FALLOUT_QUIRKS_H_ */
