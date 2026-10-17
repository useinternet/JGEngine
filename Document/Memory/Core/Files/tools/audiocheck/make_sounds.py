# AudioCheck verification game sounds (Audio module, D-37). Python 3 standard library only.
# Usage: python make_sounds.py <game Content/Sounds folder>
# Writes Effect.wav (0.2 s 880 Hz blip), MusicA.wav (6 s arpeggio loop, 44.1 kHz stereo) and Rain.wav (4 s soft noise loop).
# MusicB.ogg is copied from a file already on this PC (see README.md) and is never committed.
import math
import os
import random
import struct
import sys
import wave

RATE = 44100


def write_wav(path, frames):
    with wave.open(path, "wb") as out:
        out.setnchannels(2)
        out.setsampwidth(2)
        out.setframerate(RATE)
        data = bytearray()
        for left, right in frames:
            data += struct.pack("<hh", int(max(-1.0, min(1.0, left)) * 32767), int(max(-1.0, min(1.0, right)) * 32767))
        out.writeframes(bytes(data))


def effect():
    # Short blip: 880 Hz, 5 ms attack, exponential decay, 0.2 s. Loud enough to measure on the output.
    total = int(0.2 * RATE)
    frames = []
    for i in range(total):
        t = i / RATE
        envelope = min(1.0, t / 0.005) * math.exp(-t * 18.0)
        value = 0.5 * envelope * math.sin(2.0 * math.pi * 880.0 * t)
        frames.append((value, value))
    return frames


def music_a():
    # C major arpeggio, 8 notes per 2 s, three bars = 6 s. Each note has a short attack and decay so the loop point is clean.
    notes = [261.63, 329.63, 392.00, 523.25, 392.00, 329.63, 261.63, 196.00]
    note_seconds = 0.25
    total = int(6.0 * RATE)
    frames = []
    for i in range(total):
        t = i / RATE
        index = int(t / note_seconds) % len(notes)
        local = t - int(t / note_seconds) * note_seconds
        envelope = min(1.0, local / 0.01) * math.exp(-local * 6.0)
        value = 0.30 * envelope * math.sin(2.0 * math.pi * notes[index] * t)
        frames.append((value, value))
    return frames


def rain():
    # Low-passed white noise with a slow swell; left and right differ a little.
    random.seed(11)
    total = int(4.0 * RATE)
    frames = []
    left_state = 0.0
    right_state = 0.0
    for i in range(total):
        t = i / RATE
        swell = 0.75 + 0.25 * math.sin(2.0 * math.pi * t / 4.0)
        left_state += 0.25 * (random.uniform(-1.0, 1.0) - left_state)
        right_state += 0.25 * (random.uniform(-1.0, 1.0) - right_state)
        frames.append((0.35 * swell * left_state, 0.35 * swell * right_state))
    return frames


def main():
    folder = sys.argv[1]
    os.makedirs(folder, exist_ok=True)
    write_wav(os.path.join(folder, "Effect.wav"), effect())
    write_wav(os.path.join(folder, "MusicA.wav"), music_a())
    write_wav(os.path.join(folder, "Rain.wav"), rain())
    print("wrote Effect.wav, MusicA.wav and Rain.wav to " + folder)


if __name__ == "__main__":
    main()
