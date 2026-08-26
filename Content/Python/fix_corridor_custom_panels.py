# Fix corridor panel import scale (100x) and door-wall yaw rotations.
import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
FBX_SOURCE_DIR = unreal.Paths.project_content_dir() + "Meshes/CorridorPanels/"
OVERRIDE_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride"

MESH_NAMES = [
    "SM_Floor_400",
    "SM_Ceiling_400",
    "SM_Wall_Solid_400x300",
    "SM_Wall_Door_400x300",
    "SM_DoorFrame_Ring",
]


def reimport_scaled(mesh_name: str):
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
    options.import_materials = False
    options.import_textures = False
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
    # Blender numeric cm + FBX unit metadata → Unreal came in 100x too large.
    options.static_mesh_import_data.set_editor_property("import_uniform_scale", 0.01)
    options.static_mesh_import_data.set_editor_property("convert_scene", True)
    options.static_mesh_import_data.set_editor_property("force_front_x_axis", False)
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
    if mesh:
        extent = mesh.get_bounds().box_extent * 2.0
        unreal.log(f"Reimported {mesh_name} extent=({extent.x:.1f}, {extent.y:.1f}, {extent.z:.1f})")
    return mesh


def yaw_transform(location, yaw_deg: float = 0.0):
    # Explicit quaternion from yaw Rotator to avoid Transform(Rotator) mixups.
    rotator = unreal.Rotator(0.0, yaw_deg, 0.0)
    quat = rotator.quaternion()
    tr = unreal.Transform()
    tr.translation = unreal.Vector(*location)
    tr.rotation = quat
    tr.scale3d = unreal.Vector(1.0, 1.0, 1.0)
    return tr


def make_part(mesh, location, yaw_deg=0.0):
    part = unreal.ShipModuleVisualPart()
    part.set_editor_property("mesh", mesh)
    part.set_editor_property("relative_transform", yaw_transform(location, yaw_deg))
    return part


def main():
    unreal.log("=== fix_corridor_custom_panels start ===")
    meshes = {}
    for name in MESH_NAMES:
        mesh = reimport_scaled(name)
        if mesh:
            meshes[name] = mesh
            unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())

    required = ["SM_Floor_400", "SM_Ceiling_400", "SM_Wall_Solid_400x300", "SM_Wall_Door_400x300"]
    if any(n not in meshes for n in required):
        unreal.log_error("Missing meshes after reimport")
        return

    override = unreal.EditorAssetLibrary.load_asset(OVERRIDE_PATH)
    if not override:
        unreal.log_error(f"Missing override {OVERRIDE_PATH}")
        return

    floor = meshes["SM_Floor_400"]
    ceiling = meshes["SM_Ceiling_400"]
    wall = meshes["SM_Wall_Solid_400x300"]
    door = meshes["SM_Wall_Door_400x300"]
    frame = meshes.get("SM_DoorFrame_Ring")

    parts = [
        make_part(floor, (0.0, 0.0, -142.0), 0.0),
        make_part(ceiling, (0.0, 0.0, 141.0), 0.0),
        make_part(wall, (0.0, -190.0, 0.0), 0.0),
        make_part(wall, (0.0, 190.0, 0.0), 180.0),
        make_part(door, (191.0, 0.0, 0.0), 90.0),
        make_part(door, (-191.0, 0.0, 0.0), -90.0),
    ]
    if frame:
        parts.append(make_part(frame, (191.0, 0.0, 0.0), 90.0))
        parts.append(make_part(frame, (-191.0, 0.0, 0.0), -90.0))

    override.set_editor_property("visual_parts", parts)
    unreal.EditorAssetLibrary.save_asset(override.get_path_name())

    for i, part in enumerate(parts):
        tr = part.get_editor_property("relative_transform")
        unreal.log(f"Part[{i}] loc={tr.translation} rot={tr.rotation.rotator()}")

    unreal.log("=== fix_corridor_custom_panels done ===")


if __name__ == "__main__":
    main()
