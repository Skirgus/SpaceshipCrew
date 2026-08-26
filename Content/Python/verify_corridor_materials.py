import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
MESHES = [
    "SM_Floor_400",
    "SM_Ceiling_400",
    "SM_Wall_Solid_400x300",
    "SM_Wall_Door_FB_400x300",
    "SM_DoorFrame_Ring_FB",
]

for name in MESHES:
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{name}")
    if not mesh:
        unreal.log_error(f"missing {name}")
        continue
    mats = mesh.get_editor_property("static_materials") or []
    unreal.log(f"{name} slots={len(mats)}")
    for i, sm in enumerate(mats):
        mi = sm.get_editor_property("material_interface")
        slot = sm.get_editor_property("material_slot_name")
        unreal.log(f"  [{i}] {slot} -> {mi.get_name() if mi else None}")

for mat_name in ["M_CorridorHull_PBR", "M_CorridorFloor_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"]:
    mat = unreal.EditorAssetLibrary.load_asset(f"/Game/Meshes/CorridorPanels/Materials/{mat_name}")
    unreal.log(f"Material {mat_name}: {'OK' if mat else 'MISSING'}")
