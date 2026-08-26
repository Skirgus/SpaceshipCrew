# Only reassign rebuilt PBR materials to meshes (textures/materials already fixed).
import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"

MESH_MATS = {
    "SM_Floor_400": ["M_CorridorFloor_PBR", "M_CorridorTrim_PBR"],
    "SM_Ceiling_400": ["M_CorridorHull_PBR", "M_CorridorGlow_PBR"],
    "SM_Wall_Solid_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR"],
    "SM_Wall_Door_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_Wall_Door_FB_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_DoorFrame_Ring": ["M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_DoorFrame_Ring_FB": ["M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
}

for mesh_name, mat_names in MESH_MATS.items():
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
    mats = []
    for name in mat_names:
        m = unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/{name}")
        if not m:
            unreal.log_error(f"Missing material {name}")
        mats.append(m)

    materials = list(mesh.get_editor_property("static_materials") or [])
    while len(materials) < len(mats):
        materials.append(unreal.StaticMaterial())
    for i, mat in enumerate(mats):
        sm = materials[i]
        sm.set_editor_property("material_interface", mat)
        materials[i] = sm
    mesh.set_editor_property("static_materials", materials)
    unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)

    # verify
    materials = mesh.get_editor_property("static_materials")
    names = [sm.get_editor_property("material_interface").get_name() for sm in materials[: len(mats)]]
    unreal.log(f"{mesh_name} -> {names}")

# verify texture sizes
for tname in ["T_CorridorFloor_D", "T_CorridorHull_D"]:
    tex = unreal.EditorAssetLibrary.load_asset(f"/Game/Meshes/CorridorPanels/Textures/{tname}")
    unreal.log(f"{tname} size={tex.blueprint_get_size_x()}x{tex.blueprint_get_size_y()}")
