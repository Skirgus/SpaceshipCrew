import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"

ASSIGN = {
    "SM_Floor_400": "M_CorridorFloor_PBR",
    "SM_Ceiling_400": "M_CorridorHull_PBR",
    "SM_Wall_Solid_400x300": "M_CorridorHull_PBR",
    "SM_Wall_Door_400x300": "M_CorridorHull_PBR",
    "SM_Wall_Door_FB_400x300": "M_CorridorHull_PBR",
    "SM_DoorFrame_Ring": "M_CorridorTrim_PBR",
    "SM_DoorFrame_Ring_FB": "M_CorridorTrim_PBR",
}

for mesh_name, mat_name in ASSIGN.items():
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
    mat = unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/{mat_name}")
    if not mesh or not mat:
        unreal.log_error(f"Missing {mesh_name}/{mat_name}")
        continue
    try:
        mat.set_editor_property("used_with_instanced_static_meshes", True)
    except Exception:
        pass
    unreal.MaterialEditingLibrary.recompile_material(mat)
    sm = unreal.StaticMaterial()
    sm.set_editor_property("material_interface", mat)
    sm.set_editor_property("material_slot_name", unreal.Name("Element_0"))
    mesh.set_editor_property("static_materials", [sm])
    mesh.set_material(0, mat)
    unreal.EditorAssetLibrary.save_asset(mat.get_path_name(), only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
    flag = mat.get_editor_property("used_with_instanced_static_meshes")
    unreal.log(f"{mesh_name} -> {mat_name} ISM={flag}")
