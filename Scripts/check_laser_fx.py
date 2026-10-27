"""cx.py test --capture-laser 실행 후 양쪽 효과 재생·정리와 원본 WAV를 확인한다."""
from array import array
import json
from pathlib import Path
import re
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
checks = {}
shots = {}
for role, logfile in [('HOST', 'Host.log'), ('CLIENT', 'Client.log')]:
    text = (ROOT / 'Saved/Tests' / logfile).read_text(encoding='utf-8-sig', errors='replace')
    events = re.findall(r'CX_LASER_FX role=' + role + r' beam=(\d) sound=(\d) hit=(\d)', text)
    cleanup = re.findall(r'CX_LASER_CLEANUP role=' + role + r' beam_gone=(\d) sound_stopped=(\d)', text)
    key = role.lower()
    checks[key + '_four_shots_have_beam_and_audio'] = len(events) == 4 and all(a == b == '1' for a, b, _ in events)
    checks[key + '_miss_and_three_hits'] = [hit for _, _, hit in events].count('0') == 1 and [hit for _, _, hit in events].count('1') == 3
    checks[key + '_effects_and_audio_finish'] = len(cleanup) == 4 and all(a == b == '1' for a, b in cleanup)
    checks[key + '_material_compiles'] = 'Failed to compile Material' not in text and 'Fatal error' not in text
    captures = [ROOT / f'Saved/Tests/LaserScreens/Laser_{role}_{index:02d}.png' for index in range(4)]
    checks[key + '_render_captures_saved'] = all(p.exists() and p.stat().st_size > 20000 for p in captures)
    shots[key] = len(events)

with wave.open(str(ROOT / 'SourceAssets/Audio/LaserFire.wav'), 'rb') as sound:
    channels, width, rate, frames = sound.getnchannels(), sound.getsampwidth(), sound.getframerate(), sound.getnframes()
    pcm = array('h', sound.readframes(frames))
if sys.byteorder != 'little':
    pcm.byteswap()
peak = max(map(abs, pcm)) / 32767
checks['source_audio_format_and_no_clipping'] = channels == 1 and width == 2 and rate == 48000 and abs(frames / rate - .28) < .001 and .70 < peak < .75 and pcm[0] == pcm[-1] == 0
report = {'checks': checks, 'check_count': len(checks), 'passed': all(checks.values()),
          'measurements': {'shots_per_side': shots, 'audio_seconds': frames / rate, 'audio_peak': round(peak, 4)}}
output = ROOT / 'Docs/LaserFXReport.json'
output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report, indent=2))
if not report['passed']:
    raise SystemExit('Laser verification failed. Inspect Saved/Tests logs and captures.')
