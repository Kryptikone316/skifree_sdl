/* SF3500 gamepad -> SkiFree input. Included into skifree_decomp.c (the game's
 * state lives in data.h variables of that one translation unit).
 *
 *   D-pad          arrow keys (turn, down, up/stop), repeating like a held key
 *   A / Y          straight down-right / down-left   (keyboard C / A)
 *   B, R1          jump (the mouse-click jump), also restarts after a crash
 *   X              short hop (keyboard ` )
 *   R2 / L2        the F key (speed boost on/off) / the S key (half speed on/off), a tap each
 *   START          new game (F2)         SELECT  pause/resume (F3), on release
 *   SELECT+START   back to the launcher menu
 */
#ifdef SF3500
#include "sf3500.h"

void handleGameReset(void);

static void sfKey(SDL_Keycode k) {
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = SDL_KEYDOWN;
    e.key.keysym.sym = k;
    handleKeydownMessage(&e);
}

static void sfJump(void) {
    handleMouseClick();
}

typedef struct { unsigned mask; SDL_Keycode key; int repeat; } SfKey;
static const SfKey sfKeys[] = {
    { SB_LEFT, SDLK_LEFT, 1 },   { SB_RIGHT, SDLK_RIGHT, 1 },
    { SB_UP, SDLK_UP, 1 },       { SB_DOWN, SDLK_DOWN, 1 },
    { SB_A, SDLK_c, 0 },         { SB_Y, SDLK_a, 0 },
    { SB_X, SDLK_BACKQUOTE, 0 },
};

#define SF_REPEAT_DELAY 230
#define SF_REPEAT_EVERY 90

static void sfPollInput(void) {
    static unsigned prev, nextRepeat[16];
    static int selectDown, comboUsed;
    static int traceOn = -1;
    static unsigned polls;
    unsigned b = sf3500_buttons(), pressed = b & ~prev, released = prev & ~b;
    unsigned now = SDL_GetTicks();
    size_t i;

    if ((b & SB_START) && (b & SB_SELECT)) sf3500_quit(SF_EXIT_MENU);

    /* test hook: SKI_TRACE=<n> prints the game state every n frames */
    if (traceOn < 0) traceOn = getenv("SKI_TRACE") ? atoi(getenv("SKI_TRACE")) : 0;
    if (traceOn > 0 && ++polls % (unsigned)traceOn == 0)
        fprintf(stderr, "trace ms=%u f=%u player=%s frame=%d dist=%d x=%d running=%d paused=%d turbo=%d slow=%d modes=%d%d%d t=%d ax=%d ay=%d\n", (unsigned)SDL_GetTicks(), polls,
                playerActor ? "alive" : "none", playerActor ? (int)playerActor->frameNo : -1, (int)playerY,
                (int)playerX, (int)isGameTimerRunning, (int)isPaused, (int)isTurboMode, (int)isSlowMode, (int)isSsGameMode, (int)isFsGameMode, (int)isGsGameMode, (int)elapsedTime, playerActor ? (int)playerActor->xPosMaybe : 0, playerActor ? (int)playerActor->yPosMaybe : 0);

    for (i = 0; i < sizeof(sfKeys) / sizeof(sfKeys[0]); i++) {
        const SfKey* k = &sfKeys[i];
        if (pressed & k->mask) {
            sfKey(k->key);
            nextRepeat[i] = now + SF_REPEAT_DELAY;
        } else if ((b & k->mask) && k->repeat && now >= nextRepeat[i]) {
            sfKey(k->key);
            nextRepeat[i] = now + SF_REPEAT_EVERY;
        }
    }
    if (pressed & (SB_B | SB_R1)) sfJump();
    /* R2 = the F key (double speed), L2 = the S key (half speed): one tap each, they cancel each other */
    if (pressed & SB_R2) { isTurboMode = (isTurboMode == 0); if (isTurboMode) isSlowMode = 0; }
    if (pressed & SB_L2) { isSlowMode = (isSlowMode == 0); if (isSlowMode) isTurboMode = 0; }
    if ((pressed & SB_START) && !(b & SB_SELECT)) handleGameReset();

    if (pressed & SB_SELECT) { selectDown = 1; comboUsed = 0; }
    if ((b & SB_SELECT) && (b & ~SB_SELECT)) comboUsed = 1;
    if ((released & SB_SELECT) && selectDown && !comboUsed) togglePausedState();
    if (released & SB_SELECT) selectDown = 0;

    prev = b;
}
#endif
