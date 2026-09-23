# Unreal Editor Python: import EngineerTraining FBX and build training level.
# Run in Editor Output Log:
#   py "H:/Projects/SpaceshipCrew/Content/Python/setup_engineer_training_level.py"
# Or: UnrealEditor-Cmd (Editor CLOSED) -ExecutePythonScript=...

from __future__ import annotations

import unreal

MESH_DIR = "/Game/Meshes/EngineerTraining"
MAP_DIR = "/Game/Maps/Training"
MAP_NAME = "EngineerTraining"
MAP_PATH = f"{MAP_DIR}/{MAP_NAME}"
FBX_SOURCE_DIR = unreal.Paths.project_content_dir() + "Meshes/EngineerTraining/"

FBX_MESHES = [
    "SM_TechBay_Interior_1x1x1",
    "SM_EnergyConsole",
    "SM_RepairTorch",
    "SM_HullPanel_Intact",
    "SM_HullPanel_Damaged",
]

# Blender unit was cm; new exports usually need scale 1.0. Fallback try 0.01 if bounds tiny.
IMPORT_SCALE = 1.0


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_fbx(mesh_name: str, uniform_scale: float = IMPORT_SCALE):
    destination = f"{MESH_DIR}/{mesh_name}"
    existing = unreal.EditorAssetLibrary.load_asset(destination)
    if existing:
        unreal.log(f"[EngineerTraining] Already imported: {destination}")
        return existing

    fbx_path = FBX_SOURCE_DIR + mesh_name + ".fbx"
    if not unreal.Paths.file_exists(fbx_path):
        unreal.log_error(f"[EngineerTraining] Missing FBX: {fbx_path}")
        return None

    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = MESH_DIR
    task.destination_name = mesh_name
    task.replace_existing = True
    task.automated = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = False
    options.import_textures = False
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
    options.static_mesh_import_data.set_editor_property("import_uniform_scale", uniform_scale)
    options.static_mesh_import_data.set_editor_property("convert_scene", True)
    options.static_mesh_import_data.set_editor_property("force_front_x_axis", False)
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = unreal.EditorAssetLibrary.load_asset(destination)
    if imported:
        unreal.log(f"[EngineerTraining] Imported: {imported.get_path_name()}")
    else:
        unreal.log_error(f"[EngineerTraining] Failed import: {mesh_name}")
    return imported


def spawn_static_mesh(mesh, location, rotation=(0.0, 0.0, 0.0), label: str = "") -> unreal.StaticMeshActor:
    loc = unreal.Vector(*location)
    rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])  # Pitch, Yaw, Roll
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    actor.static_mesh_component.set_static_mesh(mesh)
    if label:
        actor.set_actor_label(label)
    return actor


def spawn_actor(class_path: str, location, rotation=(0.0, 0.0, 0.0), label: str = ""):
    cls = unreal.load_class(None, class_path)
    if not cls:
        unreal.log_warning(f"[EngineerTraining] Class not found (compile C++?): {class_path}")
        return None
    loc = unreal.Vector(*location)
    rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(cls, loc, rot)
    if actor and label:
        actor.set_actor_label(label)
    return actor


def assign_mesh_if_possible(actor, mesh) -> None:
    if not actor or not mesh:
        return
    # Common component name on our C++ actors
    for prop_name in ("MeshComponent", "mesh_component", "StaticMeshComponent"):
        try:
            comp = actor.get_editor_property(prop_name)
            if comp and hasattr(comp, "set_static_mesh"):
                comp.set_static_mesh(mesh)
                return
        except Exception:
            pass
    # Fallback: first static mesh component
    try:
        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        if comps:
            comps[0].set_static_mesh(mesh)
    except Exception as exc:
        unreal.log_warning(f"[EngineerTraining] assign mesh failed: {exc}")


def create_or_open_level() -> bool:
    ensure_directory(MAP_DIR)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log(f"[EngineerTraining] Loading existing map {MAP_PATH}")
        return unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)

    # Create blank level asset then load
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.WorldFactory()
    world_asset = asset_tools.create_asset(MAP_NAME, MAP_DIR, unreal.World, factory)
    if not world_asset:
        unreal.log_error("[EngineerTraining] Failed to create World asset")
        return False
    unreal.EditorAssetLibrary.save_asset(MAP_PATH)
    return unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)


def clear_placeable_actors() -> None:
    keep = {"WorldSettings", "Brush", "DefaultPhysicsVolume", "GameplayDebuggerPlayerManager"}
    for actor in list(unreal.EditorLevelLibrary.get_all_level_actors()):
        label = actor.get_actor_label()
        cls_name = actor.get_class().get_name()
        if cls_name in ("WorldSettings", "Brush", "DefaultPhysicsVolume"):
            continue
        if label in keep:
            continue
        # Keep lights we might add intentionally — wipe previous setup actors by prefix
        if label.startswith("ET_") or cls_name in (
            "StaticMeshActor",
            "PlayerStart",
            "DirectionalLight",
            "SkyLight",
            "ExponentialHeightFog",
            "SkyAtmosphere",
        ):
            unreal.EditorLevelLibrary.destroy_actor(actor)


def setup_lighting() -> None:
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 400), unreal.Rotator(-45, 30, 0)
    )
    sun.set_actor_label("ET_Sun")
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 300), unreal.Rotator(0, 0, 0)
    )
    sky.set_actor_label("ET_SkyLight")
    # Soft ambient for indoor greybox
    try:
        atmos = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
        )
        atmos.set_actor_label("ET_SkyAtmosphere")
    except Exception:
        pass


def main() -> None:
    unreal.log("[EngineerTraining] === setup start ===")
    ensure_directory(MESH_DIR)
    ensure_directory(MAP_DIR)

    meshes = {}
    for name in FBX_MESHES:
        meshes[name] = import_fbx(name)

    if not create_or_open_level():
        unreal.log_error("[EngineerTraining] Could not open/create level")
        return

    clear_placeable_actors()
    setup_lighting()

    # Tech bay shell at origin (center of 1x1x1 cell)
    bay = meshes.get("SM_TechBay_Interior_1x1x1")
    if bay:
        spawn_static_mesh(bay, (0.0, 0.0, 0.0), label="ET_TechBay")

    # Player start inside bay (capsule half-height 96, floor top ≈ -134 → Z ≈ -36)
    ps = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, -36.0), unreal.Rotator(0.0, 180.0, 0.0)
    )
    ps.set_actor_label("ET_PlayerStart")

    # Оборудование живёт в VisualOverride модуля, не отдельными акторами карты.

    # World settings / game mode override for this map
    try:
        world = unreal.EditorLevelLibrary.get_editor_world()
        ws = world.world_settings if world else None
        if ws:
            gm = unreal.load_class(None, "/Script/SpaceshipCrew.SpaceshipCrewTrainingGameMode")
            if gm:
                ws.set_editor_property("default_game_mode", gm)
                unreal.log("[EngineerTraining] Map GameMode -> SpaceshipCrewTrainingGameMode")
    except Exception as exc:
        unreal.log_warning(f"[EngineerTraining] GameMode override failed: {exc}")

    unreal.EditorLevelLibrary.save_current_level()
    unreal.EditorAssetLibrary.save_directory(MESH_DIR, only_if_is_dirty=True, recursive=True)
    unreal.EditorAssetLibrary.save_directory(MAP_DIR, only_if_is_dirty=True, recursive=True)
    unreal.log(f"[EngineerTraining] === done. Map: {MAP_PATH} ===")


if __name__ == "__main__":
    main()
