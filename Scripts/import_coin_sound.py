"""UE 5.5 에디터 Python: 원본 WAV를 코인 전용 SoundWave 애셋으로 가져온다.

명령줄의 -run=pythonscript 실행에는 -AllowCommandletAudio도 지정하여 사운드 디코더를 초기화한다.
"""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'SourceAssets/Audio/CoinPickup.wav'
ASSET_PATH = '/Game/Mining/Audio/S_CoinPickup'
if not SOURCE.exists():
    raise FileNotFoundError('먼저 Scripts/create_coin_sound.py로 원본 WAV를 생성하세요.')

unreal.EditorAssetLibrary.make_directory('/Game/Mining/Audio')
factory = unreal.SoundFactory()
factory.set_editor_property('auto_create_cue', False)
task = unreal.AssetImportTask()
task.set_editor_property('filename', str(SOURCE))
task.set_editor_property('destination_path', '/Game/Mining/Audio')
task.set_editor_property('destination_name', 'S_CoinPickup')
task.set_editor_property('factory', factory)
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound = unreal.load_asset(ASSET_PATH)
if not isinstance(sound, unreal.SoundWave):
    raise RuntimeError('코인 SoundWave 애셋을 가져오지 못했습니다.')

# 재생 코드에서도 원래 피치를 사용한다. 짧은 획득음은 루프와 스트리밍을 사용하지 않는다.
sound.set_editor_property('looping', False)
sound.set_editor_property('volume', 1.0)
sound.set_editor_property('pitch', 1.0)
sound.set_editor_property('compression_quality', 100)
sound.set_sound_asset_compression_type(unreal.SoundAssetCompressionType.PCM)
sound.set_editor_property('loading_behavior', unreal.SoundWaveLoadingBehavior.FORCE_INLINE)
unreal.EditorAssetLibrary.save_loaded_asset(sound)
duration = sound.get_editor_property('duration')
if not 0.37 <= duration <= 0.39:
    raise RuntimeError(f'코인 획득음의 길이가 잘못됐습니다: {duration}')
report = {'asset': ASSET_PATH, 'duration_seconds': duration, 'looping': False, 'pitch': 1.0, 'compression': 'PCM'}
report_path = ROOT / 'Saved/Tests/CoinSoundAsset.json'
report_path.parent.mkdir(parents=True, exist_ok=True)
report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('CX_COIN_SOUND_IMPORTED ' + json.dumps(report, ensure_ascii=False))
