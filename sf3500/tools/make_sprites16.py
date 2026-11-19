#!/usr/bin/env python3
"""Draws the "16-bit" sprite pack for the SF3500 SkiFree port: original art (MIT, see sf3500/LICENSE-ASSETS), same sizes as the
exe's sprites (so hitboxes do not change), written as 24-bit BMPs named ski32_<id>.bmp.
White (255,255,255) is the game's transparent colour: the art never uses it (snow white is 250,252,255).
Usage: make_sprites16.py <font-source.c> <output-dir>
Stage 1: scenery and objects. Characters (skier, yeti, snowboarders, dogs) come later; anything not
drawn here falls back to the exe's own sprite in the game."""
import re, sys, os, math
from PIL import Image, ImageDraw

KEY = (255, 255, 255)
SIZES = {}   # id -> (w, h) of the exe's sprites, read from sprites-reference/INDEX.txt when present (sanity check)
_idx = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'sprites-reference', 'INDEX.txt')
if os.path.exists(_idx):
    for _l in open(_idx).read().split('\n')[1:]:
        _m = re.match(r'\s*(\d+)\s+(\d+)\s+x\s+(\d+)', _l)
        if _m: SIZES[int(_m.group(1))] = (int(_m.group(2)), int(_m.group(3)))
SNOW = (250, 252, 255)
SNOW_SH = (200, 216, 244)
SNOW_SH2 = (160, 184, 232)
OUT = (28, 32, 46)

G_DARK, G_MID, G_LITE, G_LINE = (18, 84, 52), (34, 122, 74), (72, 168, 98), (10, 52, 36)
BARK, BARK_L = (112, 70, 36), (156, 100, 54)

# ---- font8x8 (public domain) for the sign text ----
FONT = {}
def load_font(path):
    src = open(path).read()
    body = src[src.index('font8x8_basic[128][8]'):]
    rows = re.findall(r'\{\s*((?:0x[0-9A-Fa-f]{2}\s*,?\s*){8})\}', body)
    for code, r in enumerate(rows[:128]):
        FONT[code] = [int(h, 16) for h in re.findall(r'0x([0-9A-Fa-f]{2})', r)]

def text(d, x, y, s, col, tight=6, shadow=None):
    for ch in s:
        g = FONT.get(ord(ch), FONT[32])
        for gy in range(8):
            for gx in range(8):
                if g[gy] & (1 << gx):
                    if shadow: d.point((x + gx + 1, y + gy + 1), fill=shadow)
        for gy in range(8):
            for gx in range(8):
                if g[gy] & (1 << gx): d.point((x + gx, y + gy), fill=col)
        x += tight

def new(w, h):
    im = Image.new('RGB', (w, h), KEY)
    return im, ImageDraw.Draw(im)

def save(im, outdir, n):
    # never leave pure white inside art by accident (only the background may be the key)
    im.save(os.path.join(outdir, f'ski32_{n}.bmp'), format='BMP')

# ---------- trees ----------
def pine(w, h, tiers, variant=0):
    im, d = new(w, h)
    cx = w // 2
    tw = 4 if w >= 26 else 3
    th = max(3, h // 8)
    d.rectangle([cx - tw // 2, h - th, cx + tw // 2 - 1, h - 1], fill=BARK, outline=None)
    d.line([cx - tw // 2, h - th, cx - tw // 2, h - 1], fill=BARK_L)
    body_h = h - th + 2
    for i in range(tiers):
        top = int(i * body_h / (tiers + 0.6))
        bot = int(top + body_h * 1.55 / (tiers + 0.6))
        bot = min(bot, h - th + 1)
        maxhw = int((w / 2 - 1) * (0.58 + 0.42 * (i + 1) / tiers))
        for y in range(top, bot + 1):
            t = (y - top) / max(1, bot - top)
            hw = max(1, int(maxhw * (0.12 + 0.88 * t)))
            for x in range(-hw, hw + 1):
                if x < -hw * 0.35: c = G_LITE
                elif x < hw * 0.35: c = G_MID
                else: c = G_DARK
                if x in (-hw, hw) or y == bot: c = G_LINE
                elif (x + y + variant) % 5 == 0 and abs(x) < hw - 1 and t > 0.3: c = G_LINE if x > 0 else G_MID  # needles texture
                d.point((cx + x, y), fill=c)
            # snow along the upper-left edge and a cap on the first rows of each tier
            if y - top < 3 and i == 0:
                for x in range(-hw + 1, hw): d.point((cx + x, y), fill=SNOW if x < hw - 1 else SNOW_SH)
            elif y - top < 2:
                for x in range(-hw + 1, hw - 1): d.point((cx + x, y), fill=SNOW if (x + variant) % 3 else SNOW_SH)
            if t > 0.55 and (y + variant) % 4 == 0:
                d.point((cx - hw + 1, y), fill=SNOW)
        # snow clumps resting on the tier's lower edges (different per variant)
        for k in range(2 + variant % 2):
            sx = ((k * 7 + variant * 5 + i * 3) % max(3, maxhw)) - maxhw // 2
            d.point((cx + sx, bot - 1), fill=SNOW); d.point((cx + sx + 1, bot - 1), fill=SNOW_SH)
            d.point((cx + sx, bot - 2), fill=SNOW)
    return im

def bare_tree(w, h):
    im, d = new(w, h)
    cx = w // 2
    d.line([cx, h - 1, cx, h // 3], fill=BARK, width=3)
    d.line([cx - 1, h - 1, cx - 1, h // 3], fill=BARK_L, width=1)
    for (x0, y0, x1, y1) in [(cx, h * 0.55, 3, h * 0.3), (cx, h * 0.45, w - 4, h * 0.2), (cx, h * 0.33, 5, 2), (cx, h * 0.3, w - 6, 1), (cx, h * 0.7, w - 3, h * 0.5), (cx, h * 0.75, 2, h * 0.58)]:
        d.line([cx, y0, x1, y1], fill=BARK, width=1)
        d.point((int(x1), int(y1) - 1), fill=SNOW)
        d.point((int((x1 + cx) / 2), int((y1 + y0) / 2) - 1), fill=SNOW_SH)
    d.line([cx - 2, h - 1, cx + 2, h - 1], fill=SNOW_SH)
    return im

def burning_tree(w, h, frame):
    im, d = new(w, h)
    cx = w // 2
    d.line([cx, h - 1, cx, h // 2], fill=(70, 40, 24), width=3)
    d.line([cx, h - 8, 4, h - 14], fill=(70, 40, 24), width=1); d.line([cx, h - 10, w - 5, h - 15], fill=(70, 40, 24), width=1)
    sh = [0, 2, -2][frame]
    flames = [(cx + sh - 7, h - 8, 5, 12), (cx + sh + 6, h - 8, 5, 13), (cx + sh, h - 14, 7, 18), (cx - sh - 3, h - 12, 4, 14), (cx + sh + 3, h - 11, 4, 15)]
    for (fx, fy, fw, fh) in flames:
        for layer, col in enumerate([(214, 40, 20), (250, 140, 20), (255, 226, 70)]):
            k = 1 - layer * 0.28
            ww, hh = max(1, int(fw * k)), max(2, int(fh * k))
            pts = [(fx - ww, fy), (fx - ww // 2, fy - hh // 2), (fx + (frame - 1), fy - hh), (fx + ww // 2, fy - hh // 2), (fx + ww, fy)]
            d.polygon(pts, fill=col)
    d.line([cx - 4, h - 1, cx + 4, h - 1], fill=(120, 60, 30))
    return im

# ---------- rocks, bumps, clouds ----------
def rock(w, h):
    im, d = new(w, h)
    for y in range(h):
        t = y / (h - 1)
        hw = int((w / 2 - 1) * math.sqrt(max(0.0, 1 - (1 - t * 0.96) ** 2.2)))
        for x in range(-hw, hw + 1):
            c = (176, 180, 194) if x < -hw * 0.2 and t < 0.7 else ((128, 132, 148) if x < hw * 0.35 else (88, 92, 110))
            if x in (-hw, hw) or y == h - 1: c = (46, 48, 64)
            elif y < 3: c = SNOW if x < hw * 0.5 else SNOW_SH
            d.point((w // 2 + x, y), fill=c)
    return im

def stump(w, h):
    im, d = new(w, h)
    d.rectangle([2, 3, w - 3, h - 1], fill=BARK, outline=(60, 36, 20))
    d.rectangle([3, 4, 5, h - 2], fill=BARK_L)
    d.ellipse([1, 0, w - 2, 5], fill=(190, 140, 80), outline=(60, 36, 20))
    d.ellipse([w // 2 - 3, 1, w // 2 + 2, 4], outline=(130, 88, 46))
    d.line([2, 0, 6, 0], fill=SNOW)
    return im

def bump(w, h):
    im, d = new(w, h)
    for x in range(w):
        t = (x + 0.5) / w * 2 - 1
        top = int((h - 1) * (1 - math.sqrt(max(0, 1 - t * t))) * 0.9)
        for y in range(top, h):
            c = SNOW_SH if y >= h - 2 else (SNOW if y < top + 2 else (226, 236, 252))
            if y == top: c = SNOW_SH2
            d.point((x, y), fill=c)
    return im

def clouds(w, h):
    im, d = new(w, h)
    blobs = [(10, 20, 11), (22, 14, 10), (36, 17, 12), (50, 14, 9), (56, 22, 8), (28, 25, 11), (14, 26, 8)]
    for (bx, by, r) in blobs:
        d.ellipse([bx - r, by - r // 2 - 2, bx + r, by + r // 2], fill=(238, 244, 255), outline=(176, 196, 238))
    for (bx, by, r) in blobs:
        d.ellipse([bx - r + 2, by - r // 2, bx + r - 2, by + r // 2 - 2], fill=(238, 244, 255))
    d.line([3, 27, 60, 27], fill=(214, 226, 250))
    return im

def puddle(w, h):
    im, d = new(w, h)
    d.ellipse([0, 1, w - 1, h - 2], fill=(230, 214, 40), outline=(176, 150, 20))
    d.ellipse([3, 2, w // 2, h // 2], fill=(252, 240, 110))
    return im

def rainbow(w, h):
    im, d = new(w, h)
    cols = [(220, 36, 36), (244, 140, 28), (246, 220, 40), (50, 180, 60), (40, 110, 230), (140, 60, 200), (60, 40, 120), (40, 24, 80)]
    for y in range(h): d.line([0, y, w - 1, y], fill=cols[y % len(cols)])
    return im

# ---------- post signs ----------
def post(im, d, cx, top, h):
    d.rectangle([cx - 1, top, cx, h - 2], fill=BARK, outline=None)
    d.line([cx - 1, top, cx - 1, h - 2], fill=BARK_L)
    d.line([cx - 3, h - 1, cx + 2, h - 1], fill=SNOW_SH)

def post_sign(kind):
    w, h = 12, 24
    im, d = new(w, h)
    post(im, d, 6, 11, h)
    if kind == 'left' or kind == 'right':
        col = (214, 36, 40) if kind == 'left' else (40, 90, 220)
        d.rounded_rectangle([0, 0, 11, 11], radius=2, fill=col, outline=OUT)
        ar = SNOW
        if kind == 'left':
            d.line([3, 6, 8, 6], fill=ar); d.line([3, 6, 5, 4], fill=ar); d.line([3, 6, 5, 8], fill=ar)
        else:
            d.line([3, 6, 8, 6], fill=ar); d.line([8, 6, 6, 4], fill=ar); d.line([8, 6, 6, 8], fill=ar)
    else:
        happy = kind == 'happy'
        col = (60, 190, 60) if happy else (220, 40, 40)
        d.ellipse([0, 0, 11, 11], fill=col, outline=OUT)
        d.point((3, 4), fill=OUT); d.point((8, 4), fill=OUT)
        if happy: d.line([3, 8, 4, 9], fill=OUT); d.line([4, 9, 7, 9], fill=OUT); d.line([7, 9, 8, 8], fill=OUT)
        else: d.line([3, 9, 4, 8], fill=OUT); d.line([4, 8, 7, 8], fill=OUT); d.line([7, 8, 8, 9], fill=OUT)
        d.point((4, 2), fill=(255, 255, 200)) if happy else None
    return im

# ---------- course boards ----------
def board(w, h, body, edge, label_lines, arrow, tcol, shadow=None, tip=None, posts=True):
    im, d = new(w, h)
    bh = h - 8 if posts else h
    if tip == 'right':
        d.polygon([(0, 0), (w - 9, 0), (w - 1, bh // 2), (w - 9, bh - 1), (0, bh - 1)], fill=body, outline=edge)
    elif tip == 'left':
        d.polygon([(w - 1, 0), (8, 0), (0, bh // 2), (8, bh - 1), (w - 1, bh - 1)], fill=body, outline=edge)
    else:
        d.rounded_rectangle([0, 0, w - 1, bh - 1], radius=3, fill=body, outline=edge)
    d.line([2 if tip != 'left' else 9, 1, (w - 10) if tip == 'right' else w - 3, 1], fill=tuple(min(255, c + 60) for c in body))   # top highlight
    if posts:
        for px_ in (2, w - 4):
            d.rectangle([px_, bh, px_ + 1, h - 2], fill=(70, 74, 90), outline=None)
            d.line([px_ - 1, h - 1, px_ + 2, h - 1], fill=SNOW_SH)
    return im, d, bh

def start_flag(right):
    w, h = 42, 27
    body, edge = (74, 206, 72), (14, 80, 28)
    im, d, bh = board(w, h, body, edge, None, None, None, tip='right' if right else 'left')
    off = 3 if right else 11
    text(d, off, 4, 'Start', (14, 56, 24), tight=6)
    return im

def finish_flag(right):
    w, h = 50, 29
    body, edge = (226, 56, 60), (110, 14, 24)
    im, d, bh = board(w, h, body, edge, None, None, None, tip='right' if right else 'left')
    off = 4 if right else 12
    text(d, off, 6, 'Finish', SNOW, tight=6, shadow=(110, 14, 24))
    return im

def arrow_glyph(d, cx, cy, dx, dy, col, size=6):
    ex, ey = cx + dx * size, cy + dy * size
    d.line([cx - dx * size, cy - dy * size, ex, ey], fill=col, width=2)
    if dx == 0:
        d.polygon([(ex - 4, ey - 4 * dy), (ex + 4, ey - 4 * dy), (ex, ey + 1 * dy)], fill=col)
    else:
        d.polygon([(ex - 4 * dx, ey - 4), (ex - 4 * dx, ey + 4), (ex + 1 * dx, ey)], fill=col)

def darrow(d, x0, y0, x1, y1, col, width=2, head=5):
    d.line([x0, y0, x1, y1], fill=col, width=width)
    L = math.hypot(x1 - x0, y1 - y0) or 1
    ux, uy = (x1 - x0) / L, (y1 - y0) / L
    bx, by = x1 - ux * head, y1 - uy * head
    nx, ny = -uy, ux
    d.polygon([(x1 + ux * 1.5, y1 + uy * 1.5), (bx + nx * head * 0.8, by + ny * head * 0.8), (bx - nx * head * 0.8, by - ny * head * 0.8)], fill=col)

def slalom_sign():
    w, h = 40, 36
    c = (8, 46, 70)
    im, d, bh = board(w, h, (44, 190, 200), (8, 76, 96), None, None, None)
    text(d, 2, 3, 'Slalom', c, tight=5)
    darrow(d, 27, 13, 14, 25, c)
    return im

def tree_slalom_sign():
    w, h = 44, 36
    c = (92, 52, 0)
    im, d, bh = board(w, h, (250, 224, 70), (120, 90, 10), None, None, None)
    text(d, 3, 3, 'Tree', c, tight=6)
    darrow(d, 29, 3, 39, 12, c, width=2, head=4)
    text(d, 3, 15, 'Slalom', c, tight=6)
    return im

def freestyle_sign():
    w, h = 40, 35
    c = (10, 56, 24)
    im, d, bh = board(w, h, (96, 206, 84), (16, 84, 34), None, None, None)
    text(d, 3, 3, 'Free-', c, tight=6)
    text(d, 2, 14, 'style', c, tight=6)
    darrow(d, 36, 4, 36, 22, c, width=2, head=4)
    return im

# ---------- lift ----------
def tower(w, h):
    im, d = new(w, h)
    cx = w // 2
    d.rectangle([cx - 2, 8, cx + 1, h - 1], fill=(36, 38, 50), outline=None)
    d.line([cx - 1, 8, cx - 1, h - 1], fill=(92, 98, 120))
    d.rectangle([cx - 2, h - 20, cx + 1, h - 1], fill=(246, 206, 40))
    d.line([cx - 2, h - 20, cx - 2, h - 1], fill=(255, 238, 120))
    d.line([cx + 1, h - 20, cx + 1, h - 1], fill=(176, 130, 20))
    d.rectangle([cx - 1, h - 38, cx, h - 34], fill=(236, 120, 40), outline=(90, 40, 10))
    d.rectangle([2, 4, w - 3, 7], fill=(36, 38, 50))
    d.line([2, 4, w - 3, 4], fill=(92, 98, 120))
    for x in (0, w - 3): d.rectangle([x, 0, x + 2, 8], fill=(36, 38, 50))
    d.line([cx - 4, h - 1, cx + 3, h - 1], fill=SNOW_SH)
    return im

def chair(w, h, riders):
    im, d = new(w, h)
    cx = w // 2
    d.line([cx, 0, cx, 11], fill=(36, 38, 50))
    d.line([cx - 1, 0, cx - 1, 11], fill=(92, 98, 120))
    d.rectangle([2, 18, w - 3, 28], fill=(226, 36, 40), outline=(90, 12, 16))
    d.line([3, 19, w - 4, 19], fill=(250, 130, 120))
    d.line([3, 23, w - 4, 23], fill=(250, 244, 244))
    d.rectangle([5, 28, 7, 31], fill=(90, 98, 120)); d.rectangle([w - 8, 28, w - 6, 31], fill=(90, 98, 120))
    d.line([2, 12, 2, 18], fill=(36, 38, 50)); d.line([w - 3, 12, w - 3, 18], fill=(36, 38, 50))
    d.line([2, 12, w - 3, 12], fill=(36, 38, 50))
    for i, (hat, jacket) in enumerate(riders):
        x = 6 + i * 10
        d.rectangle([x, 8, x + 5, 12], fill=(244, 206, 70), outline=OUT)
        d.rectangle([x - 1, 5, x + 6, 8], fill=hat, outline=OUT)
        d.rectangle([x - 1, 13, x + 6, 20], fill=jacket, outline=OUT)
    return im

BATCH = {
    23: lambda: post_sign('left'), 24: lambda: post_sign('right'), 25: lambda: post_sign('happy'), 26: lambda: post_sign('sad'),
    27: lambda: clouds(64, 32),
    45: lambda: rock(23, 11), 46: lambda: stump(16, 11), 47: lambda: bump(16, 4), 48: lambda: bump(24, 8),
    49: lambda: pine(28, 32, 3, 0), 50: lambda: bare_tree(22, 27), 51: lambda: pine(32, 64, 5, 1),
    52: lambda: rainbow(32, 8),
    57: lambda: start_flag(True), 58: lambda: start_flag(False), 59: lambda: finish_flag(True), 60: lambda: finish_flag(False),
    61: slalom_sign, 62: tree_slalom_sign, 63: freestyle_sign,
    64: lambda: tower(24, 64),
    65: lambda: chair(26, 32, [((40, 150, 220), (210, 40, 190)), ((60, 170, 60), (240, 150, 30))]),
    66: lambda: chair(26, 32, [((222, 36, 40), (40, 92, 224))]),
    67: lambda: chair(26, 32, []),
    82: lambda: puddle(16, 8),
    83: lambda: burning_tree(22, 27, 0), 84: lambda: burning_tree(22, 27, 1), 85: lambda: burning_tree(22, 27, 2),
    87: lambda: pine(28, 32, 3, 0), 88: lambda: pine(28, 32, 3, 2), 89: lambda: pine(28, 32, 3, 3),
}
# ids 49, 87-89 share the size: give them slightly different snow patterns via the variant

if __name__ == '__main__':
    load_font(sys.argv[1])
    out = sys.argv[2]
    os.makedirs(out, exist_ok=True)
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import make_sprites16 as _m   # the characters module imports this file by name: give that copy the font too
    _m.load_font(sys.argv[1])
    try:
        import sprites16_chars
        BATCH.update(sprites16_chars.BATCH)
    except ImportError:
        pass
    for n, fn in BATCH.items():
        im = fn()
        assert im.size == SIZES.get(n, im.size), (n, im.size, SIZES.get(n))
        save(im, out, n)
    print(len(BATCH), 'sprites ->', out)
