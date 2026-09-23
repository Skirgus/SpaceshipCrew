# BP_CrewSeat (Occupy, no UI) and one placement on Bridge_Training_01.
# Output Log: py "H:/Projects/SpaceshipCrew/Content/Python/author_crew_seat.py"

from __future__ import annotations

import unreal

BP_DIR = "/Game/Blueprints/Equipment"
BP_NAME = "BP_CrewSeat"
BP_PATH = f"{BP_DIR}/{BP_NAME}"
OVERRIDE_PATH = "/Game/Data/ShipModules/Bridge_Training_01_VisualOverride"


def main() -> None:
    unreal.log("[CrewSeat] start")
    parent = unreal.load_class(None, "/Script/SpaceshipCrew.UsableEquipment")
    if not parent:
        unreal.log_error("[CrewSeat] AUsableEquipment is missing. Compile the C++ module first.")
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
        unreal.log_error("[CrewSeat] failed to create or load blueprint")
        return

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated = unreal.load_object(None, f"{BP_PATH}.{BP_NAME}_C")
    if not generated:
        unreal.log_error("[CrewSeat] generated class was not created")
        return
    cdo = unreal.get_default_object(generated)

    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    display = cdo.get_editor_property("display_mesh")
    if display and cube:
        display.set_static_mesh(cube)
        display.set_relative_scale3d(unreal.Vector(0.5, 0.5, 0.45))

    occupy = unreal.UsableEquipmentUseMode.OCCUPY
    cdo.set_editor_property("use_mode", occupy)
    cdo.set_editor_property("display_name", "Кресло")
    cdo.set_editor_property("prompt_text", "Сесть")

    anchor = cdo.get_editor_property("use_anchor")
    if anchor:
        anchor.set_editor_property("relative_location", unreal.Vector(80.0, 0.0, 70.0))

    volume = cdo.get_editor_property("interaction_volume")
    if volume:
        volume.set_editor_property("relative_location", unreal.Vector(90.0, 0.0, 40.0))
        volume.set_editor_property("box_extent", unreal.Vector(70.0, 70.0, 90.0))

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)

    override = unreal.EditorAssetLibrary.load_asset(OVERRIDE_PATH)
    if not override:
        unreal.log_error(f"[CrewSeat] missing {OVERRIDE_PATH}")
        return

    placement = unreal.ShipModuleEquipmentPlacement()
    placement.set_editor_property("equipment_class", generated)
    transform = unreal.Transform()
    transform.translation = unreal.Vector(200.0, -120.0, -159.0)
    transform.rotation = unreal.Rotator(0.0, 180.0, 0.0).quaternion()
    transform.scale3d = unreal.Vector(1.0, 1.0, 1.0)
    placement.set_editor_property("relative_transform", transform)

    kept = []
    for item in override.get_editor_property("equipment_placements"):
        item_class = item.get_editor_property("equipment_class")
        class_name = ""
        if item_class:
            try:
                class_name = item_class.get_name()
            except Exception:
                class_name = str(item_class)
        if "BP_CrewSeat" not in class_name:
            kept.append(item)
    kept.append(placement)
    override.set_editor_property("equipment_placements", kept)
    unreal.EditorAssetLibrary.save_loaded_asset(override)
    unreal.log("[CrewSeat] BP_CrewSeat placed on Bridge_Training_01")


main()
