#include "sfall_starting_gift.h"

#include "config.h"
#include "critter.h"
#include "item.h"
#include "map.h"
#include "obj_types.h"
#include "object.h"
#include "sfall_config.h"
#include "sfall_global_vars.h"
#include "tile.h"

namespace fallout {

// Map header index assigned to Arroyo Village (ARVILLAG.MAP, read from
// master.dat:maps/arvillag.map header).
static constexpr int kArroyoVillageMapIndex = 4;

// Prototype IDs derived from master.dat proto data.
// Critter PID encoding: high byte = OBJ_TYPE_CRITTER (1), low 24 bits = index.
// Item PID encoding: high byte = OBJ_TYPE_ITEM (0), low 24 bits = index.
static constexpr int kAlienCritterPid = 0x010000F2; // proto/critters/00000242.pro
static constexpr int kAlienBlasterPid = 0x00000078; // proto/items/00000120.pro

// Persistent savegame flag — sfall global vars survive map transitions and saves.
static constexpr const char* kStartingGiftGivenKey = "starting_gift_given";

// Walks outward from `centerTile` in concentric hex rings up to `maxDistance`,
// returns the first hex with no blocking object, or -1 if all are blocked.
static int findClearTileNear(int centerTile, int elevation, int maxDistance)
{
    for (int distance = 1; distance <= maxDistance; distance++) {
        for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
            int t = tileGetTileInDirection(centerTile, rotation, distance);
            if (t >= 0 && _obj_blocking_at(nullptr, t, elevation) == nullptr) {
                return t;
            }
        }
    }
    return -1;
}

void sfallStartingGiftCheck()
{
    int enabled = 0;
    configGetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_STARTING_GIFT_KEY, &enabled);
    if (!enabled) {
        return;
    }

    if (gMapHeader.index != kArroyoVillageMapIndex) {
        return;
    }

    int alreadyGiven = 0;
    sfall_gl_vars_fetch(kStartingGiftGivenKey, alreadyGiven);
    if (alreadyGiven) {
        return;
    }

    int elev = gMapHeader.enteringElevation;
    int alienTile = findClearTileNear(gMapHeader.enteringTile, elev, 3);
    if (alienTile < 0) {
        return;
    }

    Object* alien = nullptr;
    if (objectCreateWithPid(&alien, kAlienCritterPid) != 0 || alien == nullptr) {
        return;
    }
    objectSetLocation(alien, alienTile, elev, nullptr);
    critterKill(alien, -1, true);

    int blasterTile = findClearTileNear(alienTile, elev, 2);
    if (blasterTile < 0) {
        blasterTile = alienTile;
    }
    Object* blaster = nullptr;
    if (objectCreateWithPid(&blaster, kAlienBlasterPid) == 0 && blaster != nullptr) {
        objectSetLocation(blaster, blasterTile, elev, nullptr);
        int capacity = ammoGetCapacity(blaster);
        if (capacity > 0) {
            ammoSetQuantity(blaster, capacity);
        }
    }

    sfall_gl_vars_store(kStartingGiftGivenKey, 1);
}

} // namespace fallout
