# BP_CrewSeat (Occupy, no UI) and one placement on Bridge_Training_01.
# Output Log: py "H:/Projects/SpaceshipCrew/Content/Python/author_crew_seat.py"

from __future__ import annotations

import math
import os

import unreal

BP_DIR = "/Game/Blueprints/Equipment"
BP_NAME = "BP_CrewSeat"
BP_PATH = f"{BP_DIR}/{BP_NAME}"
OVERRIDE_PATH = "/Game/Data/ShipModules/Bridge_Training_01_VisualOverride"
MESH_DIR = "/Game/Meshes/Equipment"
MESH_NAME = "SM_CrewSeat"
FBX_PATH = os.path.join(unreal.Paths.project_content_dir(), "Meshes", "Equipment", "SM_CrewSeat.fbx")
# Пол мостика: FloorTopLocalZ. Origin меша — на полу.
FLOOR_TOP_Z = -184.0
# Точка посадки: капсула над ступнями перед сиденьем. author_crew_seat_sit.py ставит то же значение.
# Перед сиденья — +Y. Якорь с yaw +90, см. author_crew_seat_sit.py.
STAND_ANCHOR = unreal.Vector(0.0, 57.5, 96.0)
STAND_YAW = 90.0


def import_seat_mesh(uniform_scale: float):
    if not os.path.isfile(FBX_PATH):
        unreal.log_error(f"[CrewSeat] FBX missing: {FBX_PATH}")
        return None

    unreal.EditorAssetLibrary.make_directory(MESH_DIR)
    task = unreal.AssetImportTask()
    task.filename = FBX_PATH
    task.destination_path = MESH_DIR
    task.destination_name = MESH_NAME
    task.replace_existing = True
    task.automated = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = True
    options.import_textures = False
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
    options.static_mesh_import_data.set_editor_property("import_uniform_scale", uniform_scale)
    options.static_mesh_import_data.set_editor_property("convert_scene", True)
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{MESH_NAME}")
    if not mesh:
        unreal.log_error("[CrewSeat] import failed")
        return None
    extent = mesh.get_bounds().box_extent * 2.0
    unreal.log(
        f"[CrewSeat] imported scale={uniform_scale} size=({extent.x:.1f},{extent.y:.1f},{extent.z:.1f})"
    )
    return mesh


def load_seat_mesh():
    for scale in (1.0, 100.0, 0.01):
        mesh = import_seat_mesh(scale)
        if not mesh:
            return None
        height = (mesh.get_bounds().box_extent * 2.0).z
        if 90.0 <= height <= 170.0:
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            return mesh
    unreal.log_error("[CrewSeat] mesh height is outside the expected 110–170 cm range")
    return mesh


def main() -> None:
    unreal.log("[CrewSeat] start")
    parent = unreal.load_class(None, "/Script/SpaceshipCrew.UsableEquipment")
    if not parent:
        unreal.log_error("[CrewSeat] AUsableEquipment is missing. Compile the C++ module first.")
        return

    if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
        unreal.EditorAssetLibrary.make_directory(BP_DIR)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent)
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        blueprint = tools.create_asset(BP_NAME, BP_DIR, unreal.Blueprint, factory)
    else:
        blueprint = unreal.EditorAssetLibrary.load_asset(BP_PATH)

    if not blueprint:
        unreal.log_error("[CrewSeat] failed to create or load blueprint")
        return

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated = unreal.load_object(None, f"{BP_PATH}.{BP_NAME}_C")
    if not generated:
        unreal.log_error("[CrewSeat] generated class was not created")
        return
    cdo = unreal.get_default_object(generated)

    seat_mesh = load_seat_mesh()
    display = cdo.get_editor_property("display_mesh")
    if display and seat_mesh:
        display.set_static_mesh(seat_mesh)
        display.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, 0.0))
        display.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
        display.set_editor_property("relative_scale3d", unreal.Vector(1.0, 1.0, 1.0))
    elif not seat_mesh:
        unreal.log_error("[CrewSeat] SM_CrewSeat was not imported; blueprint mesh left unchanged")

    occupy = unreal.UsableEquipmentUseMode.OCCUPY
    cdo.set_editor_property("use_mode", occupy)
    cdo.set_editor_property("display_name", "Кресло")
    cdo.set_editor_property("prompt_text", "Сесть")

    # Точку посадки и монтаж задаёт author_crew_seat_sit.py. Повторный прогон меша их не сбрасывает.
    anchor = cdo.get_editor_property("use_anchor")
    existing_montage = cdo.get_editor_property("character_use_montage")
    if anchor and not existing_montage:
        anchor.set_editor_property("relative_location", STAND_ANCHOR)
        anchor.set_editor_property(
            "relative_rotation", unreal.Rotator(pitch=0.0, yaw=STAND_YAW, roll=0.0)
        )

    prompt = cdo.get_editor_property("prompt_anchor")
    if prompt:
        prompt.set_editor_property("relative_location", unreal.Vector(-8.0, 18.0, 118.0))

    volume = cdo.get_editor_property("interaction_volume")
    if volume:
        volume.set_editor_property("relative_location", unreal.Vector(55.0, 0.0, 90.0))
        volume.set_editor_property("box_extent", unreal.Vector(80.0, 55.0, 100.0))

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)

    override = unreal.EditorAssetLibrary.load_asset(OVERRIDE_PATH)
    if not override:
        unreal.log_error(f"[CrewSeat] missing {OVERRIDE_PATH}")
        return

    placement = unreal.ShipModuleEquipmentPlacement()
    placement.set_editor_property("equipment_class", generated)
    transform = unreal.Transform()
    transform.translation = unreal.Vector(200.0, -120.0, FLOOR_TOP_Z)
    # Явный рысканье вокруг Z. Rotator(0, 180, 0).quaternion() в этом редакторе
    # даёт кватернион (0, -1, 0, 0) — тангаж 180, кресло уходит в пол.
    yaw = math.radians(180.0)
    transform.rotation = unreal.Quat(0.0, 0.0, math.sin(yaw * 0.5), math.cos(yaw * 0.5))
    transform.scale3d = unreal.Vector(1.0, 1.0, 1.0)
    placement.set_editor_property("relative_transform", transform)
    unreal.log("[CrewSeat] placement rot=%s loc=%s" % (transform.rotation, transform.translation))

    kept = []
    for item in override.get_editor_property("equipment_placements"):
        item_class = item.get_editor_property("equipment_class")
        class_name = ""
        if item_class:
            try:
                class_name = item_class.get_name()
            except Exception:
                class_name = str(item_class)
        if "BP_CrewSeat" not in class_name:
            kept.append(item)
    kept.append(placement)
    override.set_editor_property("equipment_placements", kept)
    unreal.EditorAssetLibrary.save_loaded_asset(override)
    unreal.log("[CrewSeat] BP_CrewSeat placed on Bridge_Training_01")


main()
