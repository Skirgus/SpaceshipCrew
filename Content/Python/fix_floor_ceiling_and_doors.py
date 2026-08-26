# Fix floor/ceiling ISM materials + rebuild VisualOverride without permanent doors.
import unreal
import math

MESH_DIR = "/Game/Meshes/CorridorPanels"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"
VO_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride"
DEF_PATH = "/Game/Data/ShipModules/Corridor_CustomPanels_01"


def enable_ism(mat):
    try:
        mat.set_editor_property("used_with_instanced_static_meshes", True)
    except Exception:
        pass
    try:
        mat.set_editor_property("used_with_static_lighting", True)
    except Exception:
        pass
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(mat.get_path_name(), only_if_is_dirty=False)


def assign_mat(mesh_name, mat_name):
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
    mat = unreal.EditorAssetLibrary.load_asset(f"{MAT_DIR}/{mat_name}")
    if not mesh or not mat:
        unreal.log_error(f"Missing {mesh_name} / {mat_name}")
        return None
    enable_ism(mat)
    sm = unreal.StaticMaterial()
    sm.set_editor_property("material_interface", mat)
    sm.set_editor_property("material_slot_name", unreal.Name("Element_0"))
    mesh.set_editor_property("static_materials", [sm])
    mesh.set_material(0, mat)
    unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
    cur = mesh.get_editor_property("static_materials")[0].get_editor_property("material_interface")
    flag = mat.get_editor_property("used_with_instanced_static_meshes")
    unreal.log(f"{mesh_name} -> {cur.get_name()} ISM={flag}")
    return mesh


def yaw_transform(location, yaw_deg=0.0):
    tr = unreal.Transform()
    tr.translation = unreal.Vector(*location)
    tr.rotation = unreal.Rotator(0.0, yaw_deg, 0.0).quaternion()
    tr.scale3d = unreal.Vector(1.0, 1.0, 1.0)
    return tr


def make_part(mesh, location, yaw_deg=0.0):
    part = unreal.ShipModuleVisualPart()
    part.set_editor_property("mesh", mesh)
    part.set_editor_property("relative_transform", yaw_transform(location, yaw_deg))
    return part


def main():
    unreal.log("=== fix_floor_ceiling_and_doors start ===")

    # 1) Floor/ceiling (and walls for consistency) — textured PBR + ISM
    floor = assign_mat("SM_Floor_400", "M_CorridorFloor_PBR")
    ceiling = assign_mat("SM_Ceiling_400", "M_CorridorHull_PBR")
    wall = assign_mat("SM_Wall_Solid_400x300", "M_CorridorHull_PBR")
    assign_mat("SM_Wall_Door_FB_400x300", "M_CorridorHull_PBR")  # keep asset valid, but don't use in VO
    assign_mat("SM_DoorFrame_Ring_FB", "M_CorridorTrim_PBR")

    if not all([floor, ceiling, wall]):
        unreal.log_error("Required meshes missing")
        return

    # 2) VisualOverride: closed shell, NO door meshes.
    #    Docking openings are not auto-cut for VisualOverride yet (procedural shell only).
    override = unreal.EditorAssetLibrary.load_asset(VO_PATH)
    definition = unreal.EditorAssetLibrary.load_asset(DEF_PATH)
    if not override or not definition:
        unreal.log_error("Missing definition/override")
        return

    parts = [
        # Floor / ceiling
        make_part(floor, (0.0, 0.0, -142.0), 0.0),
        make_part(ceiling, (0.0, 0.0, 141.0), 0.0),
        # Four solid walls (Left, Right, Front, Back)
        # Solid wall mesh is thin in Y; detail on +Y.
        make_part(wall, (0.0, -190.0, 0.0), 0.0),      # Left
        make_part(wall, (0.0, 190.0, 0.0), 180.0),     # Right
        # Front/Back: rotate solid wall 90° so thin axis is X.
        # Use only yaw 0/180 on FB-oriented mesh if available; else yaw 90 on LR mesh.
        # Prefer dedicated FB solid if we had one; rotate LR wall with quat via 90 yaw
        # (known pitch quirk for 90° — build FB solid from door mesh? Use wall + 90 via two 180? )
    ]

    # For Front/Back use the door FB mesh orientation (thin in X) but we want SOLID.
    # Create solid front/back by using wall with yaw 90 — may have euler quirk.
    # Safer: use SM_Wall_Door_FB geometry? No openings wanted.
    # Rotate solid wall: place with explicit quaternion around Z.
    def make_part_quat(mesh, location, yaw_deg):
        part = unreal.ShipModuleVisualPart()
        part.set_editor_property("mesh", mesh)
        rad = math.radians(yaw_deg)
        # FQuat(X,Y,Z,W) for yaw around Z
        q = unreal.Quat(0.0, 0.0, math.sin(rad * 0.5), math.cos(rad * 0.5))
        tr = unreal.Transform()
        tr.translation = unreal.Vector(*location)
        tr.rotation = q
        tr.scale3d = unreal.Vector(1.0, 1.0, 1.0)
        part.set_editor_property("relative_transform", tr)
        return part

    parts.append(make_part_quat(wall, (191.0, 0.0, 0.0), 90.0))   # Front
    parts.append(make_part_quat(wall, (-191.0, 0.0, 0.0), -90.0))  # Back

    override.set_editor_property("visual_parts", parts)
    unreal.EditorAssetLibrary.save_asset(override.get_path_name(), only_if_is_dirty=False)

    definition.set_editor_property("visual_override", override)
    definition.set_editor_property("has_interior", True)
    unreal.EditorAssetLibrary.save_loaded_asset(definition)
    unreal.EditorAssetLibrary.save_loaded_asset(override)

    saved = override.get_editor_property("visual_parts") or []
    unreal.log(f"VisualParts now={len(saved)} (expected 6: floor,ceil,4 walls; no doors)")
    for i, p in enumerate(saved):
        m = p.get_editor_property("mesh")
        unreal.log(f"  [{i}] {m.get_name() if m else None}")

    unreal.log("=== fix_floor_ceiling_and_doors done ===")
    unreal.log("NOTE: openings at dock joints are NOT auto-generated for VisualOverride modules yet.")


if __name__ == "__main__":
    main()
