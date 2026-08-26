import unreal

# Force floor+ceiling to the SAME material that already works on walls in PIE.
MESH_DIR = "/Game/Meshes/CorridorPanels"
hull = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/Materials/M_CorridorHull_PBR")
hull.set_editor_property("used_with_instanced_static_meshes", True)
unreal.MaterialEditingLibrary.recompile_material(hull)
unreal.EditorAssetLibrary.save_asset(hull.get_path_name(), only_if_is_dirty=False)

for name in ["SM_Floor_400", "SM_Ceiling_400"]:
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{name}")
    sm = unreal.StaticMaterial()
    sm.set_editor_property("material_interface", hull)
    sm.set_editor_property("material_slot_name", unreal.Name("Element_0"))
    mesh.set_editor_property("static_materials", [sm])
    mesh.set_material(0, hull)
    unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
    cur = mesh.get_material(0)
    unreal.log(f"{name} mat0={cur.get_name() if cur else None} ISM={hull.get_editor_property('used_with_instanced_static_meshes')}")
