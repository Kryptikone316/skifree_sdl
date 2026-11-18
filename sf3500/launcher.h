/* SkiFree launcher menu: Play Game / Audio-Video / Gameplay / Exit.
 * Settings are edited in g_cfg and saved to skifree.txt on the way out. */
#ifndef SF3500_LAUNCHER_H
#define SF3500_LAUNCHER_H

/* Returns 1 = play, 0 = exit the port. */
int launcher_run(const char* cfg_path);

#endif
