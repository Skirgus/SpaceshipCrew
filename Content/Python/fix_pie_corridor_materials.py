import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"
DEF = "/Game/Data/ShipModules/Corridor_CustomPanels_01"
VO = "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride"

MESH_MATS = {
    "SM_Floor_400": ["M_CorridorFloor_PBR", "M_CorridorTrim_PBR"],
    "SM_Ceiling_400": ["M_CorridorHull_PBR", "M_CorridorGlow_PBR"],
    "SM_Wall_Solid_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR"],
    "SM_Wall_Door_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_Wall_Door_FB_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_DoorFrame_Ring": ["M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_DoorFrame_Ring_FB": ["M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
}


def main():
    unreal.log("=== diagnose_pie_look start ===")

    # Definition / override soft refs
    definition = unreal.EditorAssetLibrary.load_asset(DEF)
    override = unreal.EditorAssetLibrary.load_asset(VO)
    unreal.log(f"Definition={definition}")
    unreal.log(f"Override asset={override}")
    if definition:
        vo_soft = definition.get_editor_property("visual_override")
        unreal.log(f"Def.VisualOverride soft={vo_soft}")
        loaded = definition.get_visual_override() if hasattr(definition, "get_visual_override") else None
        unreal.log(f"Def.GetVisualOverride()={loaded}")
        has_interior = definition.get_editor_property("has_interior")
        unreal.log(f"HasInterior={has_interior} ModuleId={definition.get_editor_property('module_id')}")

    if override:
        parts = override.get_editor_property("visual_parts") or []
        unreal.log(f"VisualParts={len(parts)}")
        for i, part in enumerate(parts):
            mesh = part.get_editor_property("mesh")
            unreal.log(f"  Part[{i}] mesh={mesh.get_path_name() if mesh else None}")

    # Material validity
    for mat_name in [
        "M_CorridorHull_PBR",
        "M_CorridorFloor_PBR",
        "M_CorridorTrim_PBR",
        "M_CorridorGlow_PBR",
    ]:
        mat = unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/{mat_name}")
        unreal.log(f"Material {mat_name}: {'OK' if mat else 'MISSING'}")

    # Mesh slots BEFORE fix
    for mesh_name in MESH_MATS:
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        if not mesh:
            unreal.log_error(f"Missing mesh {mesh_name}")
            continue
        slots = mesh.get_editor_property("static_materials") or []
        unreal.log(f"{mesh_name} slots={len(slots)}")
        for i, sm in enumerate(slots):
            mi = sm.get_editor_property("material_interface")
            unreal.log(f"  BEFORE [{i}] -> {mi.get_path_name() if mi else 'NONE'}")

    # FORCE reassign valid materials
    for mesh_name, mat_names in MESH_MATS.items():
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        mats = [unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/{n}") for n in mat_names]
        if any(m is None for m in mats):
            unreal.log_error(f"Cannot assign {mesh_name}, missing materials")
            continue

        # Replace materials array cleanly
        new_slots = []
        for i, mat in enumerate(mats):
            sm = unreal.StaticMaterial()
            sm.set_editor_property("material_interface", mat)
            # Keep slot name stable for section mapping
            sm.set_editor_property("material_slot_name", unreal.Name(f"Element_{i}"))
            new_slots.append(sm)
        mesh.set_editor_property("static_materials", new_slots)

        # Also push via mesh API if available
        for i, mat in enumerate(mats):
            try:
                mesh.set_material(i, mat)
            except Exception as exc:
                unreal.log_warning(f"set_material {mesh_name}[{i}]: {exc}")

        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)

        slots = mesh.get_editor_property("static_materials") or []
        for i, sm in enumerate(slots):
            mi = sm.get_editor_property("material_interface")
            unreal.log(f"  AFTER  [{i}] -> {mi.get_path_name() if mi else 'NONE'}")

    # Ensure definition points at override
    if definition and override:
        definition.set_editor_property("visual_override", override)
        definition.set_editor_property("has_interior", True)
        unreal.EditorAssetLibrary.save_asset(definition.get_path_name(), only_if_is_dirty=False)
        unreal.EditorAssetLibrary.save_asset(override.get_path_name(), only_if_is_dirty=False)
        unreal.log("Definition VisualOverride re-saved")

    unreal.log("=== diagnose_pie_look done ===")


if __name__ == "__main__":
    main()
