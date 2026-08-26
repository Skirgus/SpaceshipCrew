# Verify / fix SM_Wall_Solid_FB import scale vs Door_FB.
import unreal
import os

MESH_DIR = "/Game/Meshes/CorridorPanels"
FBX_DIR = unreal.Paths.project_content_dir() + "Meshes/CorridorPanels/"


def extent(mesh):
    e = mesh.get_bounds().box_extent * 2.0
    return (e.x, e.y, e.z)


def import_mesh(mesh_name: str, uniform_scale: float):
    path = f"{MESH_DIR}/{mesh_name}"
    fbx_path = FBX_DIR + mesh_name + ".fbx"
    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = MESH_DIR
    task.destination_name = mesh_name
    task.replace_existing = True
    task.automated = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = False
    options.import_textures = False
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
    options.static_mesh_import_data.set_editor_property("import_uniform_scale", uniform_scale)
    options.static_mesh_import_data.set_editor_property("convert_scene", True)
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if mesh:
        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
    return mesh


def main():
    door_fb = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Wall_Door_FB_400x300")
    solid_fb = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_Wall_Solid_FB_400x300")
    unreal.log(f"Door_FB extent={extent(door_fb) if door_fb else None}")
    unreal.log(f"Solid_FB extent={extent(solid_fb) if solid_fb else None}")

    if not solid_fb or not door_fb:
        unreal.log_error("Missing meshes")
        return

    de = extent(door_fb)
    se = extent(solid_fb)
    # Door_FB should be ~18 x 400 x 300 cm. If Solid_FB is ~100x smaller, reimport scale=1.
    if max(se) < max(de) * 0.5:
        unreal.log("Solid_FB too small — reimport with uniform_scale=1.0")
        solid_fb = import_mesh("SM_Wall_Solid_FB_400x300", 1.0)
        se = extent(solid_fb)
        unreal.log(f"Solid_FB after scale=1 extent={se}")
        if max(se) < max(de) * 0.5:
            unreal.log("Still small — try uniform_scale=100")
            solid_fb = import_mesh("SM_Wall_Solid_FB_400x300", 100.0)
            se = extent(solid_fb)
            unreal.log(f"Solid_FB after scale=100 extent={se}")

    mat = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/Materials/M_CorridorHull_PBR")
    if mat and solid_fb:
        try:
            mat.set_editor_property("used_with_instanced_static_meshes", True)
        except Exception:
            pass
        solid_fb.set_material(0, mat)
        unreal.EditorAssetLibrary.save_asset(solid_fb.get_path_name(), only_if_is_dirty=False)
    unreal.log("=== fix_solid_fb_scale done ===")


if __name__ == "__main__":
    main()
