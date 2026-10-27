"""UE 5.5 에디터 Python: 점프/2층 경기장을 갱신하고 새 코스 아이템만 추가·이동한다."""
import json
import math
from pathlib import Path
import shutil
import unreal

ROOT = Path(__file__).resolve().parents[1]
MAP_PATH = '/Game/Maps/CXMiningArena'
map_file = ROOT / 'Content/Maps/CXMiningArena.umap'
backup = ROOT / 'Saved/Backups/ArenaBeforeUpperCourse_20261028.umap'
if not backup.exists():
    backup.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(map_file, backup)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level(MAP_PATH)
arenas = [actor for actor in actors.get_all_level_actors() if actor.get_class().get_name() == 'CXMiningArena']
if len(arenas) != 1:
    raise RuntimeError(f'경기장 액터는 한 개여야 합니다: {len(arenas)}')
arena = arenas[0]
arena.rebuild()
arena.set_actor_label('채굴 경기장 - 지상 점프와 2층 스카이 코스')

# 지상 코인은 연결되는 각 경사면 위에 놓는다. 높이는 차량 트리거에 맞춰 윗면 +150cm이다.
ground_points = [(-8500.0, 8.0)]
for pitch in (4, 8, 14, 19, 23):
    x, height = ground_points[-1]
    ground_points.append((x + 1200, height + 1200 * math.tan(math.radians(pitch))))

def ramp_height(x):
    for (ax, az), (bx, bz) in zip(ground_points, ground_points[1:]):
        if ax <= x <= bx:
            return az + (bz - az) * (x - ax) / (bx - ax)
    return 8.0

H = 1400.0
def access_height(y):
    t = (y + 2500.0) / 7500.0
    return 8.0 + (H - 8.0) * t * t * (3.0 - 2.0 * t)

placements = [
    (-12100, -5800, 158, False), (-10800, -5800, 158, True), (-9500, -5800, 158, False),
    (-7000, -5800, ramp_height(-7000) + 150, False),
    (-4600, -5800, ramp_height(-4600) + 150, False),
    (-2800, -5800, ramp_height(-2800) + 150, False),
    (1200, -5800, 700 - 1200 * 692 / 4500 + 150, False), (6500, -5800, 158, False),
    (-10500, 1000, access_height(1000) + 150, False),
    (-10500, 6000, H + 150, False), (-8000, 7500, H + 150, False),
    (-5500, 7500, H + 150, True), (4500, 7500, H + 150, False),
    (7500, 7500, H + 150, False), (9500, 7500, H + 150, False),
    (-6000, 9300, H + 150, False), (-4200, 10300, H + 150, False),
    (0, 10300, H + 150, True), (4200, 10300, H + 150, False),
    (10500, 0, access_height(0) + 150, False),
]
spawner_class = unreal.load_class(None, '/Script/CX.CXItemSpawner')
coin_class = unreal.load_class(None, '/Script/CX.CXCoin')
buff_class = unreal.load_class(None, '/Script/CX.CXSpeedBuff')
existing = actors.get_all_level_actors()
for index, (x, y, z, speed) in enumerate(placements, 1):
    tag = f'CX_UpperCourseItem_{index:02d}'
    matches = [actor for actor in existing if actor.actor_has_tag(tag)]
    if len(matches) > 1:
        raise RuntimeError(f'중복된 코스 아이템 태그: {tag}')
    spawner = matches[0] if matches else actors.spawn_actor_from_class(spawner_class, unreal.Vector(x, y, z))
    spawner.set_actor_location(unreal.Vector(x, y, z), False, True)
    spawner.set_editor_property('item_class', buff_class if speed else coin_class)
    spawner.set_editor_property('tags', [unreal.Name(tag)])
    spawner.set_actor_label(f'스카이코스_{"속도" if speed else "코인"}_{index:02d}')
    spawner.set_folder_path('경기장/스카이 코스 아이템')

# 처음 열었을 때 지상 점프와 고가 도로를 함께 볼 수 있는 구도로 저장한다.
camera = unreal.Vector(-19500, -23500, 22000)
target = unreal.Vector(0, 1800, 700)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera, unreal.MathLibrary.find_look_at_rotation(camera, target))
if not levels.save_current_level():
    raise RuntimeError('갱신한 경기장을 저장하지 못했습니다.')
report = {'map': MAP_PATH, 'upper_deck_height_cm': H, 'new_course_spawners': len(placements),
          'ground_launch_height_cm': ground_points[-1][1], 'backup': str(backup)}
output = ROOT / 'Saved/Tests/ArenaLayout.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('CX_ARENA_LAYOUT_UPDATED ' + json.dumps(report, ensure_ascii=False))
