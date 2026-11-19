#!/usr/bin/env python3
"""Synthesizes the nine built-in SkiFree-port sounds (original to this port, MIT: see sf3500/LICENSE-ASSETS).
Usage: make_sounds.py <output-dir>     writes <name>.wav, 22.05 kHz mono 16-bit."""
import sys, wave
import numpy as np
from scipy import signal

SR = 22050
rng = np.random.default_rng(1991)

def t(sec): return np.arange(int(sec * SR)) / SR
def env_exp(n, tau): return np.exp(-np.arange(n) / (tau * SR))
def noise(n): return rng.uniform(-1, 1, n)
def lp(x, fc, order=2): return signal.lfilter(*signal.butter(order, fc / (SR / 2), 'low'), x)
def bp(x, lo, hi): return signal.lfilter(*signal.butter(2, [lo / (SR / 2), hi / (SR / 2)], 'band'), x)
def sweep(f0, f1, n, exp=True):
    f = np.geomspace(f0, f1, n) if exp else np.linspace(f0, f1, n)
    return np.cumsum(2 * np.pi * f / SR)
def attack(x, ms): 
    a = int(ms / 1000 * SR); x = x.copy(); x[:a] *= np.linspace(0, 1, a); return x
def place(buf, x, at):
    i = int(at * SR); buf[i:i + len(x)] += x[:len(buf) - i]

def crash():
    n = int(0.40 * SR)
    thump = np.sin(sweep(210, 60, n)) * env_exp(n, 0.10)
    body = lp(noise(n), 1400) * env_exp(n, 0.07) * 1.6
    click = np.zeros(n); click[:60] = noise(60) * np.linspace(1, 0, 60)
    return thump * 0.9 + body + click * 0.8

def jump():
    n = int(0.22 * SR)
    ph = sweep(260, 1000, n)
    x = (np.sin(ph) + 0.35 * np.sign(np.sin(ph))) * np.hanning(n * 2)[:n][::-1][::-1]
    x = x * np.minimum(1, np.arange(n) / (0.01 * SR)) * env_exp(n, 0.16)
    return x

def bark(f0, f1, dur):
    n = int(dur * SR)
    ph = sweep(f0, f1, n, exp=False)
    src = signal.sawtooth(ph) * 0.8 + noise(n) * 0.25
    x = bp(src, 500, 1000) * 2.2 + bp(src, 1100, 1900) * 1.3
    return attack(x * env_exp(n, dur / 3.2), 4)

def woof():
    buf = np.zeros(int(0.78 * SR))
    place(buf, bark(300, 190, 0.20), 0.02)
    place(buf, bark(270, 165, 0.22), 0.30)
    return buf

def land():
    n = int(0.16 * SR)
    return np.sin(sweep(120, 50, n)) * env_exp(n, 0.045) + lp(noise(n), 700) * env_exp(n, 0.02) * 1.4

def board():
    n = int(0.30 * SR)
    src = noise(n)
    out = np.zeros(n); low = band = 0.0
    fc = np.geomspace(500, 3200, n)
    for i in range(n):          # state-variable filter, cutoff sweeping upward
        f = 2 * np.sin(np.pi * fc[i] / SR)
        low += f * band
        high = src[i] - low - 0.35 * band
        band += f * high
        out[i] = band
    e = np.sin(np.linspace(0, np.pi, n)) ** 1.5
    crackle = (rng.random(n) > 0.995) * noise(n)
    return out * e * 0.55 + lp(crackle, 5000) * 1.5

def bump():
    n = int(0.26 * SR)
    ph = sweep(190, 85, n)
    x = lp(signal.square(ph) * 0.8 + np.sin(ph), 900)
    return attack(x * env_exp(n, 0.11), 3)

def gobble():
    buf = np.zeros(int(0.95 * SR))
    for k in range(3):
        n = int(0.14 * SR)
        chomp = bp(noise(n), 300, 1800) * env_exp(n, 0.04) * 2.0
        chomp += np.sin(sweep(150, 90, n)) * env_exp(n, 0.06)
        chomp[:30] += noise(30) * 0.8
        place(buf, chomp, 0.02 + k * 0.21)
    n = int(0.22 * SR)
    gulp = np.sin(sweep(340, 110, n) + 3 * np.sin(2 * np.pi * 14 * t(0.22)[:n])) * np.hanning(n)
    place(buf, gulp * 0.9, 0.68)
    return buf

def piddle():
    n = int(0.9 * SR)
    stream = bp(noise(n), 1800, 5200)
    am = lp(np.abs(noise(n)), 90, 1) * 6
    x = stream * (0.35 + np.clip(am, 0, 1)) * 0.9
    for _ in range(14):         # little bubbles
        d = int(rng.uniform(0.02, 0.05) * SR)
        f = rng.uniform(1400, 2600)
        b = np.sin(sweep(f, f * 1.6, d)) * env_exp(d, d / SR / 3)
        i = int(rng.uniform(0, 0.85) * SR); x[i:i + d] += b[:n - i] * 0.7
    return x * np.minimum(1, np.minimum(np.arange(n), n - np.arange(n)) / (0.06 * SR))

def roar():
    n = int(1.0 * SR)
    tt = t(1.0)[:n]
    f0 = np.linspace(95, 55, n) * (1 + 0.06 * np.sin(2 * np.pi * 22 * tt))
    src = signal.sawtooth(np.cumsum(2 * np.pi * f0 / SR)) + 0.3 * noise(n)
    x = lp(src, 1700) * (0.65 + 0.35 * np.sin(2 * np.pi * 31 * tt))
    x = np.tanh(2.4 * x)
    e = np.minimum(1, tt / 0.07) * np.minimum(1, (1.0 - tt) / 0.35)
    return x * e

TRIM = {'jump': 0.42, 'roar': 0.45, 'bump': 0.6, 'piddle': 0.8}   # near-square waves read louder than their peak

def write(path, x, trim=1.0):
    x = x - np.mean(x)
    x = x * (0.17 / max(1e-9, np.sqrt(np.mean(x ** 2))))   # even loudness across sounds
    x = np.tanh(x)                                         # soft limiter for the spiky ones
    x = x / max(1e-9, np.max(np.abs(x))) * 0.9 * trim
    fade = int(0.008 * SR); x[-fade:] *= np.linspace(1, 0, fade)
    with wave.open(path, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((x * 32767).astype('<i2').tobytes())

if __name__ == '__main__':
    out = sys.argv[1]
    for name, fn in [('crash', crash), ('jump', jump), ('woof', woof), ('land', land), ('board', board),
                     ('bump', bump), ('gobble', gobble), ('piddle', piddle), ('roar', roar)]:
        write(f'{out}/{name}.wav', fn(), TRIM.get(name, 1.0))
        print(name)
