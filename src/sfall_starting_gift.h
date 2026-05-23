#ifndef FALLOUT_SFALL_STARTING_GIFT_H_
#define FALLOUT_SFALL_STARTING_GIFT_H_

namespace fallout {

// Spawns a dead alien + loaded Alien Blaster in Arroyo Village on first visit,
// gated by the sfall config flag StartingGift and a savegame-persistent global var.
// No-op if the flag is off, the current map is not Arroyo Village, or the gift
// has already been given in this save.
void sfallStartingGiftCheck();

} // namespace fallout

#endif /* FALLOUT_SFALL_STARTING_GIFT_H_ */
