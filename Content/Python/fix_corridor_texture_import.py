# Force-reimport corridor textures at full resolution and rebuild materials.
import unreal

TEX_DIR = "/Game/Meshes/CorridorPanels/Textures"
MAT_DIR = "/Game/Meshes/CorridorPanels/Materials"
SRC = unreal.Paths.project_content_dir() + "Meshes/CorridorPanels/Textures/Source/"
MESH_DIR = "/Game/Meshes/CorridorPanels"

TEXTURES = {
    "T_CorridorHull_D": "Color",
    "T_CorridorHull_N": "Normal",
    "T_CorridorHull_ORM": "Masks",
    "T_CorridorFloor_D": "Color",
    "T_CorridorFloor_N": "Normal",
    "T_CorridorFloor_ORM": "Masks",
    "T_CorridorTrim_D": "Color",
    "T_CorridorTrim_N": "Normal",
    "T_CorridorTrim_ORM": "Masks",
    "T_CorridorGlow_E": "Color",
}

MESH_MATS = {
    "SM_Floor_400": ["M_CorridorFloor_PBR", "M_CorridorTrim_PBR"],
    "SM_Ceiling_400": ["M_CorridorHull_PBR", "M_CorridorGlow_PBR"],
    "SM_Wall_Solid_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR"],
    "SM_Wall_Door_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_Wall_Door_FB_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_DoorFrame_Ring": ["M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_DoorFrame_Ring_FB": ["M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
}


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def force_import_texture(name, kind):
    png = SRC + name + ".png"
    dest_path = f"{TEX_DIR}/{name}"
    if not unreal.Paths.file_exists(png):
        unreal.log_error(f"Missing source {png}")
        return None

    # Delete broken stub if present
    if unreal.EditorAssetLibrary.does_asset_exist(dest_path):
        unreal.EditorAssetLibrary.delete_asset(dest_path)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", png)
    task.set_editor_property("destination_path", TEX_DIR)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)

    factory = unreal.TextureFactory()
    try:
        factory.set_editor_property("create_material", False)
    except Exception:
        pass
    task.set_editor_property("factory", factory)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex = unreal.EditorAssetLibrary.load_asset(dest_path)
    if not tex:
        unreal.log_error(f"Import failed {name}")
        return None

    if kind == "Normal":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif kind == "Masks":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    else:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("srgb", True)

    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    # Force never stream tiny mips for editor preview reliability
    try:
        tex.set_editor_property("never_stream", True)
    except Exception:
        pass

    unreal.EditorAssetLibrary.save_asset(tex.get_path_name(), only_if_is_dirty=False)
    sx = tex.blueprint_get_size_x()
    sy = tex.blueprint_get_size_y()
    unreal.log(f"Imported {name} size={sx}x{sy}")
    if sx < 256 or sy < 256:
        unreal.log_error(f"Texture still too small after import: {name} ({sx}x{sy})")
    return tex


def rebuild_material(asset_name, albedo, normal=None, orm=None, emissive=None, emissive_strength=0.0):
    ensure_dir(MAT_DIR)
    path = f"{MAT_DIR}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)

    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew()
    )
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)

    def tex_sample(texture, y, sampler=None):
        node = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionTextureSample, -450, y
        )
        node.set_editor_property("texture", texture)
        if sampler is not None:
            try:
                node.set_editor_property("sampler_type", sampler)
            except Exception:
                pass
        return node

    albedo_n = tex_sample(albedo, -200, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    ok = unreal.MaterialEditingLibrary.connect_material_property(
        albedo_n, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.log(f"{asset_name} connect BaseColor={ok}")

    if normal:
        normal_n = tex_sample(normal, 0, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        ok = unreal.MaterialEditingLibrary.connect_material_property(
            normal_n, "RGB", unreal.MaterialProperty.MP_NORMAL
        )
        unreal.log(f"{asset_name} connect Normal={ok}")

    if orm:
        orm_n = tex_sample(orm, 200, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_n, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_n, "G", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_n, "B", unreal.MaterialProperty.MP_METALLIC
        )

    if emissive and emissive_strength > 0.0:
        em = tex_sample(emissive, 400, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        mul = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionMultiply, -80, 400
        )
        strength = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant, -250, 460
        )
        strength.set_editor_property("r", float(emissive_strength))
        unreal.MaterialEditingLibrary.connect_material_expressions(em, "RGB", mul, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(strength, "", mul, "B")
        unreal.MaterialEditingLibrary.connect_material_property(
            mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(mat.get_path_name(), only_if_is_dirty=False)
    unreal.log(f"Rebuilt {path}")
    return mat


def assign_mesh_materials(mat_map):
    for mesh_name, mat_names in MESH_MATS.items():
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        if not mesh:
            continue
        materials = list(mesh.get_editor_property("static_materials") or [])
        # Grow slot list if needed
        while len(materials) < len(mat_names):
            materials.append(unreal.StaticMaterial())
        for i, mat_name in enumerate(mat_names):
            mat = mat_map[mat_name]
            sm = materials[i]
            sm.set_editor_property("material_interface", mat)
            try:
                sm.set_editor_property("material_slot_name", unreal.Name(f"Element_{i}"))
            except Exception:
                pass
            materials[i] = sm
        mesh.set_editor_property("static_materials", materials)
        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
        unreal.log(f"Reassigned {mesh_name}")


def main():
    unreal.log("=== fix_corridor_texture_import start ===")
    ensure_dir(TEX_DIR)
    ensure_dir(MAT_DIR)

    tex = {}
    for name, kind in TEXTURES.items():
        t = force_import_texture(name, kind)
        if t:
            tex[name] = t

    needed = list(TEXTURES.keys())
    missing = [n for n in needed if n not in tex]
    if missing:
        unreal.log_error(f"Missing after import: {missing}")
        return

    mats = {
        "M_CorridorHull_PBR": rebuild_material(
            "M_CorridorHull_PBR",
            tex["T_CorridorHull_D"],
            tex["T_CorridorHull_N"],
            tex["T_CorridorHull_ORM"],
        ),
        "M_CorridorFloor_PBR": rebuild_material(
            "M_CorridorFloor_PBR",
            tex["T_CorridorFloor_D"],
            tex["T_CorridorFloor_N"],
            tex["T_CorridorFloor_ORM"],
        ),
        "M_CorridorTrim_PBR": rebuild_material(
            "M_CorridorTrim_PBR",
            tex["T_CorridorTrim_D"],
            tex["T_CorridorTrim_N"],
            tex["T_CorridorTrim_ORM"],
        ),
        "M_CorridorGlow_PBR": rebuild_material(
            "M_CorridorGlow_PBR",
            tex["T_CorridorHull_D"],
            tex["T_CorridorHull_N"],
            tex["T_CorridorHull_ORM"],
            emissive=tex["T_CorridorGlow_E"],
            emissive_strength=4.0,
        ),
    }
    assign_mesh_materials(mats)
    unreal.log("=== fix_corridor_texture_import done ===")


if __name__ == "__main__":
    main()
