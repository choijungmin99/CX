"""UE 에디터 Python: 레이저 발사음과 발광 재질 생성. -AllowCommandletAudio 필요."""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
assets = unreal.AssetToolsHelpers.get_asset_tools()
source = ROOT / 'SourceAssets/Audio/LaserFire.wav'
if not source.exists():
    raise FileNotFoundError('먼저 Scripts/create_laser_sound.py를 실행하세요.')

unreal.EditorAssetLibrary.make_directory('/Game/Mining/Audio')
factory = unreal.SoundFactory()
factory.set_editor_property('auto_create_cue', False)
task = unreal.AssetImportTask()
for name, value in {
    'filename': str(source), 'destination_path': '/Game/Mining/Audio',
    'destination_name': 'S_LaserFire', 'factory': factory,
    'automated': True, 'replace_existing': True, 'save': True,
}.items():
    task.set_editor_property(name, value)
assets.import_asset_tasks([task])
sound = unreal.load_asset('/Game/Mining/Audio/S_LaserFire')
if not isinstance(sound, unreal.SoundWave):
    raise RuntimeError('레이저 SoundWave 가져오기 실패')
sound.set_editor_property('looping', False)
sound.set_editor_property('volume', 1.0)
sound.set_editor_property('pitch', 1.0)
sound.set_sound_asset_compression_type(unreal.SoundAssetCompressionType.PCM)
sound.set_editor_property('loading_behavior', unreal.SoundWaveLoadingBehavior.FORCE_INLINE)
unreal.EditorAssetLibrary.save_loaded_asset(sound)
assert 0.279 <= sound.get_editor_property('duration') <= 0.281

material_path = '/Game/Mining/M_LaserGlow'
material = unreal.load_asset(material_path) if unreal.EditorAssetLibrary.does_asset_exist(material_path) else None
if not material:
    material = assets.create_asset('M_LaserGlow', '/Game/Mining', unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided', True)
edit = unreal.MaterialEditingLibrary

def node(kind, x, y):
    # 기존 노드를 재사용해 재실행 시 중복 생성과 불필요한 UObject 삭제를 피한다.
    existing = unreal.find_object(material, kind.__name__ + '_0')
    return existing or edit.create_material_expression(material, kind, x, y)

color = node(unreal.MaterialExpressionVectorParameter, -500, -100)
color.set_editor_property('parameter_name', 'Color')
color.set_editor_property('default_value', unreal.LinearColor(0.1, 4.0, 8.0, 1.0))
fade = node(unreal.MaterialExpressionScalarParameter, -500, 250)
fade.set_editor_property('parameter_name', 'Fade')
fade.set_editor_property('default_value', 1.0)
edge = node(unreal.MaterialExpressionFresnel, -500, 100)
edge.set_editor_property('exponent', 2.0)
edge.set_editor_property('base_reflect_fraction', 0.0)
inverse = node(unreal.MaterialExpressionOneMinus, -250, 100)
opacity = node(unreal.MaterialExpressionMultiply, -50, 150)
# 단일 입력 노드는 빈 이름으로 연결한다. 표시되는 핀 이름은 C++ 속성명과 다를 수 있다.
assert edit.connect_material_expressions(edge, '', inverse, '')
assert edit.connect_material_expressions(inverse, '', opacity, 'A')
assert edit.connect_material_expressions(fade, '', opacity, 'B')
assert edit.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert edit.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
edit.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)

report = {'sound': '/Game/Mining/Audio/S_LaserFire', 'duration_seconds': sound.get_editor_property('duration'),
          'compression': 'PCM', 'looping': False, 'material': material_path, 'blend': 'Additive', 'shading': 'Unlit'}
output = ROOT / 'Saved/Tests/LaserAssets.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('CX_LASER_ASSETS_IMPORTED ' + json.dumps(report))
