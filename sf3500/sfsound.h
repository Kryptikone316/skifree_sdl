/* Sound for the SF3500 port: nine game sounds on the shared software mixer
 * (sf3500_mixer -> hwaudio). The game's own sounds are not distributable, so
 * the port plays sounds of its own (sounds/<name>.wav) and, if the player
 * wants, replacements from sounds/custom/<name>.wav. */
#ifndef SF3500_SFSOUND_H
#define SF3500_SFSOUND_H

/* Starts the mixer and loads the sounds now (so the first sound in the game does not stall). */
void sfsnd_prepare(void);

/* Slots 1..9 are the game's sound_1..sound_9. */
void sfsnd_play(int slot);
void sfsnd_play_by_name(const char* name);   /* menu previews */
int sfsnd_custom_count(void);                /* custom files present in sounds/custom/ */
const char* sfsnd_slot_name(int slot);
void sfsnd_shutdown(void);

#endif
