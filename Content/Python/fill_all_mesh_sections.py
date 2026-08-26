import unreal

MESH_DIR = "/Game/Meshes/CorridorPanels"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"

# Primary material per mesh; extra sections get the same (no WorldGrid holes).
MESH_PRIMARY = {
    "SM_Floor_400": "M_CorridorFloor_PBR",
    "SM_Ceiling_400": "M_CorridorHull_PBR",
    "SM_Wall_Solid_400x300": "M_CorridorHull_PBR",
    "SM_Wall_Door_400x300": "M_CorridorHull_PBR",
    "SM_Wall_Door_FB_400x300": "M_CorridorHull_PBR",
    "SM_DoorFrame_Ring": "M_CorridorTrim_PBR",
    "SM_DoorFrame_Ring_FB": "M_CorridorTrim_PBR",
}


def section_count(mesh):
    # Prefer LOD0 section count
    try:
        return int(mesh.get_num_sections(0))
    except Exception:
        pass
    try:
        return len(mesh.get_editor_property("static_materials") or [])
    except Exception:
        return 1


def main():
    unreal.log("=== fill_all_mesh_sections start ===")
    for mesh_name, mat_name in MESH_PRIMARY.items():
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        mat = unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/{mat_name}")
        if not mesh or not mat:
            unreal.log_error(f"Missing {mesh_name} or {mat_name}")
            continue

        n = max(1, section_count(mesh))
        unreal.log(f"{mesh_name} sections={n}")

        slots = []
        for i in range(n):
            sm = unreal.StaticMaterial()
            sm.set_editor_property("material_interface", mat)
            sm.set_editor_property("material_slot_name", unreal.Name(f"Element_{i}"))
            slots.append(sm)
        mesh.set_editor_property("static_materials", slots)

        for i in range(n):
            try:
                mesh.set_material(i, mat)
            except Exception as exc:
                unreal.log_warning(f"set_material[{i}] {exc}")

        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)

        # Verify no nulls
        after = mesh.get_editor_property("static_materials") or []
        nulls = sum(1 for sm in after if not sm.get_editor_property("material_interface"))
        names = [sm.get_editor_property("material_interface").get_name() for sm in after if sm.get_editor_property("material_interface")]
        unreal.log(f"  slots={len(after)} nulls={nulls} mats={names}")

    # Soft override sanity
    definition = unreal.EditorAssetLibrary.load_asset("/Game/Data/ShipModules/Corridor_CustomPanels_01")
    override = unreal.EditorAssetLibrary.load_asset("/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride")
    definition.set_editor_property("visual_override", override)
    unreal.EditorAssetLibrary.save_loaded_asset(definition)
    vo = definition.get_visual_override() if hasattr(definition, "get_visual_override") else None
    unreal.log(f"GetVisualOverride={vo.get_name() if vo else None} parts={(vo.get_editor_property('visual_parts') if vo else None)}")
    unreal.log("=== fill_all_mesh_sections done ===")


if __name__ == "__main__":
    main()
