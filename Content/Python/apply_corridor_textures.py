# Import corridor textures, build materials, assign to panel meshes.
import unreal

TEX_CONTENT = "/Game/Meshes/CorridorPanels/Textures"
MAT_CONTENT = "/Game/Meshes/CorridorPanels/Materials"
SRC_DIR = unreal.Paths.project_content_dir() + "Meshes/CorridorPanels/Textures/Source/"
MESH_DIR = "/Game/Meshes/CorridorPanels"

TEXTURE_FILES = [
    ("T_CorridorHull_D", "Diffuse"),
    ("T_CorridorHull_N", "Normal"),
    ("T_CorridorHull_ORM", "Masks"),
    ("T_CorridorFloor_D", "Diffuse"),
    ("T_CorridorFloor_N", "Normal"),
    ("T_CorridorFloor_ORM", "Masks"),
    ("T_CorridorTrim_D", "Diffuse"),
    ("T_CorridorTrim_N", "Normal"),
    ("T_CorridorTrim_ORM", "Masks"),
    ("T_CorridorGlow_E", "Diffuse"),
]

MESHES = [
    "SM_Floor_400",
    "SM_Ceiling_400",
    "SM_Wall_Solid_400x300",
    "SM_Wall_Door_400x300",
    "SM_Wall_Door_FB_400x300",
    "SM_DoorFrame_Ring",
    "SM_DoorFrame_Ring_FB",
]


def ensure_dir(path: str):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_texture(name: str, kind: str):
    dest = f"{TEX_CONTENT}/{name}"
    existing = unreal.EditorAssetLibrary.load_asset(dest)
    png = SRC_DIR + name + ".png"
    if not unreal.Paths.file_exists(png):
        unreal.log_error(f"Missing texture file: {png}")
        return existing

    task = unreal.AssetImportTask()
    task.filename = png
    task.destination_path = TEX_CONTENT
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True

    # Texture factory options via Interchange/automated import
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex = unreal.EditorAssetLibrary.load_asset(dest)
    if not tex:
        unreal.log_error(f"Failed import {name}")
        return None

    # Compression / sampling
    try:
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
        unreal.EditorAssetLibrary.save_asset(tex.get_path_name())
    except Exception as exc:
        unreal.log_warning(f"Texture flags warning for {name}: {exc}")

    unreal.log(f"Imported texture {tex.get_path_name()}")
    return tex


def _new_tex_sample(material, tex, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -400, y
    )
    node.texture = tex
    return node


def _new_const(material, value, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -200, y
    )
    node.r = value
    return node


def create_pbr_material(asset_name: str, albedo, normal, orm, emissive=None, emissive_strength=0.0):
    ensure_dir(MAT_CONTENT)
    path = f"{MAT_CONTENT}/{asset_name}"
    mat = unreal.EditorAssetLibrary.load_asset(path)
    if mat:
        # Rebuild cleanly
        unreal.EditorAssetLibrary.delete_asset(path)

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, MAT_CONTENT, unreal.Material, factory
    )
    if not mat:
        unreal.log_error(f"Could not create material {asset_name}")
        return None

    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)

    y = -200
    albedo_n = _new_tex_sample(mat, albedo, y)
    unreal.MaterialEditingLibrary.connect_material_property(
        albedo_n, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )

    if normal:
        normal_n = _new_tex_sample(mat, normal, y + 180)
        # Normal maps often need sampler type Normal
        try:
            normal_n.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        except Exception:
            pass
        unreal.MaterialEditingLibrary.connect_material_property(
            normal_n, "RGB", unreal.MaterialProperty.MP_NORMAL
        )

    if orm:
        orm_n = _new_tex_sample(mat, orm, y + 360)
        try:
            orm_n.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        except Exception:
            pass
        # ORM packed: R=AO, G=Roughness, B=Metallic
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_n, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_n, "G", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            orm_n, "B", unreal.MaterialProperty.MP_METALLIC
        )
    else:
        rough = _new_const(mat, 0.45, y + 360)
        metal = _new_const(mat, 0.6, y + 420)
        unreal.MaterialEditingLibrary.connect_material_property(
            rough, "", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            metal, "", unreal.MaterialProperty.MP_METALLIC
        )

    if emissive and emissive_strength > 0.0:
        em_n = _new_tex_sample(mat, emissive, y + 540)
        mul = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionMultiply, -50, y + 540
        )
        strength = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant, -200, y + 600
        )
        strength.r = emissive_strength
        unreal.MaterialEditingLibrary.connect_material_expressions(em_n, "RGB", mul, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(strength, "", mul, "B")
        unreal.MaterialEditingLibrary.connect_material_property(
            mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )

    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(mat.get_path_name())
    unreal.log(f"Created material {mat.get_path_name()}")
    return mat


def assign_materials_to_meshes(mats_by_role):
    """
    mats_by_role keys: hull, floor, trim, glow
    Mesh slot mapping heuristics by mesh name + slot index.
    """
    for mesh_name in MESHES:
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{mesh_name}")
        if not mesh:
            unreal.log_warning(f"Missing mesh {mesh_name}")
            continue

        # Determine primary materials for this mesh
        if "Floor" in mesh_name:
            slot_mats = [mats_by_role["floor"], mats_by_role["trim"]]
        elif "Ceiling" in mesh_name:
            slot_mats = [mats_by_role["hull"], mats_by_role["glow"]]
        elif "DoorFrame" in mesh_name or "Ring" in mesh_name:
            slot_mats = [mats_by_role["trim"], mats_by_role["glow"]]
        elif "Door" in mesh_name:
            slot_mats = [mats_by_role["hull"], mats_by_role["trim"], mats_by_role["glow"]]
        else:
            # solid walls
            slot_mats = [mats_by_role["hull"], mats_by_role["trim"]]

        num_slots = mesh.get_num_sections(0) if hasattr(mesh, "get_num_sections") else len(slot_mats)
        # Prefer EditorStaticMeshLibrary / set_material
        try:
            materials = mesh.static_materials
            # Ensure enough slots
            while len(materials) < len(slot_mats):
                materials.append(unreal.StaticMaterial())
            for i, mat in enumerate(slot_mats):
                if i >= len(materials):
                    break
                sm = materials[i]
                sm.set_editor_property("material_interface", mat)
                sm.set_editor_property(
                    "material_slot_name",
                    unreal.Name(["Hull", "Trim", "Glow", "Floor"][min(i, 3)]),
                )
                materials[i] = sm
            mesh.set_editor_property("static_materials", materials)
        except Exception as exc:
            unreal.log_warning(f"static_materials assign failed for {mesh_name}: {exc}")
            # Fallback API
            for i, mat in enumerate(slot_mats):
                try:
                    unreal.EditorStaticMeshLibrary.set_material_slot_name(mesh, i, f"Slot_{i}")
                except Exception:
                    pass
                try:
                    mesh.set_material(i, mat)
                except Exception as exc2:
                    unreal.log_warning(f"set_material failed {mesh_name}[{i}]: {exc2}")

        unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())
        unreal.log(f"Assigned materials on {mesh_name}")


def main():
    unreal.log("=== apply_corridor_textures start ===")
    ensure_dir(TEX_CONTENT)
    ensure_dir(MAT_CONTENT)

    tex = {}
    for name, kind in TEXTURE_FILES:
        t = import_texture(name, kind)
        if t:
            tex[name] = t

    required = [
        "T_CorridorHull_D",
        "T_CorridorHull_N",
        "T_CorridorHull_ORM",
        "T_CorridorFloor_D",
        "T_CorridorFloor_N",
        "T_CorridorFloor_ORM",
        "T_CorridorTrim_D",
        "T_CorridorTrim_N",
        "T_CorridorTrim_ORM",
        "T_CorridorGlow_E",
    ]
    missing = [n for n in required if n not in tex]
    if missing:
        unreal.log_error(f"Missing textures: {missing}")
        return

    mats = {
        "hull": create_pbr_material(
            "M_CorridorHull_PBR",
            tex["T_CorridorHull_D"],
            tex["T_CorridorHull_N"],
            tex["T_CorridorHull_ORM"],
        ),
        "floor": create_pbr_material(
            "M_CorridorFloor_PBR",
            tex["T_CorridorFloor_D"],
            tex["T_CorridorFloor_N"],
            tex["T_CorridorFloor_ORM"],
        ),
        "trim": create_pbr_material(
            "M_CorridorTrim_PBR",
            tex["T_CorridorTrim_D"],
            tex["T_CorridorTrim_N"],
            tex["T_CorridorTrim_ORM"],
        ),
        "glow": create_pbr_material(
            "M_CorridorGlow_PBR",
            tex["T_CorridorHull_D"],
            tex["T_CorridorHull_N"],
            tex["T_CorridorHull_ORM"],
            emissive=tex["T_CorridorGlow_E"],
            emissive_strength=8.0,
        ),
    }
    if any(v is None for v in mats.values()):
        unreal.log_error("Material creation failed")
        return

    assign_materials_to_meshes(mats)
    unreal.log("=== apply_corridor_textures done ===")


if __name__ == "__main__":
    main()
