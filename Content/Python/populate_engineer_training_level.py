# Populate already-created EngineerTraining map (safe for live Editor).
# Output Log:
#   py "H:/Projects/SpaceshipCrew/Content/Python/populate_engineer_training_level.py"

from __future__ import annotations

import unreal

MAP_PATH = "/Game/Maps/Training/EngineerTraining"
MESH_DIR = "/Game/Meshes/EngineerTraining"


def mesh(name: str):
    return unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{name}")


def destroy_et_actors() -> None:
    for actor in list(unreal.EditorLevelLibrary.get_all_level_actors()):
        label = actor.get_actor_label()
        if label.startswith("ET_"):
            unreal.EditorLevelLibrary.destroy_actor(actor)


def spawn_sma(mesh_asset, location, rotation=(0.0, 0.0, 0.0), label: str = ""):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*location),
        unreal.Rotator(rotation[0], rotation[1], rotation[2]),
    )
    actor.static_mesh_component.set_static_mesh(mesh_asset)
    if label:
        actor.set_actor_label(label)
    return actor


def spawn_native(class_path: str, location, rotation=(0.0, 0.0, 0.0), label: str = ""):
    cls = unreal.load_class(None, class_path)
    if not cls:
        unreal.log_warning(f"[ET] Missing class (compile C++ module?): {class_path}")
        return None
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        cls,
        unreal.Vector(*location),
        unreal.Rotator(rotation[0], rotation[1], rotation[2]),
    )
    if actor and label:
        actor.set_actor_label(label)
    return actor


def assign_mesh(actor, mesh_asset) -> None:
    if not actor or not mesh_asset:
        return
    try:
        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        if comps:
            comps[0].set_static_mesh(mesh_asset)
            return
    except Exception:
        pass
    for name in ("mesh_component", "MeshComponent"):
        try:
            comp = actor.get_editor_property(name)
            if comp:
                comp.set_static_mesh(mesh_asset)
                return
        except Exception:
            continue


def main() -> None:
    unreal.log("[ET] populate start")
    world = unreal.EditorLevelLibrary.get_editor_world()
    current = world.get_path_name() if world else ""
    if MAP_PATH not in current:
        if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
            unreal.log_error(f"[ET] Failed to load {MAP_PATH}")
            return
    else:
        unreal.log(f"[ET] already on {current}")

    destroy_et_actors()

    bay = mesh("SM_TechBay_Interior_1x1x1")

    if bay:
        spawn_sma(bay, (0, 0, 0), label="ET_TechBay")

    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-50, 40, 0)
    )
    sun.set_actor_label("ET_Sun")
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 300), unreal.Rotator()
    )
    sky.set_actor_label("ET_SkyLight")

    ps = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0, 0, -36), unreal.Rotator(0, 180, 0)
    )
    ps.set_actor_label("ET_PlayerStart")

    # Оборудование задаётся в модуле (EquipmentPlacements), не акторами этой карты.

    try:
        world = unreal.EditorLevelLibrary.get_editor_world()
        gm = unreal.load_class(None, "/Script/SpaceshipCrew.SpaceshipCrewTrainingGameMode")
        if world and gm and world.world_settings:
            world.world_settings.set_editor_property("default_game_mode", gm)
            unreal.log("[ET] GameMode set")
    except Exception as exc:
        unreal.log_warning(f"[ET] GameMode: {exc}")

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("[ET] populate done — PIE or Menu → Тренировки → Инженер")


if __name__ == "__main__":
    main()
