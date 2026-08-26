# Unreal Editor Python: import Blender corridor panels and create ShipModule assets.
# Run: UnrealEditor-Cmd ... -ExecutePythonScript=.../import_corridor_panels_module.py

import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
MODULE_DIR = "/Game/Data/ShipModules"
FBX_SOURCE_DIR = unreal.Paths.project_content_dir() + "Meshes/CorridorPanels/"

MESH_NAMES = [
    "SM_Floor_400",
    "SM_Ceiling_400",
    "SM_Wall_Solid_400x300",
    "SM_Wall_Door_400x300",
    "SM_DoorFrame_Ring",
]

MODULE_NAME = "Corridor_CustomPanels_01"
OVERRIDE_NAME = "Corridor_CustomPanels_01_VisualOverride"


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_fbx(mesh_name: str):
    destination = f"{MESH_DIR}/{mesh_name}"
    existing = unreal.EditorAssetLibrary.load_asset(destination)
    if existing:
        unreal.log(f"Already imported: {destination}")
        return existing

    fbx_path = FBX_SOURCE_DIR + mesh_name + ".fbx"
    if not unreal.Paths.file_exists(fbx_path):
        unreal.log_error(f"Missing FBX: {fbx_path}")
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
    options.import_materials = True
    options.import_textures = False
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
    options.static_mesh_import_data.set_editor_property("import_uniform_scale", 0.01)
    options.static_mesh_import_data.set_editor_property("convert_scene", True)
    options.static_mesh_import_data.set_editor_property("force_front_x_axis", False)
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported = unreal.EditorAssetLibrary.load_asset(destination)
    if not imported:
        for candidate in unreal.EditorAssetLibrary.list_assets(MESH_DIR, recursive=False):
            if mesh_name in candidate:
                imported = unreal.EditorAssetLibrary.load_asset(candidate)
                break
    if imported:
        unreal.log(f"Imported: {imported.get_path_name()}")
    else:
        unreal.log_error(f"Failed to import {mesh_name}")
    return imported


def make_transform(location, yaw_deg: float = 0.0, scale=(1.0, 1.0, 1.0)):
    # unreal.Rotator is Pitch, Yaw, Roll — yaw rotates around Z.
    return unreal.Transform(
        unreal.Vector(*location),
        unreal.Rotator(0.0, yaw_deg, 0.0),
        unreal.Vector(*scale),
    )


def make_visual_part(mesh, transform):
    part = unreal.ShipModuleVisualPart()
    part.set_editor_property("mesh", mesh)
    part.set_editor_property("relative_transform", transform)
    return part


def create_or_load_asset(asset_name: str, package_path: str, asset_class, factory_class_name: str):
    full_path = f"{package_path}/{asset_name}"
    existing = unreal.EditorAssetLibrary.load_asset(full_path)
    if existing:
        return existing

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    # Prefer project factories when available to Python.
    factory = None
    if hasattr(unreal, factory_class_name):
        factory = getattr(unreal, factory_class_name)()
    else:
        factory = unreal.DataAssetFactory()
        try:
            factory.set_editor_property("data_asset_class", asset_class)
        except Exception:
            pass

    created = asset_tools.create_asset(asset_name, package_path, asset_class, factory)
    if created:
        return created

    # Fallback: duplicate a known sibling asset of the same class.
    fallbacks = {
        "ShipModuleDefinition": "/Game/Data/ShipModules/TestCorridorModule",
        "ShipModuleVisualOverride": "/Game/Data/ShipModules/ModularSciFiCorridor_VisualOverride",
    }
    source = fallbacks.get(asset_class.__name__)
    if source and unreal.EditorAssetLibrary.does_asset_exist(source):
        duplicated = unreal.EditorAssetLibrary.duplicate_asset(source, package_path, asset_name)
        unreal.log(f"Duplicated fallback {source} -> {package_path}/{asset_name}")
        return duplicated

    return None


def build_module(meshes) -> None:
    ensure_directory(MODULE_DIR)

    definition = create_or_load_asset(
        MODULE_NAME,
        MODULE_DIR,
        unreal.ShipModuleDefinition,
        "ShipModuleDefinitionFactory",
    )
    override = create_or_load_asset(
        OVERRIDE_NAME,
        MODULE_DIR,
        unreal.ShipModuleVisualOverride,
        "ShipModuleVisualOverrideFactory",
    )

    if not definition or not override:
        unreal.log_error("Failed to create Definition or VisualOverride")
        return

    # Identity / physics
    definition.set_editor_property("module_id", "Corridor_CustomPanels_01")
    definition.set_editor_property("module_type", unreal.ShipModuleType.CORRIDOR)
    definition.set_editor_property("display_name", unreal.Text("Коридор (Custom Panels)"))
    definition.set_editor_property(
        "description",
        unreal.Text("Коридор 1×1×1 из панелей Blender (400×400×300 см)."),
    )
    definition.set_editor_property("mass", 120.0)
    definition.set_editor_property("credit_cost", 120)
    definition.set_editor_property("cell_size", unreal.IntVector(1, 1, 1))
    # UE Python strips leading 'b' from bool UPROPERTY names.
    definition.set_editor_property("has_interior", True)

    # Compatible neighbors
    compatible = [
        unreal.ShipModuleType.CORRIDOR,
        unreal.ShipModuleType.BRIDGE,
        unreal.ShipModuleType.REACTOR,
        unreal.ShipModuleType.AIRLOCK,
        unreal.ShipModuleType.CARGO_HOLD,
        unreal.ShipModuleType.ENGINE,
        unreal.ShipModuleType.SCIENCE_LAB,
    ]
    definition.set_editor_property("compatible_module_types", compatible)

    # Sync size + default panel sockets
    if hasattr(definition, "sync_cell_size_and_size_from_legacy"):
        definition.sync_cell_size_and_size_from_legacy()
    if hasattr(definition, "regenerate_default_contact_points_from_cell_size"):
        definition.regenerate_default_contact_points_from_cell_size()

    definition.set_editor_property("visual_override", override)

    floor = meshes["SM_Floor_400"]
    ceiling = meshes["SM_Ceiling_400"]
    wall = meshes["SM_Wall_Solid_400x300"]
    door = meshes["SM_Wall_Door_400x300"]
    frame = meshes.get("SM_DoorFrame_Ring")

    # Placement relative to module center (0,0,0), Size 400x400x300
    parts = [
        make_visual_part(floor, make_transform((0.0, 0.0, -142.0))),
        make_visual_part(ceiling, make_transform((0.0, 0.0, 141.0))),
        # Solid walls Left/Right — mesh thin in Y, detail on +Y
        make_visual_part(wall, make_transform((0.0, -190.0, 0.0), yaw_deg=0.0)),
        make_visual_part(wall, make_transform((0.0, 190.0, 0.0), yaw_deg=180.0)),
        # Door walls Front/Back — rotate so thin axis aligns with X
        make_visual_part(door, make_transform((191.0, 0.0, 0.0), yaw_deg=90.0)),
        make_visual_part(door, make_transform((-191.0, 0.0, 0.0), yaw_deg=-90.0)),
    ]
    if frame:
        parts.append(make_visual_part(frame, make_transform((191.0, 0.0, 0.0), yaw_deg=90.0)))
        parts.append(make_visual_part(frame, make_transform((-191.0, 0.0, 0.0), yaw_deg=-90.0)))

    override.set_editor_property("visual_parts", parts)
    override.set_editor_property("override_contact_points", False)

    unreal.EditorAssetLibrary.save_asset(definition.get_path_name())
    unreal.EditorAssetLibrary.save_asset(override.get_path_name())
    for mesh in meshes.values():
        if mesh:
            unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())

    unreal.log(f"Created module: {definition.get_path_name()}")
    unreal.log(f"Visual parts: {len(parts)}")
    unreal.log(f"Size: {definition.get_editor_property('size')}")
    unreal.log(f"CellSize: {definition.get_editor_property('cell_size')}")
    contact_points = definition.get_editor_property("contact_points")
    unreal.log(f"ContactPoints: {len(contact_points) if contact_points else 0}")


def main() -> None:
    unreal.log("=== import_corridor_panels_module start ===")
    ensure_directory(MESH_DIR)
    ensure_directory(MODULE_DIR)

    meshes = {}
    for name in MESH_NAMES:
        mesh = import_fbx(name)
        if mesh:
            meshes[name] = mesh

    required = ["SM_Floor_400", "SM_Ceiling_400", "SM_Wall_Solid_400x300", "SM_Wall_Door_400x300"]
    missing = [n for n in required if n not in meshes]
    if missing:
        unreal.log_error(f"Missing required meshes: {missing}")
        return

    build_module(meshes)
    unreal.log("=== import_corridor_panels_module done ===")


if __name__ == "__main__":
    main()
