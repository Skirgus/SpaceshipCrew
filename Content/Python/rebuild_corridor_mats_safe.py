# Rebuild corridor materials with safer PBR (less chrome blowout) and explicit UV tiling.
import unreal

TEX = "/Game/Meshes/CorridorPanels/Textures"
MAT = "/Game/Meshes/CorridorPanels/Materials"
MESH_DIR = "/Game/Meshes/CorridorPanels"

MESH_MATS = {
    "SM_Floor_400": ["M_CorridorFloor_PBR", "M_CorridorTrim_PBR"],
    "SM_Ceiling_400": ["M_CorridorHull_PBR", "M_CorridorGlow_PBR"],
    "SM_Wall_Solid_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR"],
    "SM_Wall_Door_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_Wall_Door_FB_400x300": ["M_CorridorHull_PBR", "M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_DoorFrame_Ring": ["M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
    "SM_DoorFrame_Ring_FB": ["M_CorridorTrim_PBR", "M_CorridorGlow_PBR"],
}


def load_tex(name):
    tex = unreal.EditorAssetLibrary.load_asset(f"{TEX}/{name}")
    # Prefer source size reporting
    try:
        imported = tex.get_editor_property("imported_size")
        unreal.log(f"{name} imported_size={imported} bp={tex.blueprint_get_size_x()}x{tex.blueprint_get_size_y()}")
    except Exception:
        unreal.log(f"{name} bp={tex.blueprint_get_size_x()}x{tex.blueprint_get_size_y()}")
    try:
        tex.set_editor_property("never_stream", True)
        # Ensure full res available in editor
        tex.set_editor_property("lod_bias", 0)
    except Exception:
        pass
    unreal.EditorAssetLibrary.save_asset(tex.get_path_name(), only_if_is_dirty=False)
    return tex


def make_material(asset_name, albedo_name, normal_name=None, metallic=0.35, roughness=0.6, emissive_name=None, emissive_strength=0.0, tile=2.0):
    path = f"{MAT}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)

    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, MAT, unreal.Material, unreal.MaterialFactoryNew()
    )
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)

    # UV tile
    texcoord = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionTextureCoordinate, -700, -40
    )
    mul_uv = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, -520, -40
    )
    tile_c = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -700, 40
    )
    tile_c.set_editor_property("r", float(tile))
    unreal.MaterialEditingLibrary.connect_material_expressions(texcoord, "", mul_uv, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(tile_c, "", mul_uv, "B")

    def sample(tex_name, y, sampler):
        node = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionTextureSample, -300, y
        )
        node.set_editor_property("texture", load_tex(tex_name))
        node.set_editor_property("sampler_type", sampler)
        unreal.MaterialEditingLibrary.connect_material_expressions(mul_uv, "", node, "UVs")
        return node

    albedo = sample(albedo_name, -120, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(
        albedo, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )

    if normal_name:
        nrm = sample(normal_name, 80, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        unreal.MaterialEditingLibrary.connect_material_property(
            nrm, "RGB", unreal.MaterialProperty.MP_NORMAL
        )

    # Stable constants instead of packed ORM (avoids chrome blowout)
    metal = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -120, 260
    )
    metal.set_editor_property("r", float(metallic))
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -120, 320
    )
    rough.set_editor_property("r", float(roughness))
    unreal.MaterialEditingLibrary.connect_material_property(
        metal, "", unreal.MaterialProperty.MP_METALLIC
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )

    if emissive_name and emissive_strength > 0.0:
        em = sample(emissive_name, 400, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        mul = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionMultiply, -40, 400
        )
        strength = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant, -200, 460
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
    unreal.log(f"Built {asset_name} metal={metallic} rough={roughness} tile={tile}")
    return mat


def main():
    unreal.log("=== rebuild_corridor_mats_safe start ===")
    mats = {
        "M_CorridorHull_PBR": make_material(
            "M_CorridorHull_PBR", "T_CorridorHull_D", "T_CorridorHull_N",
            metallic=0.45, roughness=0.55, tile=2.0,
        ),
        "M_CorridorFloor_PBR": make_material(
            "M_CorridorFloor_PBR", "T_CorridorFloor_D", "T_CorridorFloor_N",
            metallic=0.25, roughness=0.75, tile=2.0,
        ),
        "M_CorridorTrim_PBR": make_material(
            "M_CorridorTrim_PBR", "T_CorridorTrim_D", "T_CorridorTrim_N",
            metallic=0.7, roughness=0.35, tile=4.0,
        ),
        "M_CorridorGlow_PBR": make_material(
            "M_CorridorGlow_PBR", "T_CorridorHull_D", "T_CorridorHull_N",
            metallic=0.4, roughness=0.5, emissive_name="T_CorridorGlow_E",
            emissive_strength=3.0, tile=2.0,
        ),
    }

    for mesh_name, mat_names in MESH_MATS.items():
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        materials = list(mesh.get_editor_property("static_materials") or [])
        while len(materials) < len(mat_names):
            materials.append(unreal.StaticMaterial())
        for i, mat_name in enumerate(mat_names):
            sm = materials[i]
            sm.set_editor_property("material_interface", mats[mat_name])
            materials[i] = sm
        mesh.set_editor_property("static_materials", materials)
        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
        unreal.log(f"Assigned {mesh_name}")

    unreal.log("=== rebuild_corridor_mats_safe done ===")


if __name__ == "__main__":
    main()
