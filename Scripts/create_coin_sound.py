"""짧은 금속성 두 음으로 코인 획득용 원본 WAV를 생성한다. 외부 음원을 사용하지 않는다."""
from array import array
import math
from pathlib import Path
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'SourceAssets/Audio/CoinPickup.wav'
SAMPLE_RATE = 48000
DURATION = 0.38


def create():
    samples = []
    # B5 → E6: 짧게 올라가는 밝은 코인 소리. 높은 금속성 배음은 먼저 감쇠시킨다.
    notes = ((0.0, 987.7666, 0.65, 0.065), (0.055, 1318.5102, 1.0, 0.09))
    for index in range(round(SAMPLE_RATE * DURATION)):
        time = index / SAMPLE_RATE
        value = 0.0
        for start, frequency, gain, decay in notes:
            age = time - start
            if age < 0:
                continue
            attack = 1.0 - math.exp(-age / 0.0015)
            phase = 2.0 * math.pi * frequency * age
            value += gain * attack * (
                math.sin(phase) * math.exp(-age / decay)
                + 0.20 * math.sin(phase * 2.0) * math.exp(-age / 0.045)
                + 0.10 * math.sin(phase * 2.756) * math.exp(-age / 0.025)
            )
        # 끝을 부드럽게 닫아 반복 획득에도 클릭 잡음이나 긴 잔향이 남지 않게 한다.
        value *= min(1.0, max(0.0, (DURATION - time) / 0.035))
        samples.append(value)

    peak = max(abs(value) for value in samples)
    pcm = array('h', (round(value / peak * 0.62 * 32767) for value in samples))
    if sys.byteorder != 'little':
        pcm.byteswap()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT), 'wb') as sound:
        sound.setnchannels(1)
        sound.setsampwidth(2)
        sound.setframerate(SAMPLE_RATE)
        sound.writeframes(pcm.tobytes())
    print(f'코인 획득음 생성: {OUTPUT} / {DURATION:.2f}초, 48kHz, 16-bit mono')


if __name__ == '__main__':
    create()
