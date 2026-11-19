#!/bin/bash
# Builds the SF3500 SD-card package in dist/: dist/SkiFree.sh, dist/skifree/ and dist/skifree-sf3500.zip.
#   - the binary from build_sf3500.sh (needs the MIPS toolchain, see that script)
#   - the sprite packs and sounds, drawn and synthesised by sf3500/tools/ (Python 3 with Pillow, numpy, scipy)
#   - the launcher backdrop converted to 640x480, the box art, and the files in sf3500/package/
# ski32.exe is NOT included: players put their own copy in skifree/gamedata/ (see sf3500/package/skifree/README.txt).
set -euo pipefail
cd "$(dirname "$0")/.."
PY=${PYTHON:-python3}
OUT=${OUT:-dist}

./build_sf3500.sh
mkdir -p "$OUT/skifree/sounds" "$OUT/skifree/sprites16" "$OUT/skifree/sprites8"
cp -r sf3500/package/. "$OUT/"
cp build-sf3500/skifree.stripped.mipsel "$OUT/skifree/skifree.mipsel"
cp sf3500/assets/boxart.png "$OUT/skifree/boxart.png"
cp sf3500/LICENSE-ASSETS "$OUT/skifree/LICENSE-ASSETS.txt"
"$PY" sf3500/tools/make_sounds.py "$OUT/skifree/sounds"
"$PY" sf3500/tools/make_sprites16.py sf3500/ttfshim.c "$OUT/skifree/sprites16"
"$PY" sf3500/tools/make_sprites8.py "$OUT/skifree/sprites16" "$OUT/skifree/sprites8" sf3500/palette8.h
"$PY" - "$OUT/skifree/launcher.bmp" <<'PYEOF'
import sys
from PIL import Image
Image.open('sf3500/assets/launcher-backdrop.png').convert('RGB').resize((640, 480), Image.LANCZOS).save(sys.argv[1], format='BMP')
PYEOF
chmod +x "$OUT/SkiFree.sh" "$OUT/skifree/run.sh"
( cd "$OUT" && "$PY" -m zipfile -c skifree-sf3500.zip SkiFree.sh skifree )
ls -l "$OUT/skifree-sf3500.zip"
