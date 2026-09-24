# BP_RepairTorch — экипируемый предмет на AEquippableItem + SM_RepairTorch.
# UnrealEditor-Cmd -ExecutePythonScript=H:/Projects/SpaceshipCrew/Content/Python/author_repair_torch.py

from __future__ import annotations

import unreal

BP_DIR = "/Game/Blueprints/Items"
BP_NAME = "BP_RepairTorch"
BP_PATH = f"{BP_DIR}/{BP_NAME}"
MESH_PATH = "/Game/Meshes/EngineerTraining/SM_RepairTorch"


def main() -> None:
    unreal.log("[RepairTorch] start")
    parent = unreal.load_class(None, "/Script/SpaceshipCrew.EquippableItem")
    if not parent:
        unreal.log_error("[RepairTorch] AEquippableItem missing. Compile C++ first.")
        return

    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if not mesh:
        unreal.log_error(f"[RepairTorch] mesh missing: {MESH_PATH}")
        return

    if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
        unreal.EditorAssetLibrary.make_directory(BP_DIR)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent)
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        blueprint = tools.create_asset(BP_NAME, BP_DIR, unreal.Blueprint, factory)
    else:
        blueprint = unreal.EditorAssetLibrary.load_asset(BP_PATH)

    if not blueprint:
        unreal.log_error("[RepairTorch] failed to create/load blueprint")
        return

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated = unreal.load_object(None, f"{BP_PATH}.{BP_NAME}_C")
    if not generated:
        unreal.log_error("[RepairTorch] generated class missing")
        return

    cdo = unreal.get_default_object(generated)
    display = cdo.get_editor_property("display_mesh")
    if display:
        display.set_static_mesh(mesh)
        display.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, 0.0))
        display.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
        display.set_editor_property("relative_scale3d", unreal.Vector(1.0, 1.0, 1.0))

    cdo.set_editor_property("display_name", "Горелка")
    cdo.set_editor_property("prompt_text", "Взять")
    try:
        cdo.set_editor_property("supports_secondary_action", False)
    except Exception:
        try:
            cdo.set_editor_property("b_supports_secondary_action", False)
        except Exception as exc:
            unreal.log_warning(f"[RepairTorch] secondary flag skipped: {exc}")
    cdo.set_editor_property("hand_socket_name", "hand_r")
    # Небольшой оффсет в ладони Manny (см. skill usable-equipment).
    cdo.set_editor_property("held_relative_location", unreal.Vector(8.0, 4.0, 0.0))
    cdo.set_editor_property("held_relative_rotation", unreal.Rotator(0.0, 90.0, 0.0))

    volume = cdo.get_editor_property("interaction_volume")
    if volume:
        volume.set_box_extent(unreal.Vector(40.0, 40.0, 50.0))
        volume.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, 30.0))

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    unreal.log(f"[RepairTorch] done → {BP_PATH} saved={saved}")


if __name__ == "__main__":
    main()
