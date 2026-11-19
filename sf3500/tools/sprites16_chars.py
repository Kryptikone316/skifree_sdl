"""Stage 2: the characters (skier first). Skeleton-style drawing with shared limb/ski/head/hat helpers
so every pose uses the same colours and proportions. Imported by make_sprites16.py."""
import math
from PIL import Image, ImageDraw
from make_sprites16 import new, KEY, SNOW, SNOW_SH, SNOW_SH2, OUT, text

HAT, HAT_L, HAT_D, POMP = (222, 36, 40), (250, 116, 104), (140, 14, 28), (50, 110, 240)
SKIN, SKIN_L, SKIN_D = (240, 200, 70), (255, 232, 130), (178, 138, 30)
GOG, GOG_L = (32, 76, 214), (160, 204, 255)
JAC, JAC_L, JAC_D = (216, 56, 196), (248, 136, 228), (146, 26, 128)
PAN, PAN_L, PAN_D = (40, 94, 226), (104, 152, 252), (20, 50, 156)
BOOT, BOOT_L, BOOT_D = (38, 172, 54), (118, 224, 126), (16, 96, 30)
SKI, SKI_L, SKI_D = (248, 212, 40), (255, 242, 136), (172, 128, 10)
POLE = (40, 42, 56)
STASH = (150, 84, 30)

SKI_EDGE = (118, 84, 8)

def limb(d, a, b, w, fill, edge=OUT, hi=None, lo=None):
    d.line([a, b], fill=edge, width=w + 2)
    d.line([a, b], fill=fill, width=w)
    if hi is not None and w >= 2:
        d.line([(a[0] - 1, a[1]), (b[0] - 1, b[1])], fill=hi, width=1)
    if lo is not None and w >= 3:
        d.line([(a[0] + 1, a[1]), (b[0] + 1, b[1])], fill=lo, width=1)

def ski(d, a, b, w=2):
    d.line([(a[0] + 1, a[1] + 1), (b[0] + 1, b[1] + 1)], fill=SKI_EDGE, width=w + 1)   # dark underside
    d.line([a, b], fill=SKI, width=w + 1)
    d.line([(a[0] - 1, a[1] - 1), (b[0] - 1, b[1] - 1)], fill=SKI_L, width=1)

def pole(d, a, b, basket=True):
    d.line([a, b], fill=POLE, width=1)
    if basket:
        bx, by = b
        d.point((bx - 1, by), fill=POLE); d.point((bx + 1, by), fill=POLE)

def glove(d, p):
    d.rectangle([p[0] - 1, p[1] - 1, p[0], p[1]], fill=SKI)
    d.point((p[0] + 1, p[1]), fill=SKI_D)

def boot(d, p, wd=3):
    x, y = p
    d.rectangle([x - 1, y - 1, x + wd - 2, y + 2], fill=BOOT, outline=BOOT_D)
    d.point((x, y - 1), fill=BOOT_L)

def leg(d, hip, knee, foot, w=3, with_boot=True):
    limb(d, hip, knee, w, PAN, PAN_D, PAN_L)
    limb(d, knee, foot, w, PAN, PAN_D, PAN_L)
    if with_boot: boot(d, foot)

def arm(d, sh, el, hand, w=2):
    limb(d, sh, el, w, JAC, JAC_D, JAC_L)
    limb(d, el, hand, w, JAC, JAC_D, JAC_L)
    glove(d, hand)

def torso(d, sh, hip, w=7, front=True):
    limb(d, sh, hip, w, JAC, JAC_D, JAC_L, JAC_D)
    if front:
        limb(d, (sh[0], sh[1] + 2), (hip[0], hip[1] - 1), max(2, w - 4), PAN, PAN, PAN_L)
        d.line([(hip[0] - w // 2, hip[1]), (hip[0] + w // 2, hip[1])], fill=STASH)   # belt

def head(d, c, facing='front'):
    x, y = c
    rows = {-2: (-2, 2), -1: (-3, 3), 0: (-3, 3), 1: (-3, 3), 2: (-3, 3), 3: (-2, 2)}
    for dy, (x0, x1) in rows.items():
        d.line([(x + x0, y + dy), (x + x1, y + dy)], fill=SKIN)
        d.point((x + x0, y + dy), fill=SKIN_L if dy < 2 else SKIN)
        d.point((x + x1, y + dy), fill=SKIN_D)
    d.line([(x - 2, y + 3), (x + 2, y + 3)], fill=SKIN_D)
    if facing == 'front':
        d.line([(x - 3, y - 1), (x + 3, y - 1)], fill=GOG); d.line([(x - 3, y), (x + 3, y)], fill=GOG)
        d.point((x - 2, y - 1), fill=GOG_L); d.point((x + 1, y - 1), fill=GOG_L)
        d.point((x, y), fill=SKIN)          # nose bridge
        d.line([(x - 1, y + 2), (x + 1, y + 2)], fill=STASH)
    else:
        sgn = 1 if facing == 'right' else -1
        xs = sorted([x + sgn * 0, x + sgn * 3])
        d.line([(xs[0], y - 1), (xs[1], y - 1)], fill=GOG); d.line([(xs[0], y), (xs[1], y)], fill=GOG)
        d.point((x + sgn, y - 1), fill=GOG_L)
        d.line([(x + sgn, y + 2), (x + sgn * 3, y + 2)], fill=STASH)
        d.point((x + sgn * 3, y + 1), fill=SKIN_D)

def hat(d, c, tail='right', up=False):
    x, y = c
    for dy, (x0, x1) in {-6: (-1, 1), -5: (-2, 2), -4: (-3, 3), -3: (-3, 3)}.items():
        d.line([(x + x0, y + dy), (x + x1, y + dy)], fill=HAT)
    d.line([(x - 3, y - 3), (x + 3, y - 3)], fill=HAT_D)        # band
    d.line([(x - 1, y - 5), (x, y - 5)], fill=HAT_L); d.point((x - 2, y - 4), fill=HAT_L)
    if tail in ('left', 'right'):
        sgn = 1 if tail == 'right' else -1
        pts = [(x + sgn * 2, y - 6), (x + sgn * 5, y - 7), (x + sgn * 7, y - 5), (x + sgn * 7, y - 3)]
        for p0, p1 in zip(pts, pts[1:]): limb(d, p0, p1, 1, HAT, HAT_D)
        d.rectangle([x + sgn * 7 - 1, y - 3, x + sgn * 7, y - 2], fill=POMP)
        d.point((x + sgn * 7 - 1, y - 3), fill=GOG_L)
    elif tail == 'up':
        limb(d, (x, y - 7), (x + 2, y - 9), 1, HAT, HAT_D)
        d.rectangle([x + 1, y - 11, x + 3, y - 9], fill=POMP)

def snowline(d, x0, x1, y):
    d.line([(x0, y), (x1, y)], fill=SNOW_SH)

def mirror(im):
    return im.transpose(Image.FLIP_LEFT_RIGHT)

# ======================= skier (ids 1-22) =======================
def _flat_skis(d, y_back, y_front, x0, x1):
    ski(d, (x0 + 3, y_back), (x1 - 1, y_back)); ski(d, (x0, y_front), (x1 - 3, y_front))

def sk1():
    im, d = new(16, 32)
    pole(d, (2, 17), (1, 30)); pole(d, (13, 17), (14, 30))
    limb(d, (5, 27), (5, 31), 2, SKI, SKI_EDGE, SKI_L); limb(d, (10, 27), (10, 31), 2, SKI, SKI_EDGE, SKI_L)
    leg(d, (6, 19), (6, 23), (5, 26)); leg(d, (10, 19), (10, 23), (10, 26))
    torso(d, (8, 12), (8, 18), 7)
    arm(d, (4, 12), (2, 15), (2, 17)); arm(d, (12, 12), (14, 15), (13, 17))
    head(d, (8, 8)); hat(d, (8, 8), 'right')
    return im

def sk2():
    im, d = new(16, 32)
    pole(d, (3, 17), (0, 28))
    ski(d, (11, 25), (3, 31)); ski(d, (14, 26), (7, 31))
    leg(d, (7, 19), (6, 23), (5, 26)); leg(d, (10, 19), (10, 23), (9, 26))
    torso(d, (9, 12), (8, 18), 6)
    arm(d, (5, 13), (3, 15), (3, 17))
    arm(d, (12, 13), (14, 15), (12, 18))
    head(d, (9, 8), 'front'); hat(d, (9, 8), 'right')
    return im

def sk3():
    im, d = new(24, 28)
    pole(d, (21, 13), (22, 26))
    ski(d, (18, 24), (2, 27)); ski(d, (17, 26), (4, 28))
    leg(d, (11, 19), (10, 22), (8, 25)); leg(d, (14, 19), (13, 22), (12, 25))
    arm(d, (17, 12), (19, 14), (20, 13))
    torso(d, (13, 11), (12, 18), 6)
    arm(d, (10, 12), (8, 15), (7, 17)); pole(d, (7, 17), (3, 26))
    head(d, (13, 7), 'front'); hat(d, (13, 7), 'right')
    return im

def sk4():   # side view, facing left, standing
    im, d = new(24, 28)
    _flat_skis(d, 24, 26, 1, 23)
    pole(d, (19, 18), (21, 25))
    arm(d, (16, 13), (18, 15), (19, 18))
    leg(d, (15, 19), (16, 22), (17, 25)); leg(d, (13, 19), (13, 22), (12, 25))
    torso(d, (14, 12), (14, 19), 5, front=False)
    arm(d, (13, 13), (10, 15), (8, 17)); pole(d, (8, 17), (6, 25))
    head(d, (13, 8), 'left'); hat(d, (13, 8), 'right')
    return im

def sk7():
    return mirror(sk4())

def _side_right(back_leg, front_leg, pole_pos, arm_pose):
    im, d = new(24, 28)
    ski(d, (back_leg[2][0] - 4, back_leg[2][1] + 1), (back_leg[2][0] + 8, back_leg[2][1] + 1))
    ski(d, (front_leg[2][0] - 7, front_leg[2][1] + 1), (front_leg[2][0] + 6, front_leg[2][1] + 1))
    pole(d, arm_pose[2], pole_pos)
    leg(d, back_leg[0], back_leg[1], back_leg[2]); leg(d, front_leg[0], front_leg[1], front_leg[2])
    torso(d, (10, 12), (10, 19), 5, front=False)
    arm(d, (11, 13), arm_pose[0], arm_pose[2])
    head(d, (11, 8), 'right'); hat(d, (11, 8), 'left')
    return im

def sk8():
    return _side_right(((9, 20), (8, 23), (6, 25)), ((11, 20), (13, 23), (14, 25)), (22, 25), ((14, 14), (16, 16), (18, 18)))
def sk9():
    return _side_right(((10, 20), (10, 23), (9, 25)), ((11, 20), (14, 22), (16, 24)), (20, 25), ((14, 15), (16, 17), (17, 19)))
def sk10():
    return _side_right(((9, 20), (9, 23), (8, 25)), ((11, 20), (12, 23), (13, 25)), (7, 25), ((8, 15), (6, 17), (7, 19)))
def sk11():
    return _side_right(((10, 20), (9, 23), (8, 25)), ((11, 20), (13, 23), (13, 25)), (5, 25), ((8, 15), (6, 17), (6, 19)))

def sk12():  # crash: ouch
    im, d = new(32, 32)
    import random
    r = random.Random(12)
    for _ in range(26):
        x, y = r.randint(3, 30), r.randint(9, 30)
        d.point((x, y), fill=(206, 214, 232) if r.random() < .7 else SNOW_SH2)
    for (x, y) in [(5, 3), (9, 6), (3, 8), (22, 5), (26, 8)]:
        d.point((x, y), fill=GOG); d.point((x + 1, y), fill=GOG_L)
    text(d, 3, 1, 'OUCH!', OUT, tight=6)
    ski(d, (8, 18), (13, 27)); ski(d, (24, 15), (18, 25))
    for dy, (x0, x1) in {11: (4, 9), 12: (3, 10), 13: (2, 10), 14: (2, 9)}.items(): d.line([(x0, dy), (x1, dy)], fill=HAT)
    d.line([(3, 12), (7, 12)], fill=HAT_L); d.line([(2, 14), (9, 14)], fill=HAT_D)
    d.rectangle([10, 12, 12, 14], fill=POMP)
    pole(d, (1, 25), (9, 25), basket=False); d.line([(1, 24), (1, 26)], fill=POLE)
    pole(d, (20, 27), (30, 28), basket=False)
    return im

def sk13():  # air: skis up at the sides
    im, d = new(32, 24)
    ski(d, (8, 3), (11, 22), 2); ski(d, (24, 3), (21, 22), 2)
    pole(d, (4, 12), (3, 21)); pole(d, (28, 12), (29, 21))
    leg(d, (14, 16), (13, 19), (12, 21)); leg(d, (18, 16), (19, 19), (20, 21))
    torso(d, (16, 11), (16, 16), 7)
    arm(d, (12, 12), (8, 12), (5, 12)); arm(d, (20, 12), (24, 12), (27, 12))
    head(d, (16, 7), 'front'); hat(d, (16, 7), 'right')
    return im

def sk14():  # star jump
    im, d = new(32, 32)
    pole(d, (3, 13), (1, 22)); pole(d, (29, 11), (31, 4))
    ski(d, (5, 30), (12, 26)); ski(d, (27, 30), (20, 26))
    leg(d, (14, 19), (11, 23), (9, 27)); leg(d, (18, 19), (21, 23), (23, 27))
    torso(d, (16, 12), (16, 19), 6)
    arm(d, (13, 13), (9, 13), (5, 12)); arm(d, (19, 13), (24, 10), (28, 9))
    head(d, (16, 8), 'front'); hat(d, (16, 8), 'right')
    return im

def _stand_arms_up(w=28, h=31, yell=False, spread=0):
    im, d = new(w, h)
    cx = w // 2
    pole(d, (cx - 10 - spread, 8), (cx - 12 - spread, 27)); pole(d, (cx + 10 + spread, 8), (cx + 12 + spread, 27))
    ski(d, (cx - 8, h - 2), (cx + 8, h - 2))
    leg(d, (cx - 2, 20), (cx - 2, 24), (cx - 3, 27)); leg(d, (cx + 2, 20), (cx + 2, 24), (cx + 3, 27))
    torso(d, (cx, 13), (cx, 19), 6)
    arm(d, (cx - 3, 13), (cx - 7 - spread, 11), (cx - 10 - spread, 8)); arm(d, (cx + 3, 13), (cx + 7 + spread, 11), (cx + 10 + spread, 8))
    head(d, (cx, 8), 'front'); hat(d, (cx, 8), 'right')
    if yell: d.rectangle([cx - 1, 10, cx + 1, 11], fill=(150, 20, 30))
    return im

def sk15(): return _stand_arms_up()
def sk16(): return _stand_arms_up(yell=True, spread=1)

def sk17():  # flying, skis pointing at us
    im, d = new(28, 34)
    pole(d, (2, 10), (2, 28)); pole(d, (26, 10), (26, 28))
    leg(d, (12, 19), (12, 23), (12, 27)); leg(d, (16, 19), (16, 23), (16, 27))
    torso(d, (14, 12), (14, 19), 6)
    arm(d, (11, 13), (6, 12), (3, 10)); arm(d, (17, 13), (22, 12), (25, 10))
    head(d, (14, 8), 'front'); hat(d, (14, 8), 'up')
    ski(d, (11, 19), (11, 33), 2); ski(d, (17, 19), (17, 33), 2)
    return im

def sk18():  # tumble, skis crossed
    im, d = new(32, 26)
    pole(d, (3, 18), (1, 25)); pole(d, (29, 18), (30, 25))
    arm(d, (12, 17), (7, 17), (3, 18)); arm(d, (20, 17), (25, 17), (29, 18))
    torso(d, (16, 15), (16, 10), 6)
    ski(d, (3, 2), (21, 11), 2); ski(d, (11, 1), (26, 11), 2)
    head(d, (16, 19), 'front'); hat(d, (16, 22), 'right')
    return im

def sk19():  # upside down, skis up
    im, d = new(32, 32)
    pole(d, (3, 22), (3, 8)); pole(d, (29, 22), (29, 8))
    ski(d, (11, 1), (11, 18), 2); ski(d, (21, 1), (21, 18), 2)
    leg(d, (14, 11), (14, 15), (14, 18), with_boot=False); leg(d, (18, 11), (18, 15), (18, 18), with_boot=False)
    torso(d, (16, 22), (16, 15), 6)
    arm(d, (12, 21), (7, 21), (3, 22)); arm(d, (20, 21), (25, 21), (29, 22))
    head(d, (16, 25), 'front'); hat(d, (16, 28), 'right')
    return im

def sk20():  # handstand tumble, no skis
    im, d = new(31, 24)
    pole(d, (2, 19), (1, 23), basket=False); pole(d, (29, 19), (30, 23), basket=False)
    leg(d, (14, 10), (12, 6), (11, 2)); leg(d, (18, 10), (20, 6), (21, 2))
    torso(d, (16, 15), (16, 10), 6)
    arm(d, (12, 16), (7, 17), (3, 19)); arm(d, (20, 16), (25, 17), (28, 19))
    head(d, (16, 17), 'front'); hat(d, (16, 20), 'right')
    return im

def sk21():
    im, d = new(25, 31)
    pole(d, (2, 14), (2, 27)); pole(d, (21, 12), (23, 28))
    ski(d, (14, 27), (23, 30)); ski(d, (11, 28), (20, 31))
    leg(d, (11, 20), (13, 24), (15, 28)); leg(d, (14, 20), (17, 24), (19, 27))
    torso(d, (13, 13), (12, 20), 6)
    arm(d, (10, 14), (6, 15), (3, 16)); arm(d, (16, 13), (19, 12), (21, 12))
    head(d, (13, 8), 'front'); hat(d, (13, 8), 'right')
    return im
def sk22(): return mirror(sk21())

SKIER = {1: sk1, 2: sk2, 3: sk3, 4: sk4, 5: lambda: mirror(sk2()), 6: lambda: mirror(sk3()), 7: sk7,
         8: sk8, 9: sk9, 10: sk10, 11: sk11, 12: sk12, 13: sk13, 14: sk14, 15: sk15, 16: sk16, 17: sk17,
         18: sk18, 19: sk19, 20: sk20, 21: sk21, 22: sk22}



# ======================= yeti (ids 68-81), 32x48 =======================
Y_OUT, Y_MID, Y_LIT, Y_SH, Y_BELLY = (44, 46, 62), (152, 154, 168), (190, 192, 204), (106, 108, 126), (206, 208, 218)
Y_EYE, Y_TEETH, Y_MOUTH, Y_TONGUE = (238, 40, 40), (252, 218, 44), (58, 16, 26), (224, 70, 90)

def _hash(x, y):
    h = (x * 374761393 + y * 668265263) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return h ^ (h >> 16)

def y_body(d, cx, cy, rx, ry, belly=True, bump=0):
    for y in range(cy - ry, cy + ry + 1):
        for x in range(cx - rx, cx + rx + 1):
            nx, ny = (x - cx) / rx, (y - cy) / ry
            r2 = nx * nx + ny * ny
            if r2 > 1.0: continue
            edge = r2 > 0.80 + 0.06 * ((_hash(x, y) % 3) - 1)      # ragged fur edge
            if edge: c = Y_OUT
            else:
                c = Y_LIT if (nx < -0.25 and ny < 0.4) else (Y_SH if nx > 0.35 else Y_MID)
                if belly and ((nx + 0.05) ** 2 / 0.32 + (ny - 0.2 - bump * 0.1) ** 2 / 0.38) < 1 and nx > -0.45: c = Y_BELLY if nx < 0.3 else Y_LIT
                h = _hash(x, y) % 11
                if h == 0: c = Y_LIT
                elif h == 1 and c != Y_SH: c = Y_SH
            d.point((x, y), fill=c)

def y_claws(d, p, direction):
    x, y = p
    dx, dy = direction
    for k in (-1, 0, 1):
        ex, ey = x + dx * 3 + (k if dx == 0 else 0), y + dy * 3 + (k if dy == 0 else 0)
        d.line([(x, y), (ex, ey)], fill=Y_OUT)

def y_limb(d, a, b, w=3):
    d.line([a, b], fill=Y_OUT, width=w + 2)
    d.line([a, b], fill=Y_MID, width=w)
    d.line([(a[0] - 1, a[1]), (b[0] - 1, b[1])], fill=Y_LIT, width=1)

def y_head(d, cx, y0, mode='front'):
    # horns
    for sgn in (-1, 1):
        hx = cx + sgn * 6
        d.polygon([(hx, y0 + 2), (hx + sgn * 2, y0 - 2), (hx + sgn * 1, y0 + 4)], fill=Y_LIT, outline=Y_OUT)
    # head block with softened corners
    for dy in range(0, 12):
        x0, x1 = (-5, 5) if dy in (0, 11) else (-6, 6)
        for x in range(x0, x1 + 1):
            c = Y_OUT if (dy in (0, 11) or x in (x0, x1)) else (Y_LIT if x < -2 and dy < 6 else (Y_SH if x > 3 else Y_MID))
            d.point((cx + x, y0 + dy), fill=c)
    if mode == 'back':
        d.line([(cx - 3, y0 + 3), (cx + 3, y0 + 3)], fill=Y_SH); d.line([(cx - 2, y0 + 6), (cx + 2, y0 + 6)], fill=Y_SH)
        return
    # brows + eyes
    d.line([(cx - 5, y0 + 3), (cx - 2, y0 + 4)], fill=Y_OUT); d.line([(cx + 5, y0 + 3), (cx + 2, y0 + 4)], fill=Y_OUT)
    d.rectangle([cx - 4, y0 + 5, cx - 3, y0 + 6], fill=Y_EYE); d.rectangle([cx + 2, y0 + 5, cx + 3, y0 + 6], fill=Y_EYE)
    d.point((cx - 4, y0 + 5), fill=(255, 170, 160)); d.point((cx + 2, y0 + 5), fill=(255, 170, 160))
    if mode in ('front', 'roar', 'closed'):
        top = y0 + 8
        h = 4 if mode == 'roar' else 3
        d.rectangle([cx - 5, top, cx + 5, top + h - 1], fill=Y_MOUTH)
        if mode == 'roar': d.rectangle([cx - 3, top + 2, cx + 3, top + 3], fill=Y_TONGUE)
        for i in range(4): d.rectangle([cx - 5 + i * 3, top, cx - 4 + i * 3, top + 1], fill=Y_TEETH)
    elif mode == 'wide':   # mouth gaping for a meal
        d.rectangle([cx - 5, y0 + 8, cx + 5, y0 + 11], fill=Y_MOUTH)
        for i in range(4): d.rectangle([cx - 5 + i * 3, y0 + 8, cx - 4 + i * 3, y0 + 9], fill=Y_TEETH)

def yeti(arm_l, arm_r, leg_l, leg_r, mode='front', cx=16, extra=None, belly_bump=0, back=False):
    im, d = new(32, 48)
    # legs first, then body, head, arms
    for (knee, foot), hip in ((leg_l, (cx - 5, 37)), (leg_r, (cx + 5, 37))):
        y_limb(d, hip, knee); y_limb(d, knee, foot)
        fx, fy = foot
        for k in (-2, 0, 2): d.line([(fx, fy), (fx + k, fy + 2)], fill=Y_OUT)
    y_body(d, cx, 27, 11, 12, belly=not back, bump=belly_bump)
    y_head(d, cx, 4, 'back' if back else mode)
    for (el, hand), sh, side in ((arm_l, (cx - 10, 20), -1), (arm_r, (cx + 10, 20), 1)):
        y_limb(d, sh, el, 3); y_limb(d, el, hand, 2)
        x, y = hand
        vx, vy = hand[0] - el[0], hand[1] - el[1]
        L = max(1.0, (vx * vx + vy * vy) ** 0.5)
        ux, uy = vx / L, vy / L
        for a in (-0.6, 0.0, 0.6):
            ca, sa = __import__('math').cos(a), __import__('math').sin(a)
            ex, ey = x + (ux * ca - uy * sa) * 4, y + (ux * sa + uy * ca) * 4
            d.line([(x, y), (int(ex), int(ey))], fill=Y_OUT)
    if extra: extra(d)
    return im

def _eat_skier(stage):
    """a little skier in the yeti's grip (stage 0 held up, 1 tipped toward the mouth, 2 going in)."""
    def f(d):
        if stage == 0:
            limb(d, (22, 12), (26, 24), 3, PAN, PAN_D, PAN_L); boot(d, (26, 25)); limb(d, (25, 12), (29, 22), 3, PAN, PAN_D, PAN_L); boot(d, (29, 23))
            torso(d, (24, 9), (24, 13), 5, front=False)
            d.rectangle([22, 2, 27, 5], fill=HAT); d.rectangle([27, 1, 29, 3], fill=POMP)
            d.rectangle([22, 5, 26, 8], fill=SKIN)
            limb(d, (21, 10), (17, 12), 2, JAC, JAC_D, JAC_L)
        elif stage == 1:
            limb(d, (20, 16), (28, 27), 3, PAN, PAN_D, PAN_L); boot(d, (28, 28)); limb(d, (22, 14), (30, 22), 3, PAN, PAN_D, PAN_L); boot(d, (30, 23))
            torso(d, (20, 13), (23, 17), 5, front=False)
            d.rectangle([16, 8, 21, 11], fill=HAT); d.rectangle([20, 6, 23, 8], fill=POMP)
            limb(d, (19, 12), (14, 14), 2, JAC, JAC_D, JAC_L)
        else:
            limb(d, (19, 12), (27, 20), 3, PAN, PAN_D, PAN_L); boot(d, (27, 21)); limb(d, (21, 11), (30, 16), 3, PAN, PAN_D, PAN_L); boot(d, (30, 17))
            d.rectangle([17, 11, 21, 14], fill=JAC)
    return f

def _swallowed(d):
    limb(d, (14, 5), (13, -1), 3, PAN, PAN_D, PAN_L)
    limb(d, (19, 5), (20, -1), 3, PAN, PAN_D, PAN_L)
    boot(d, (13, 0)); boot(d, (20, 0))

def _pole(d):
    d.line([(26, 26), (22, 13)], fill=POLE, width=1)
    d.point((22, 12), fill=POLE)

YETI = {
    68: lambda: yeti(((4, 16), (2, 7)), ((28, 15), (30, 8)), ((9, 42), (6, 46)), ((23, 42), (26, 46))),
    69: lambda: yeti(((5, 15), (6, 6)), ((27, 15), (26, 7)), ((11, 43), (9, 46)), ((21, 43), (23, 46)), mode='roar'),
    70: lambda: yeti(((3, 26), (1, 32)), ((29, 22), (30, 27)), ((10, 42), (6, 45)), ((23, 41), (28, 39))),
    71: lambda: yeti(((4, 23), (1, 27)), ((29, 27), (30, 34)), ((11, 43), (8, 46)), ((22, 42), (27, 45))),
    72: lambda: yeti(((3, 27), (1, 32)), ((29, 27), (31, 32)), ((7, 42), (3, 45)), ((25, 42), (29, 45))),
    73: lambda: yeti(((4, 26), (2, 34)), ((29, 22), (31, 27)), ((11, 43), (9, 46)), ((21, 43), (23, 46))),
    74: lambda: yeti(((4, 16), (3, 6)), ((28, 16), (29, 7)), ((9, 42), (6, 45)), ((23, 42), (26, 45)), back=True),
    75: lambda: yeti(((4, 28), (2, 35)), ((28, 16), (29, 6)), ((10, 42), (6, 45)), ((22, 43), (24, 46)), back=True),
    76: lambda: yeti(((3, 27), (1, 33)), ((25, 21), (21, 14)), ((10, 43), (7, 46)), ((22, 43), (25, 46)), mode='wide', extra=_eat_skier(0)),
    77: lambda: yeti(((3, 27), (1, 33)), ((25, 21), (21, 15)), ((10, 43), (7, 46)), ((22, 43), (25, 46)), mode='wide', extra=_eat_skier(1)),
    78: lambda: yeti(((3, 27), (1, 33)), ((25, 21), (21, 14)), ((10, 43), (7, 46)), ((22, 43), (25, 46)), mode='wide', extra=_eat_skier(2)),
    79: lambda: yeti(((4, 27), (2, 33)), ((28, 27), (30, 33)), ((10, 43), (7, 46)), ((22, 43), (25, 46)), mode='closed', extra=_swallowed),
    80: lambda: yeti(((4, 28), (2, 34)), ((27, 25), (23, 17)), ((10, 43), (7, 46)), ((22, 43), (25, 46)), mode='closed', extra=_pole, belly_bump=1),
    81: lambda: yeti(((4, 28), (2, 34)), ((27, 25), (22, 14)), ((10, 43), (7, 46)), ((22, 43), (25, 46)), mode='closed', extra=_pole, belly_bump=1),
}



# ======================= rival skiers (28-32) =======================
B_FACE, B_FACE_D = (150, 232, 242), (84, 176, 196)
B_CAP, B_CAP_L, B_CAP_D = (226, 40, 44), (252, 120, 110), (140, 14, 28)
B_JAC, B_JAC_L, B_JAC_D = (244, 112, 204), (255, 178, 232), (168, 44, 140)

def b_head(d, c, crown=False):
    x, y = c
    for dy, (x0, x1) in {-2: (-2, 2), -1: (-3, 3), 0: (-3, 3), 1: (-3, 3), 2: (-2, 2)}.items():
        d.line([(x + x0, y + dy), (x + x1, y + dy)], fill=B_FACE)
        d.point((x + x1, y + dy), fill=B_FACE_D)
    d.point((x - 2, y - 1), fill=OUT); d.point((x + 1, y - 1), fill=OUT)         # eyes
    d.line([(x - 1, y + 1), (x + 1, y + 1)], fill=(190, 40, 70))                  # mouth
    for dy, (x0, x1) in {-6: (-1, 1), -5: (-2, 2), -4: (-3, 3), -3: (-3, 3)}.items():
        d.line([(x + x0, y + dy), (x + x1, y + dy)], fill=B_CAP)
    d.line([(x - 3, y - 3), (x + 3, y - 3)], fill=B_CAP_D); d.point((x - 1, y - 5), fill=B_CAP_L)
    d.rectangle([x, y - 8, x + 1, y - 7], fill=(250, 250, 120))                   # cap tuft

def b_body(d, sh, hip, w=6):
    limb(d, sh, hip, w, B_JAC, B_JAC_D, B_JAC_L)
    limb(d, (hip[0], hip[1] - 1), (hip[0], hip[1] + 1), w - 1, PAN, PAN_D, PAN_L)

def snowplow(w, h, poles, bob=0):
    im, d = new(w, h)
    cx = w // 2
    for (a, bb) in poles: pole(d, a, bb)
    leg(d, (cx - 2, h - 13), (cx - 3, h - 9), (cx - 4, h - 6), with_boot=False)
    leg(d, (cx + 2, h - 13), (cx + 3, h - 9), (cx + 4, h - 6), with_boot=False)
    ski(d, (cx, h - 2), (1, h - 14), 2); ski(d, (cx, h - 2), (w - 2, h - 14), 2)     # the V, in front of the legs
    b_body(d, (cx, 12 + bob), (cx, h - 14), 6)
    limb(d, (cx - 3, 12 + bob), (cx - 7, 16 + bob), 2, B_JAC, B_JAC_D, B_JAC_L); glove(d, (cx - 7, 17 + bob))
    limb(d, (cx + 3, 12 + bob), (cx + 7, 16 + bob), 2, B_JAC, B_JAC_D, B_JAC_L); glove(d, (cx + 7, 17 + bob))
    b_head(d, (cx, 7 + bob))
    return im

def sk28(): return snowplow(24, 30, [((19, 17), (22, 3)), ((6, 17), (3, 27))])
def sk29(): return snowplow(22, 29, [((18, 17), (20, 4)), ((5, 17), (2, 27))], 0)
def sk30(): return snowplow(21, 29, [((16, 17), (18, 3)), ((4, 17), (3, 26))], 1)

def sk31():   # dizzy after a crash: stars, hat knocked off
    im, d = new(24, 24)
    ski(d, (3, 17), (12, 22)); ski(d, (12, 22), (21, 15)); ski(d, (4, 15), (10, 19)); ski(d, (13, 19), (20, 13))
    b_body(d, (12, 12), (12, 16), 7)
    limb(d, (9, 11), (5, 8), 2, B_JAC, B_JAC_D, B_JAC_L); glove(d, (4, 7)); limb(d, (15, 11), (19, 8), 2, B_JAC, B_JAC_D, B_JAC_L); glove(d, (20, 7))
    for dy, (x0, x1) in {-2: (-2, 2), -1: (-3, 3), 0: (-3, 3), 1: (-3, 3), 2: (-2, 2)}.items():
        d.line([(12 + x0, 7 + dy), (12 + x1, 7 + dy)], fill=B_FACE)
    d.line([(10, 6), (11, 7)], fill=OUT); d.line([(11, 6), (10, 7)], fill=OUT); d.line([(13, 6), (14, 7)], fill=OUT); d.line([(14, 6), (13, 7)], fill=OUT)
    for (x, y) in [(8, 1), (12, 0), (16, 1)]:
        d.point((x, y), fill=SKI); d.point((x - 1, y), fill=SKI_D); d.point((x + 1, y), fill=SKI_D); d.point((x, y - 1), fill=SKI_D); d.point((x, y + 1), fill=SKI_D)
    d.rectangle([1, 11, 5, 14], fill=B_CAP, outline=B_CAP_D)
    return im

def sk32():   # tumbling upside down
    im, d = new(24, 24)
    ski(d, (5, 1), (9, 13), 2); ski(d, (18, 2), (14, 13), 2)
    leg(d, (10, 13), (10, 10), (9, 8), with_boot=False); leg(d, (14, 13), (14, 10), (15, 8), with_boot=False)
    b_body(d, (12, 16), (12, 12), 6)
    limb(d, (9, 17), (4, 18), 2, B_JAC, B_JAC_D, B_JAC_L); glove(d, (3, 19)); limb(d, (15, 17), (20, 18), 2, B_JAC, B_JAC_D, B_JAC_L); glove(d, (21, 19))
    for dy, (x0, x1) in {-2: (-2, 2), -1: (-3, 3), 0: (-3, 3), 1: (-3, 3), 2: (-2, 2)}.items():
        d.line([(12 + x0, 20 + dy), (12 + x1, 20 + dy)], fill=B_FACE)
    d.point((10, 19), fill=OUT); d.point((13, 19), fill=OUT)
    for (x, y) in [(6, 22), (18, 22)]: d.point((x, y), fill=SNOW_SH2)
    return im

# ======================= dogs (33-36) =======================
D_MID, D_LIT, D_SH, D_OUT = (118, 120, 134), (160, 162, 176), (82, 84, 100), (34, 36, 48)

def dog_walk(frame):
    im, d = new(21, 15)
    legs = [((5, 10), (4, 14)), ((8, 10), (9, 14)), ((13, 10), (12, 14)), ((16, 10), (17, 14))] if frame == 0 else \
           [((5, 10), (6, 14)), ((8, 10), (7, 14)), ((13, 10), (14, 14)), ((16, 10), (15, 14))]
    for a, b in legs: limb(d, a, b, 1, D_SH, D_OUT)
    d.ellipse([3, 4, 16, 11], fill=D_MID, outline=D_OUT)
    d.line([(5, 5), (13, 5)], fill=D_LIT)
    d.ellipse([13, 2, 19, 8], fill=D_MID, outline=D_OUT)
    d.polygon([(14, 3), (15, 0), (17, 3)], fill=D_SH, outline=D_OUT)
    d.rectangle([18, 5, 20, 7], fill=D_MID, outline=D_OUT); d.point((20, 5), fill=OUT)
    d.point((16, 4), fill=(240, 240, 255))
    d.line([(19, 8), (19, 10)], fill=(240, 100, 130)); d.point((19, 11), fill=(240, 100, 130))
    d.line([(3, 6), (1, 3)], fill=D_OUT, width=1); d.line([(4, 6), (2, 3)], fill=D_MID, width=1)
    return im

WOOF = ['10001 111 111 111 1', '10001 101 101 100 1', '10101 101 101 110 1', '10101 101 101 100 0', '01010 111 111 100 1']

def dog_sit(frame):
    im, d = new(19, 19)
    for ry, row in enumerate(WOOF):
        for rx, ch in enumerate(row):
            if ch == '1': d.point((rx, ry), fill=(30, 44, 120))
    d.ellipse([1, 12, 10, 18], fill=D_MID, outline=D_OUT)                      # haunch
    d.rectangle([8, 10, 12, 18], fill=D_MID, outline=D_OUT)                    # upright chest
    d.line([(9, 11), (9, 17)], fill=D_LIT)
    d.rectangle([11, 15, 13, 18], fill=D_SH, outline=D_OUT)                    # front paw
    d.ellipse([8, 5, 15, 12], fill=D_MID, outline=D_OUT)                       # head
    d.polygon([(8, 6), (9, 3), (11, 6)], fill=D_SH, outline=D_OUT)
    d.rectangle([14, 8, 17, 10], fill=D_MID, outline=D_OUT); d.point((17, 8), fill=OUT)
    d.point((12, 7), fill=(240, 240, 255))
    d.line([(16, 11), (16, 13)], fill=(240, 100, 130)); d.point((16, 14), fill=(240, 100, 130))
    tail = [(2, 16), (0, 14), (0, 12)] if frame == 0 else [(2, 16), (0, 17), (1, 18)]
    for a_, b_ in zip(tail, tail[1:]): limb(d, a_, b_, 1, D_MID, D_OUT)
    return im

# ======================= snowboarders (37-44) =======================
BD, BD_L, BD_D = (44, 96, 232), (120, 168, 255), (22, 52, 160)
S_JAC, S_JAC_L, S_JAC_D = (70, 200, 84), (140, 240, 150), (30, 120, 44)
S_SLV = (60, 214, 230)
S_PAN, S_PAN_L = (232, 70, 200), (255, 150, 232)
S_FACE, S_CAP = (214, 188, 70), (226, 40, 44)

def board(d, a, b, w=4):
    limb(d, a, b, w, BD, BD_D, BD_L)
    d.point((int(a[0]), int(a[1])), fill=BD_D); d.point((int(b[0]), int(b[1])), fill=BD_D)

def rider_head(d, c, brim='right'):
    x, y = c
    for dy, (x0, x1) in {-2: (-2, 2), -1: (-3, 3), 0: (-3, 3), 1: (-3, 3), 2: (-2, 2)}.items():
        d.line([(x + x0, y + dy), (x + x1, y + dy)], fill=S_FACE)
    d.point((x + 1, y), fill=OUT); d.point((x - 2, y), fill=OUT)
    d.rectangle([x - 3, y - 4, x + 3, y - 2], fill=S_CAP)
    d.line([(x - 3, y - 2), (x + (5 if brim == 'right' else -5), y - 2)], fill=(120, 14, 24))

def rider(d, sh, hip, knees_feet, arms, head_c, brim='right'):
    for kn, ft in knees_feet:
        limb(d, hip, kn, 3, S_PAN, (150, 30, 130), S_PAN_L); limb(d, kn, ft, 3, S_PAN, (150, 30, 130), S_PAN_L)
        d.rectangle([ft[0] - 1, ft[1] - 1, ft[0] + 1, ft[1] + 1], fill=(130, 132, 146))
    limb(d, sh, hip, 6, S_JAC, S_JAC_D, S_JAC_L)
    for el, hd in arms:
        limb(d, sh, el, 2, S_SLV, (20, 130, 150), (170, 250, 255)); limb(d, el, hd, 2, S_SLV, (20, 130, 150), (170, 250, 255))
        d.rectangle([hd[0] - 1, hd[1] - 1, hd[0] + 1, hd[1] + 1], fill=S_FACE)
    rider_head(d, head_c, brim)

def sb37():
    im, d = new(26, 30)
    board(d, (22, 6), (9, 28), 5)
    rider(d, (13, 11), (12, 18), [((11, 21), (10, 25)), ((15, 21), (14, 24))], [((6, 10), (3, 6)), ((20, 12), (23, 8))], (13, 6))
    return im
def sb38():
    im, d = new(20, 30)
    board(d, (14, 5), (8, 28), 5)
    rider(d, (10, 11), (10, 18), [((9, 21), (8, 25)), ((12, 21), (11, 24))], [((4, 12), (3, 8)), ((15, 12), (16, 9))], (10, 6), 'left')
    return im
def sb39():
    im, d = new(25, 31)
    board(d, (15, 16), (18, 29), 5)
    rider(d, (11, 12), (12, 19), [((13, 22), (15, 26)), ((10, 22), (11, 27))], [((5, 9), (3, 4)), ((18, 10), (21, 5))], (11, 7))
    return im
def sb40():
    im, d = new(30, 29)
    board(d, (4, 20), (19, 6), 5)
    rider(d, (22, 13), (20, 19), [((18, 22), (15, 26)), ((22, 23), (23, 27))], [((16, 12), (12, 8)), ((26, 10), (28, 5))], (22, 8))
    return im
def sb41():
    im, d = new(32, 32)
    board(d, (4, 24), (20, 6), 5)
    rider(d, (20, 21), (14, 24), [((19, 27), (24, 30)), ((12, 29), (10, 31))], [((24, 17), (28, 13)), ((15, 20), (11, 17))], (24, 17), 'left')
    return im
def sb42():
    im, d = new(32, 32)
    board(d, (4, 9), (28, 5), 5)
    rider(d, (16, 19), (16, 13), [((13, 10), (11, 7)), ((19, 10), (21, 7))], [((11, 21), (7, 24)), ((21, 21), (25, 24))], (16, 25))
    return im
def sb43():
    im, d = new(25, 29)
    board(d, (14, 3), (6, 17), 5)
    rider(d, (13, 18), (12, 12), [((16, 14), (20, 11)), ((10, 14), (7, 11))], [((18, 20), (21, 24)), ((8, 20), (5, 23))], (13, 24))
    return im
def sb44():
    im, d = new(29, 25)
    board(d, (14, 18), (27, 11), 5)
    rider(d, (8, 12), (14, 15), [((17, 17), (20, 14)), ((16, 19), (21, 19))], [((5, 17), (2, 20)), ((5, 8), (2, 5))], (5, 12), 'left')
    return im

CHARS = {28: sk28, 29: sk29, 30: sk30, 31: sk31, 32: sk32,
         33: lambda: dog_walk(0), 34: lambda: dog_walk(1), 35: lambda: dog_sit(0), 36: lambda: dog_sit(1),
         37: sb37, 38: sb38, 39: sb39, 40: sb40, 41: sb41, 42: sb42, 43: sb43, 44: sb44}

BATCH = {}
BATCH.update(SKIER)
BATCH.update(YETI)
BATCH.update(CHARS)
