#!/bin/sh
set -u

PORT_DIR=/mnt/sdcard/roms/ports/skifree
cd "$PORT_DIR"
rm -f /mnt/sdcard/cubegm/port_error.txt
export LD_LIBRARY_PATH=/mnt/sdcard/cubegm/lib:/mnt/sdcard/cubegm/usr/lib:${LD_LIBRARY_PATH:-}

# Settings (sound, screen, frame rate, yeti ...) live in skifree.txt, which the
# launcher menu in the program reads and writes. Nothing to parse here.

# No real SDL video or audio device on this hardware: SDL's dummy drivers let
# SDL start, the frames go to the device driver directly (see sf3500/ in the source).
export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy

# Output goes to log.txt in the port folder, with an exit code line per run.
# Exit code 10 means "back to the launcher menu" (SELECT+START while skiing): start again.
: > log.txt
rc=0
while :; do
    rc=0
    "$PORT_DIR/skifree.mipsel" < /dev/null >> log.txt 2>&1 || rc=$?
    echo "EXIT CODE: $rc" >> log.txt
    [ "$rc" -eq 10 ] || break
done
exit "$rc"
