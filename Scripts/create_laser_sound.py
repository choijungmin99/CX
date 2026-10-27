"""외부 음원 없이 짧은 에너지 발사음 원본을 만든다. 48kHz/16bit/mono."""
from array import array
import math
from pathlib import Path
import random
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'SourceAssets/Audio/LaserFire.wav'
SAMPLE_RATE = 48000
DURATION = 0.28


def create():
    rng = random.Random(7321)
    dry = []
    phase = 0.0
    low_noise = 0.0
    for index in range(round(SAMPLE_RATE * DURATION)):
        time = index / SAMPLE_RATE
        # 높은 에너지가 빠르게 내려오는 본음에 얕은 FM과 공기음을 겹친다.
        frequency = 550 + 2700 * math.exp(-time / 0.027)
        phase += 2 * math.pi * frequency / SAMPLE_RATE
        attack = 1 - math.exp(-time / 0.0012)
        envelope = attack * math.exp(-time / 0.045)
        tone = math.sin(phase + 0.45 * math.sin(phase * 1.48))
        tone += 0.16 * math.sin(phase * 2.01) * math.exp(-time / 0.022)
        noise = rng.uniform(-1, 1)
        low_noise += 0.18 * (noise - low_noise)
        air = (noise - low_noise) * attack * math.exp(-time / 0.020) * 0.10
        body = math.sin(2 * math.pi * 170 * time) * attack * math.exp(-time / 0.013) * 0.14
        dry.append(tone * envelope + air + body)

    # 짧고 낮은 초기 반사음만 더해 과도하게 긴 전자음 꼬리를 남기지 않는다.
    samples = dry.copy()
    for delay, gain in ((0.019, 0.14), (0.037, 0.08), (0.061, 0.035)):
        offset = round(delay * SAMPLE_RATE)
        for index in range(offset, len(samples)):
            samples[index] += dry[index - offset] * gain
    for index in range(len(samples)):
        samples[index] *= min(1.0, (len(samples) - 1 - index) / (SAMPLE_RATE * 0.025))
    peak = max(map(abs, samples))
    pcm = array('h', (round(value / peak * 0.72 * 32767) for value in samples))
    if sys.byteorder != 'little':
        pcm.byteswap()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT), 'wb') as sound:
        sound.setnchannels(1)
        sound.setsampwidth(2)
        sound.setframerate(SAMPLE_RATE)
        sound.writeframes(pcm.tobytes())
    print(f'Laser fire sound: {OUTPUT} / {DURATION:.2f}s, 48kHz, 16-bit mono')


if __name__ == '__main__':
    create()
