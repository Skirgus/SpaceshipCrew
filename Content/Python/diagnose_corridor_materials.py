import unreal

def inspect_mat(path):
    mat = unreal.EditorAssetLibrary.load_asset(path)
    if not mat:
        unreal.log_error(f"Missing {path}")
        return
    unreal.log(f"=== {path} ===")
    unreal.log(f"class={mat.get_class().get_name()}")
    # List expressions if Material
    try:
        exprs = unreal.MaterialEditingLibrary.get_material_expressions(mat)
        unreal.log(f"expressions={len(exprs)}")
        for e in exprs:
            unreal.log(f"  expr {e.get_class().get_name()} path={e.get_path_name()}")
            if hasattr(e, "texture"):
                tex = e.texture
                unreal.log(f"    texture={tex.get_path_name() if tex else None}")
    except Exception as exc:
        unreal.log_warning(f"expr inspect failed: {exc}")

    # Check material property connections via editor lib where possible
    for prop_name in ["BaseColor", "Metallic", "Roughness", "Normal", "EmissiveColor", "AmbientOcclusion"]:
        try:
            # No direct getter; log shading model / blend
            pass
        except Exception:
            pass
    try:
        unreal.log(f"blend={mat.get_editor_property('blend_mode')}")
        unreal.log(f"shading={mat.get_editor_property('shading_model')}")
        unreal.log(f"two_sided={mat.get_editor_property('two_sided')}")
    except Exception as exc:
        unreal.log_warning(str(exc))


def inspect_tex(path):
    tex = unreal.EditorAssetLibrary.load_asset(path)
    if not tex:
        unreal.log_error(f"Missing tex {path}")
        return
    size_x = tex.blueprint_get_size_x() if hasattr(tex, "blueprint_get_size_x") else "?"
    size_y = tex.blueprint_get_size_y() if hasattr(tex, "blueprint_get_size_y") else "?"
    unreal.log(
        f"TEX {path} size=({size_x},{size_y}) srgb={tex.get_editor_property('srgb')} "
        f"compression={tex.get_editor_property('compression_settings')}"
    )


for p in [
    "/Game/Meshes/CorridorPanels/Materials/M_CorridorFloor_PBR",
    "/Game/Meshes/CorridorPanels/Materials/M_CorridorHull_PBR",
    "/Game/Meshes/CorridorPanels/Materials/M_CorridorGlow_PBR",
]:
    inspect_mat(p)

for p in [
    "/Game/Meshes/CorridorPanels/Textures/T_CorridorFloor_D",
    "/Game/Meshes/CorridorPanels/Textures/T_CorridorHull_D",
    "/Game/Meshes/CorridorPanels/Textures/T_CorridorGlow_E",
]:
    inspect_tex(p)

mesh = unreal.EditorAssetLibrary.load_asset("/Game/Meshes/CorridorPanels/SM_Floor_400")
mats = mesh.get_editor_property("static_materials")
unreal.log(f"SM_Floor_400 materials={len(mats)}")
for i, sm in enumerate(mats):
    mi = sm.get_editor_property("material_interface")
    unreal.log(f"  slot[{i}]={mi.get_path_name() if mi else None}")
