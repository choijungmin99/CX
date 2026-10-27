"""UE 5.5.4 Editor Python: 필수 애셋과 제출용 경기장을 재현 가능하게 생성한다."""
import unreal

assets = unreal.AssetToolsHelpers.get_asset_tools()
editor_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
for folder in ('/Game/Maps', '/Game/Input', '/Game/UI', '/Game/Mining'):
    unreal.EditorAssetLibrary.make_directory(folder)

def action(name):
    path = '/Game/Input/' + name
    a = unreal.load_asset(path)
    if not a:
        factory = unreal.InputAction_Factory()
        a = assets.create_asset(name, '/Game/Input', unreal.InputAction, factory)
    a.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    unreal.EditorAssetLibrary.save_loaded_asset(a)
    return a

fire = action('IA_Fire')
mine = action('IA_Mine')
mapping = unreal.load_asset('/Game/Input/IMC_Combat')
if not mapping:
    mapping = assets.create_asset('IMC_Combat', '/Game/Input', unreal.InputMappingContext, unreal.InputMappingContext_Factory())
mapping.unmap_all()
def key(name):
    result = unreal.Key()
    result.set_editor_property('key_name', name)
    return result
mapping.map_key(fire, key('LeftMouseButton'))
mapping.map_key(mine, key('F'))
unreal.EditorAssetLibrary.save_loaded_asset(mapping)

def widget(name, native_class):
    existing = unreal.load_asset('/Game/UI/' + name)
    if existing:
        return existing
    f = unreal.WidgetBlueprintFactory()
    f.set_editor_property('parent_class', unreal.load_class(None, '/Script/CX.' + native_class))
    bp = assets.create_asset(name, '/Game/UI', unreal.WidgetBlueprint, f)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    return bp

widget('WBP_ChatUI', 'ChatUI')
widget('WBP_CXHUD', 'CXGameWidget')

def tint_material(name, glow):
    m = unreal.load_asset('/Game/Mining/' + name)
    if m:
        return m
    m = assets.create_asset(name, '/Game/Mining', unreal.Material, unreal.MaterialFactoryNew())
    tint = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -350, 0)
    tint.set_editor_property('parameter_name', 'Tint')
    tint.set_editor_property('default_value', unreal.LinearColor(.1, .3, .2, 1))
    unreal.MaterialEditingLibrary.connect_material_property(tint, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionConstant, -350, 200)
    rough.set_editor_property('r', .65)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    if glow:
        mul = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionMultiply, -100, 100)
        mul.set_editor_property('const_b', 2.0)
        unreal.MaterialEditingLibrary.connect_material_expressions(tint, '', mul, 'A')
        unreal.MaterialEditingLibrary.connect_material_property(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

tint_material('M_Items', True)
tint_material('M_ArenaTint', False)

# 기존 템플릿 맵은 보존하고 별도 맵을 생성한다.
level_path = '/Game/Maps/CXMiningArena'
if unreal.EditorAssetLibrary.does_asset_exist(level_path):
    levels.load_level(level_path)
else:
    levels.new_level(level_path)
if not any(a.get_class().get_name() == 'CXMiningArena' for a in editor_actors.get_all_level_actors()):
    arena = editor_actors.spawn_actor_from_class(unreal.load_class(None, '/Script/CX.CXMiningArena'), unreal.Vector())
    arena.set_actor_label('채굴 경기장 - 도로, 램프, 숲, 암벽')
    arena.set_actor_location(unreal.Vector(0, 0, 0), False, True)
    arena.rebuild()
    for index, (x, y) in enumerate(((-6000, -700), (-6000, 700), (-8000, 0)), 1):
        start = editor_actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(x, y, 140), unreal.Rotator(pitch=0, yaw=0, roll=0))
        start.set_actor_label('PlayerStart_%d_겹침검사' % index)
    for index, (x, y, speed) in enumerate([(x, y, False) for x in range(-4500, 10000, 2400) for y in (-3200, 0, 3200)] + [(-8000, -9000, False), (0, -9000, True), (8000, -9000, False), (-8000, 9000, False), (0, 9000, True), (8000, 9000, False), (-11000, -5000, True), (11000, 5000, True)]):
        spawner = editor_actors.spawn_actor_from_class(unreal.load_class(None, '/Script/CX.CXItemSpawner'), unreal.Vector(x, y, 150))
        spawner.set_editor_property('item_class', unreal.load_class(None, '/Script/CX.CXSpeedBuff' if speed else '/Script/CX.CXCoin'))
        spawner.set_actor_label(('속도 버프' if speed else '원화 코인') + '_스포너_%02d' % index)
    sun = editor_actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 5000), unreal.Rotator(pitch=-42, yaw=-30, roll=0))
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    light.set_editor_property('intensity', 8.0)
    light.set_editor_property('atmosphere_sun_light', True)
    editor_actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    sky = editor_actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 5000))
    skylight = sky.get_component_by_class(unreal.SkyLightComponent)
    skylight.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    skylight.set_editor_property('real_time_capture', True)
    skylight.set_editor_property('intensity', 1.3)
    fog = editor_actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -2000))
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property('fog_density', .007)
    post = editor_actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector())
    post.set_editor_property('unbound', True)
    settings = post.get_editor_property('settings')
    for key, value in [('override_auto_exposure_min_brightness', True), ('override_auto_exposure_max_brightness', True), ('auto_exposure_min_brightness', .8), ('auto_exposure_max_brightness', .8), ('override_motion_blur_amount', True), ('motion_blur_amount', 0.0)]:
        settings.set_editor_property(key, value)
    post.set_editor_property('settings', settings)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', unreal.load_class(None, '/Script/CX.CXGameMode'))
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-10000, -2800, 2200), unreal.Rotator(pitch=-12, yaw=20, roll=0))
    levels.save_current_level()

unreal.EditorAssetLibrary.save_directory('/Game/Input')
unreal.EditorAssetLibrary.save_directory('/Game/UI')
unreal.EditorAssetLibrary.save_directory('/Game/Mining')
print('CX_SETUP_COMPLETE', level_path)
