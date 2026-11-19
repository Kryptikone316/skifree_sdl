#!/bin/bash
# Builds the SF3500 packages in dist/ (default OUT):
#   skifree/                 the complete SD-card folder (core + all modules), and SkiFree.sh beside it
#   skifree-sf3500.zip       the same, zipped
#   core/skifree/            the core only: what goes in the Ports Collection (no sounds, no sprite packs)
#   modules/skifree-sound-module.zip, skifree-modern-16bit-module.zip, skifree-modern-8bit-module.zip,
#   modules/skifree-all-modules.zip      optional downloads, unzipped into roms/ports/skifree/
# Needs the MIPS toolchain (see build_sf3500.sh) and Python 3 with Pillow, numpy and scipy.
# ski32.exe is NOT included: players put their own copy in skifree/gamedata/ (see sf3500/package/skifree/README.txt).
set -euo pipefail
cd "$(dirname "$0")/.."
PY=${PYTHON:-python3}
OUT=${OUT:-dist}

./build_sf3500.sh
mkdir -p "$OUT/skifree/sounds" "$OUT/skifree/sprites16" "$OUT/skifree/sprites8" "$OUT/core/skifree" "$OUT/modules" "$OUT/stage"
cp -r sf3500/package/skifree/. "$OUT/skifree/"
cp sf3500/package/SkiFree.sh "$OUT/"
cp build-sf3500/skifree.stripped.mipsel "$OUT/skifree/skifree.mipsel"
cp sf3500/assets/boxart.png "$OUT/skifree/boxart.png"
mkdir -p "$OUT/skifree/licenses"
cp sf3500/LICENSE-ASSETS "$OUT/skifree/licenses/LICENSE-ASSETS.txt"
"$PY" sf3500/tools/make_sounds.py "$OUT/skifree/sounds"
"$PY" sf3500/tools/make_sprites16.py sf3500/ttfshim.c "$OUT/skifree/sprites16"
"$PY" sf3500/tools/make_sprites8.py "$OUT/skifree/sprites16" "$OUT/skifree/sprites8" sf3500/palette8.h
"$PY" - "$OUT/skifree/launcher.bmp" <<'PYEOF'
import sys
from PIL import Image
Image.open('sf3500/assets/launcher-backdrop.png').convert('RGB').resize((640, 480), Image.LANCZOS).save(sys.argv[1], format='BMP')
PYEOF
chmod +x "$OUT/SkiFree.sh" "$OUT/skifree/run.sh"

# modules: the folder from the full package plus the module's README and licence, zipped on its own
for pair in sound:sounds modern-16bit:sprites16 modern-8bit:sprites8; do
    m=${pair%%:*}; d=${pair##*:}
    mkdir -p "$OUT/stage/$m/$d"
    cp "$OUT/skifree/$d/"* "$OUT/stage/$m/$d/"
    cp sf3500/package/modules/$m/README.txt sf3500/package/modules/$m/LICENSE.txt "$OUT/stage/$m/$d/"
    ( cd "$OUT/stage/$m" && "$PY" -m zipfile -c "../../modules/skifree-$m-module.zip" "$d" )
done
mkdir -p "$OUT/stage/all"
for pair in sound:sounds modern-16bit:sprites16 modern-8bit:sprites8; do
    cp -r "$OUT/stage/${pair%%:*}/${pair##*:}" "$OUT/stage/all/"
done
( cd "$OUT/stage/all" && "$PY" -m zipfile -c ../../modules/skifree-all-modules.zip sounds sprites16 sprites8 )

# the core: everything except the modules (the Ports Collection ships this)
for f in skifree.mipsel run.sh README.txt boxart.png launcher.bmp; do cp "$OUT/skifree/$f" "$OUT/core/skifree/$f"; done
cp -r "$OUT/skifree/gamedata" "$OUT/skifree/licenses" "$OUT/core/skifree/"

# the complete package
( cd "$OUT" && "$PY" -m zipfile -c skifree-sf3500.zip SkiFree.sh skifree )
ls -l "$OUT/skifree-sf3500.zip" "$OUT/modules/"*.zip
