# Import FB-oriented door panels and rebuild VisualParts without broken yaw→pitch.
import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
FBX_SOURCE_DIR = unreal.Paths.project_content_dir() + "Meshes/CorridorPanels/"
OVERRIDE_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride"
DEFINITION_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01"


def import_mesh(mesh_name: str, uniform_scale: float = 0.01):
    path = f"{MESH_DIR}/{mesh_name}"
    fbx_path = FBX_SOURCE_DIR + mesh_name + ".fbx"
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
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if mesh:
        e = mesh.get_bounds().box_extent * 2.0
        unreal.log(f"Imported {mesh_name} extent=({e.x:.1f}, {e.y:.1f}, {e.z:.1f})")
        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())
    return mesh


def identity_transform(location, yaw_deg: float = 0.0):
    tr = unreal.Transform()
    tr.translation = unreal.Vector(*location)
    # Only use yaw 0 or 180 — these read back stably.
    tr.rotation = unreal.Rotator(0.0, yaw_deg, 0.0).quaternion()
    tr.scale3d = unreal.Vector(1.0, 1.0, 1.0)
    return tr


def make_part(mesh, location, yaw_deg=0.0):
    part = unreal.ShipModuleVisualPart()
    part.set_editor_property("mesh", mesh)
    part.set_editor_property("relative_transform", identity_transform(location, yaw_deg))
    return part


def main():
    unreal.log("=== update_corridor_fb_doors start ===")
    door_fb = import_mesh("SM_Wall_Door_FB_400x300")
    frame_fb = import_mesh("SM_DoorFrame_Ring_FB")
    floor = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Floor_400")
    ceiling = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Ceiling_400")
    wall = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Wall_Solid_400x300")

    if not all([door_fb, floor, ceiling, wall]):
        unreal.log_error("Missing required meshes")
        return

    override = unreal.EditorAssetLibrary.load_asset(OVERRIDE_PATH)
    if not override:
        unreal.log_error("Missing visual override")
        return

    parts = [
        make_part(floor, (0.0, 0.0, -142.0), 0.0),
        make_part(ceiling, (0.0, 0.0, 141.0), 0.0),
        make_part(wall, (0.0, -190.0, 0.0), 0.0),
        make_part(wall, (0.0, 190.0, 0.0), 180.0),
        # FB door meshes are already thin in X; yaw 180 flips trim toward interior on Front.
        make_part(door_fb, (191.0, 0.0, 0.0), 180.0),
        make_part(door_fb, (-191.0, 0.0, 0.0), 0.0),
    ]
    if frame_fb:
        parts.append(make_part(frame_fb, (191.0, 0.0, 0.0), 180.0))
        parts.append(make_part(frame_fb, (-191.0, 0.0, 0.0), 0.0))

    override.set_editor_property("visual_parts", parts)
    unreal.EditorAssetLibrary.save_asset(override.get_path_name())

    definition = unreal.EditorAssetLibrary.load_asset(DEFINITION_PATH)
    if definition:
        unreal.EditorAssetLibrary.save_asset(definition.get_path_name())

    for i, part in enumerate(parts):
        tr = part.get_editor_property("relative_transform")
        mesh = part.get_editor_property("mesh")
        unreal.log(
            f"Part[{i}] {mesh.get_name() if mesh else None} "
            f"loc={tr.translation} rot={tr.rotation.rotator()}"
        )
    unreal.log("=== update_corridor_fb_doors done ===")


if __name__ == "__main__":
    main()
