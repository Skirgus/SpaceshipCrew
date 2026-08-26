# Assign simple solid materials (no textures) so PIE cannot fall back to WorldGrid.
import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"
DEF = "/Game/Data/ShipModules/Corridor_CustomPanels_01"
VO = "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride"

COLORS = {
    "M_CorridorSolid_Floor": unreal.LinearColor(0.08, 0.09, 0.10, 1.0),
    "M_CorridorSolid_Hull": unreal.LinearColor(0.18, 0.22, 0.28, 1.0),
    "M_CorridorSolid_Trim": unreal.LinearColor(0.45, 0.52, 0.58, 1.0),
}

MESH_MAT = {
    "SM_Floor_400": "M_CorridorSolid_Floor",
    "SM_Ceiling_400": "M_CorridorSolid_Hull",
    "SM_Wall_Solid_400x300": "M_CorridorSolid_Hull",
    "SM_Wall_Door_400x300": "M_CorridorSolid_Hull",
    "SM_Wall_Door_FB_400x300": "M_CorridorSolid_Hull",
    "SM_DoorFrame_Ring": "M_CorridorSolid_Trim",
    "SM_DoorFrame_Ring_FB": "M_CorridorSolid_Trim",
}


def make_solid(name, color):
    path = f"{MAT_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew()
    )
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    c = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector, -250, 0
    )
    c.set_editor_property("constant", color)
    unreal.MaterialEditingLibrary.connect_material_property(
        c, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    metal = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -250, 120
    )
    metal.set_editor_property("r", 0.4)
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -250, 180
    )
    rough.set_editor_property("r", 0.6)
    unreal.MaterialEditingLibrary.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(mat.get_path_name(), only_if_is_dirty=False)
    unreal.log(f"Created {path}")
    return mat


def main():
    unreal.log("=== apply_solid_corridor_mats start ===")
    mats = {name: make_solid(name, color) for name, color in COLORS.items()}

    for mesh_name, mat_name in MESH_MAT.items():
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        mat = mats[mat_name]
        sm = unreal.StaticMaterial()
        sm.set_editor_property("material_interface", mat)
        sm.set_editor_property("material_slot_name", unreal.Name("Element_0"))
        mesh.set_editor_property("static_materials", [sm])
        mesh.set_material(0, mat)
        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
        unreal.log(f"{mesh_name} -> {mat_name}")

    definition = unreal.EditorAssetLibrary.load_asset(DEF)
    override = unreal.EditorAssetLibrary.load_asset(VO)
    definition.set_editor_property("visual_override", override)
    definition.set_editor_property("has_interior", True)
    unreal.EditorAssetLibrary.save_loaded_asset(definition)
    unreal.EditorAssetLibrary.save_loaded_asset(override)

    parts = override.get_editor_property("visual_parts") or []
    unreal.log(f"VO parts={len(parts)} DefVO={definition.get_editor_property('visual_override')}")
    unreal.log("=== apply_solid_corridor_mats done ===")
    unreal.log("RESTART EDITOR or File->Refresh Visual Studio / reload content, then PIE again with Corridor_CustomPanels_01")


if __name__ == "__main__":
    main()
