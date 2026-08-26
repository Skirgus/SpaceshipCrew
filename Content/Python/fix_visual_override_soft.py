import unreal

DEF_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01"
VO_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"
MESH_DIR = "/Game/Meshes/CorridorPanels"


def force_soft_override():
    definition = unreal.EditorAssetLibrary.load_asset(DEF_PATH)
    override = unreal.EditorAssetLibrary.load_asset(VO_PATH)
    if not definition or not override:
        unreal.log_error("Missing definition/override")
        return

    # SoftObjectPtr via SoftObjectPath — надежнее, чем прямая UObject-ссылка в Python.
    soft_path = unreal.SoftObjectPath(VO_PATH + "." + "Corridor_CustomPanels_01_VisualOverride")
    try:
        definition.set_editor_property("visual_override", soft_path)
    except Exception as exc:
        unreal.log_warning(f"soft path assign failed: {exc}; fallback to object")
        definition.set_editor_property("visual_override", override)

    definition.set_editor_property("has_interior", True)
    unreal.EditorAssetLibrary.save_asset(definition.get_path_name(), only_if_is_dirty=False)

    # Reload from disk
    unreal.EditorAssetLibrary.load_asset(DEF_PATH)
    definition = unreal.EditorAssetLibrary.load_asset(DEF_PATH)

    vo_prop = definition.get_editor_property("visual_override")
    unreal.log(f"After save VisualOverride prop={vo_prop}")

    # C++ getter
    loaded = None
    if hasattr(definition, "get_visual_override"):
        loaded = definition.get_visual_override()
    unreal.log(f"After save GetVisualOverride()={loaded}")

    # Manual soft load equivalent
    try:
        if hasattr(vo_prop, "load_synchronous"):
            unreal.log(f"prop.load_synchronous()={vo_prop.load_synchronous()}")
        elif hasattr(vo_prop, "to_soft_object_path"):
            unreal.log(f"prop soft path={vo_prop.to_soft_object_path()}")
    except Exception as exc:
        unreal.log_warning(f"soft inspect: {exc}")

    if loaded is None and override is not None:
        # Last resort: hard-set again as object and mark package dirty
        definition.set_editor_property("visual_override", override)
        unreal.EditorAssetLibrary.save_loaded_asset(definition)
        unreal.log("Forced object assign + save_loaded_asset")
        loaded2 = definition.get_visual_override() if hasattr(definition, "get_visual_override") else None
        unreal.log(f"Retry GetVisualOverride()={loaded2}")


def ensure_simple_fallback_material():
    """Create an unmistakable non-grid material and assign to all panel meshes as slot0."""
    path = f"{MAT_DIR}/M_CorridorDebugSolid"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)

    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_CorridorDebugSolid", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew()
    )
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    color = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector, -200, 0
    )
    # Distinct gunmetal blue-gray so PIE can't be confused with WorldGrid
    color.set_editor_property("constant", unreal.LinearColor(0.12, 0.16, 0.20, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    metal = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -200, 120
    )
    metal.set_editor_property("r", 0.35)
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -200, 180
    )
    rough.set_editor_property("r", 0.55)
    unreal.MaterialEditingLibrary.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(mat.get_path_name(), only_if_is_dirty=False)
    return mat


def assign_debug_then_pbr():
    debug = ensure_simple_fallback_material()
    pbr = {
        "SM_Floor_400": unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/M_CorridorFloor_PBR") or debug,
        "SM_Ceiling_400": unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/M_CorridorHull_PBR") or debug,
        "SM_Wall_Solid_400x300": unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/M_CorridorHull_PBR") or debug,
        "SM_Wall_Door_FB_400x300": unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/M_CorridorHull_PBR") or debug,
        "SM_DoorFrame_Ring_FB": unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/M_CorridorTrim_PBR") or debug,
        "SM_Wall_Door_400x300": unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/M_CorridorHull_PBR") or debug,
        "SM_DoorFrame_Ring": unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/M_CorridorTrim_PBR") or debug,
    }

    for mesh_name, mat in pbr.items():
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        if not mesh:
            continue
        # Force ALL slots to a known-good material (eliminates WorldGrid fallback)
        slots = list(mesh.get_editor_property("static_materials") or [])
        if not slots:
            slots = [unreal.StaticMaterial()]
        for i in range(len(slots)):
            sm = slots[i]
            sm.set_editor_property("material_interface", mat)
            slots[i] = sm
        mesh.set_editor_property("static_materials", slots)
        try:
            for i in range(max(1, mesh.get_num_sections(0) if hasattr(mesh, "get_num_sections") else 1)):
                mesh.set_material(i, mat)
        except Exception:
            mesh.set_material(0, mat)
        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
        mi = mesh.get_editor_property("static_materials")[0].get_editor_property("material_interface")
        unreal.log(f"{mesh_name} slot0={mi.get_name() if mi else None}")


def main():
    unreal.log("=== fix_visual_override_soft start ===")
    force_soft_override()
    assign_debug_then_pbr()

    # Catalog note: PrimaryAsset may need rescan; log ModuleId
    definition = unreal.EditorAssetLibrary.load_asset(DEF_PATH)
    unreal.log(f"ModuleId={definition.get_editor_property('module_id')}")
    unreal.log("=== fix_visual_override_soft done ===")
    unreal.log("IMPORTANT: Stop PIE, reload assets or restart editor, place Corridor_CustomPanels_01")


if __name__ == "__main__":
    main()
