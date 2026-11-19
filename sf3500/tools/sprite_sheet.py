#!/usr/bin/env python3
"""Side-by-side sheet: original sprite | new sprite, for the ids found in the pack folder.
Usage: sprite_sheet.py <original-bmp-dir> <pack-dir> <out.png>"""
import sys, glob, os, re
from PIL import Image, ImageDraw
orig, pack, out = sys.argv[1:4]
ids = sorted(int(re.findall(r'(\d+)', os.path.basename(p))[-1]) for p in glob.glob(os.path.join(pack, 'ski32_*.bmp')))
S = int(os.environ.get("SS", 3)); COLS = int(os.environ.get("SC", 4)); CW = int(os.environ.get("SW", 330)); bg = (214, 224, 240)
rows = (len(ids) + COLS - 1) // COLS
heights = [max(Image.open(os.path.join(pack, f'ski32_{i}.bmp')).height for i in ids[r * COLS:(r + 1) * COLS]) * S + 24 for r in range(rows)]
sheet = Image.new('RGB', (COLS * CW, sum(heights)), bg)
d = ImageDraw.Draw(sheet)
y = 0
for r in range(rows):
    for c in range(COLS):
        k = r * COLS + c
        if k >= len(ids): break
        i = ids[k]; x = c * CW
        a = Image.open(os.path.join(orig, f'ski32_{i}.bmp')).convert('RGB'); b = Image.open(os.path.join(pack, f'ski32_{i}.bmp')).convert('RGB')
        d.rectangle([x + 1, y + 1, x + CW - 2, y + heights[r] - 2], outline=(150, 160, 190))
        d.text((x + 5, y + 4), f'#{i}  {a.width}x{a.height}' + ('' if a.size == b.size else '  SIZE MISMATCH %dx%d' % b.size), fill=(0, 0, 90))
        sheet.paste(a.resize((a.width * S, a.height * S), Image.NEAREST), (x + 6, y + 20))
        sheet.paste(b.resize((b.width * S, b.height * S), Image.NEAREST), (x + CW // 2 + 4, y + 20))
    y += heights[r]
sheet.save(out); print(sheet.size, len(ids))
