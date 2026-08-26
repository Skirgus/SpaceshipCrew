import unreal

meshes = [
    "/Game/Meshes/CorridorPanels/SM_Floor_400",
    "/Game/Meshes/CorridorPanels/SM_Ceiling_400",
    "/Game/Meshes/CorridorPanels/SM_Wall_Solid_400x300",
    "/Game/Meshes/CorridorPanels/SM_Wall_Door_400x300",
    "/Game/Meshes/CorridorPanels/SM_DoorFrame_Ring",
]

for path in meshes:
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if not mesh:
        unreal.log_error(f"Missing {path}")
        continue
    bounds = mesh.get_bounds()
    box = bounds.box_extent * 2.0
    unreal.log(f"{mesh.get_name()} extentXYZ=({box.x:.1f}, {box.y:.1f}, {box.z:.1f})")

definition = unreal.EditorAssetLibrary.load_asset("/Game/Data/ShipModules/Corridor_CustomPanels_01")
override = unreal.EditorAssetLibrary.load_asset("/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride")
if definition:
    unreal.log(f"ModuleId={definition.get_editor_property('module_id')}")
    unreal.log(f"Type={definition.get_editor_property('module_type')}")
    unreal.log(f"Size={definition.get_editor_property('size')}")
    unreal.log(f"CellSize={definition.get_editor_property('cell_size')}")
    unreal.log(f"HasInterior={definition.get_editor_property('has_interior')}")
    unreal.log(f"VisualOverride={definition.get_editor_property('visual_override')}")
    cps = definition.get_editor_property("contact_points") or []
    for cp in cps:
        unreal.log(f"  Socket {cp.get_editor_property('socket_name')} @ {cp.get_editor_property('relative_location')}")
if override:
    parts = override.get_editor_property("visual_parts") or []
    unreal.log(f"VisualParts={len(parts)}")
    for i, part in enumerate(parts):
        mesh = part.get_editor_property("mesh")
        tr = part.get_editor_property("relative_transform")
        unreal.log(f"  Part[{i}] mesh={mesh.get_name() if mesh else None} loc={tr.translation} rot={tr.rotation.rotator()}")
