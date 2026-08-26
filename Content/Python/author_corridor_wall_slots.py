# Author Corridor_CustomPanels wall slots with orientation-matched OpeningMesh.
# LR walls: SM_Wall_Solid + SM_Wall_Door (thin Y), yaw 0/180.
# FB walls: SM_Wall_Solid_FB + SM_Wall_Door_FB (thin X), yaw 0/180 only — never ±90.
import unreal
import os

MESH_DIR = "/Game/Meshes/CorridorPanels"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"
FBX_DIR = unreal.Paths.project_content_dir() + "Meshes/CorridorPanels/"
VO_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride"
DEF_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01"


def enable_ism(mat):
    try:
        mat.set_editor_property("used_with_instanced_static_meshes", True)
        unreal.MaterialEditingLibrary.recompile_material(mat)
    except Exception as exc:
        unreal.log_warning(f"ISM: {exc}")


def ensure_mat(mesh, mat_name):
    mat = unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/{mat_name}")
    if not mesh or not mat:
        return
    enable_ism(mat)
    try:
        mesh.set_material(0, mat)
    except Exception as exc:
        unreal.log_warning(f"set_material: {exc}")


def import_mesh(mesh_name: str, uniform_scale: float = 0.01):
    path = f"{MESH_DIR}/{mesh_name}"
    fbx_path = FBX_DIR + mesh_name + ".fbx"
    if not os.path.isfile(fbx_path):
        unreal.log_warning(f"FBX missing: {fbx_path}")
        return unreal.EditorAssetLibrary.load_asset(path)

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
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if mesh:
        e = mesh.get_bounds().box_extent * 2.0
        unreal.log(f"Imported {mesh_name} extent=({e.x:.1f}, {e.y:.1f}, {e.z:.1f})")
        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
    return mesh


def make_tr(location, yaw_deg=0.0):
    # Only 0/180 are used — stable Rotator→Quat round-trip (no ±90 pitch quirk).
    tr = unreal.Transform()
    tr.translation = unreal.Vector(*location)
    tr.rotation = unreal.Rotator(0.0, yaw_deg, 0.0).quaternion()
    tr.scale3d = unreal.Vector(1.0, 1.0, 1.0)
    return tr


def make_part(mesh, location, yaw_deg=0.0, wall_socket=None, opening_mesh=None, opening_kind="SlidingDoor"):
    part = unreal.ShipModuleVisualPart()
    part.set_editor_property("mesh", mesh)
    part.set_editor_property("relative_transform", make_tr(location, yaw_deg))
    if wall_socket:
        part.set_editor_property("wall_socket_name", unreal.Name(wall_socket))
        kind = (
            unreal.ShipModuleWallOpeningKind.SLIDING_DOOR
            if opening_kind == "SlidingDoor"
            else unreal.ShipModuleWallOpeningKind.PASSAGE
        )
        part.set_editor_property("opening_kind", kind)
        if opening_mesh:
            part.set_editor_property("opening_mesh", opening_mesh)
        part.set_editor_property("airtight_when_closed", True)
        part.set_editor_property("affects_oxygen_volume", True)
    return part


def log_part(i, p):
    mesh = p.get_editor_property("mesh")
    sock = p.get_editor_property("wall_socket_name")
    open_mesh = p.get_editor_property("opening_mesh")
    tr = p.get_editor_property("relative_transform")
    rot = tr.rotation.rotator()
    unreal.log(
        f"  [{i}] mesh={mesh.get_name() if mesh else None} "
        f"socket={sock} opening={open_mesh.get_name() if open_mesh else None} "
        f"yaw={rot.yaw:.1f} pitch={rot.pitch:.1f} roll={rot.roll:.1f} "
        f"loc=({tr.translation.x:.0f},{tr.translation.y:.0f},{tr.translation.z:.0f})"
    )


def main():
    unreal.log("=== author_corridor_wall_slots start ===")

    if not hasattr(unreal, "ShipModuleWallOpeningKind"):
        unreal.log_error(
            "ShipModuleWallOpeningKind missing — compile SpaceshipCrew (Ctrl+Alt+F11), then re-run."
        )
        return

    floor = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Floor_400")
    ceiling = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Ceiling_400")
    wall_lr = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Wall_Solid_400x300")
    door_lr = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Wall_Door_400x300")
    door_fb = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Wall_Door_FB_400x300")
    wall_fb = import_mesh("SM_Wall_Solid_FB_400x300", uniform_scale=1.0)

    if not all([floor, ceiling, wall_lr, door_lr, door_fb, wall_fb]):
        unreal.log_error("Missing required meshes (need SM_Wall_Solid_FB_400x300)")
        return

    for m in (floor, ceiling, wall_lr, door_lr, door_fb, wall_fb):
        ensure_mat(m, "M_CorridorHull_PBR")

    override = unreal.EditorAssetLibrary.load_asset(VO_PATH)
    definition = unreal.EditorAssetLibrary.load_asset(DEF_PATH)
    if not override or not definition:
        unreal.log_error("Missing VO/Definition")
        return

    parts = [
        make_part(floor, (0.0, 0.0, -142.0)),
        make_part(ceiling, (0.0, 0.0, 141.0)),
        # Left / Right — LR meshes, thin in Y
        make_part(wall_lr, (0.0, -190.0, 0.0), 0.0, "Left_X0_Y0_Z0", door_lr),
        make_part(wall_lr, (0.0, 190.0, 0.0), 180.0, "Right_X0_Y0_Z0", door_lr),
        # Front / Back — FB meshes, thin in X; yaw 0/180 only (same for Mesh and OpeningMesh)
        make_part(wall_fb, (191.0, 0.0, 0.0), 180.0, "Front_X0_Y0_Z0", door_fb),
        make_part(wall_fb, (-191.0, 0.0, 0.0), 0.0, "Back_X0_Y0_Z0", door_fb),
    ]

    override.set_editor_property("visual_parts", parts)
    unreal.EditorAssetLibrary.save_asset(override.get_path_name(), only_if_is_dirty=False)

    definition.set_editor_property("visual_override", override)
    definition.set_editor_property("has_interior", True)
    unreal.EditorAssetLibrary.save_loaded_asset(definition)
    unreal.EditorAssetLibrary.save_loaded_asset(override)

    saved = override.get_editor_property("visual_parts") or []
    unreal.log(f"Parts={len(saved)}")
    for i, p in enumerate(saved):
        log_part(i, p)
    unreal.log("=== author_corridor_wall_slots done ===")


if __name__ == "__main__":
    main()
