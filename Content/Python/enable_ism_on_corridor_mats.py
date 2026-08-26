# Enable "Used with Instanced Static Meshes" on corridor materials and recompile.
import unreal

MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"
MESH_DIR = "/Game/Meshes/CorridorPanels"

MAT_NAMES = [
    "M_CorridorHull_PBR",
    "M_CorridorFloor_PBR",
    "M_CorridorTrim_PBR",
    "M_CorridorGlow_PBR",
    "M_CorridorSolid_Floor",
    "M_CorridorSolid_Hull",
    "M_CorridorSolid_Trim",
]


def enable_ism(mat):
    # UE5 property names vary slightly; try common ones.
    for prop, value in [
        ("used_with_instanced_static_meshes", True),
        ("b_used_with_instanced_static_meshes", True),
        ("used_with_mesh_particles", True),
        ("used_with_static_lighting", True),
        ("used_with_skeletal_mesh", False),
    ]:
        try:
            mat.set_editor_property(prop, value)
            unreal.log(f"  set {prop}={value}")
        except Exception:
            pass


def main():
    unreal.log("=== enable_ism_on_corridor_mats start ===")

    # Also fix whatever material is currently on the wall mesh (user sees M_C_PBR)
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Wall_Solid_400x300")
    if mesh:
        slots = mesh.get_editor_property("static_materials") or []
        for sm in slots:
            mi = sm.get_editor_property("material_interface")
            if mi:
                unreal.log(f"Wall current mat={mi.get_path_name()} class={mi.get_class().get_name()}")
                # If MIC, also fix parent
                try:
                    parent = mi.get_editor_property("parent")
                    if parent:
                        unreal.log(f"  parent={parent.get_path_name()}")
                        enable_ism(parent)
                        unreal.MaterialEditingLibrary.recompile_material(parent)
                        unreal.EditorAssetLibrary.save_asset(parent.get_path_name(), only_if_is_dirty=False)
                except Exception:
                    pass
                if mi.get_class().get_name() == "Material":
                    enable_ism(mi)
                    unreal.MaterialEditingLibrary.recompile_material(mi)
                    unreal.EditorAssetLibrary.save_asset(mi.get_path_name(), only_if_is_dirty=False)

    for name in MAT_NAMES:
        path = f"{MAT_DIR}/{name}"
        mat = unreal.EditorAssetLibrary.load_asset(path)
        if not mat:
            unreal.log_warning(f"Missing {path}")
            continue
        unreal.log(f"Fixing {name}")
        enable_ism(mat)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_asset(mat.get_path_name(), only_if_is_dirty=False)

        # Verify flag
        try:
            flag = mat.get_editor_property("used_with_instanced_static_meshes")
            unreal.log(f"  used_with_instanced_static_meshes={flag}")
        except Exception as exc:
            unreal.log_warning(f"  verify failed: {exc}")

    # Re-assign known good PBR mats onto meshes (in case solids replaced them)
    assign = {
        "SM_Floor_400": "M_CorridorFloor_PBR",
        "SM_Ceiling_400": "M_CorridorHull_PBR",
        "SM_Wall_Solid_400x300": "M_CorridorHull_PBR",
        "SM_Wall_Door_400x300": "M_CorridorHull_PBR",
        "SM_Wall_Door_FB_400x300": "M_CorridorHull_PBR",
        "SM_DoorFrame_Ring": "M_CorridorTrim_PBR",
        "SM_DoorFrame_Ring_FB": "M_CorridorTrim_PBR",
    }
    for mesh_name, mat_name in assign.items():
        m = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        mat = unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/{mat_name}")
        if not m or not mat:
            continue
        # Prefer currently assigned textured mat if it already looks good and is named M_C*
        slots = list(m.get_editor_property("static_materials") or [])
        current = None
        if slots:
            current = slots[0].get_editor_property("material_interface")
        use_mat = mat
        if current and "M_C" in current.get_name() and current.get_name() != mat_name:
            # Keep user's currently working textured material (e.g. M_C_PBR)
            use_mat = current
            unreal.log(f"Keeping existing textured mat on {mesh_name}: {use_mat.get_name()}")
            if use_mat.get_class().get_name() == "Material":
                enable_ism(use_mat)
                unreal.MaterialEditingLibrary.recompile_material(use_mat)
                unreal.EditorAssetLibrary.save_asset(use_mat.get_path_name(), only_if_is_dirty=False)
        sm = unreal.StaticMaterial()
        sm.set_editor_property("material_interface", use_mat)
        sm.set_editor_property("material_slot_name", unreal.Name("Element_0"))
        m.set_editor_property("static_materials", [sm])
        m.set_material(0, use_mat)
        unreal.EditorAssetLibrary.save_asset(m.get_path_name(), only_if_is_dirty=False)
        unreal.log(f"{mesh_name} -> {use_mat.get_name()}")

    unreal.log("=== enable_ism_on_corridor_mats done ===")


if __name__ == "__main__":
    main()
