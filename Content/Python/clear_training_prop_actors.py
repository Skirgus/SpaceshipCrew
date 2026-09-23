"""Remove ET_ engineer props and cargo crate SMAs from EngineerTraining map."""
from __future__ import annotations

import unreal

MAP_PATH = "/Game/Maps/Training/EngineerTraining"
DROP_LABEL_PREFIXES = ("ET_Energy", "ET_Damaged", "ET_Repair", "ET_TechBay")
DROP_MESH_TOKENS = (
	"SM_EnergyConsole",
	"SM_HullPanel_Damaged",
	"SM_HullPanel_Intact",
	"SM_RepairTorch",
	"SM_TechBay_Interior",
	"SM_Cargo_Crate",
	"SM_Cargo_Pad",
)


def mesh_name(actor) -> str:
	try:
		comps = actor.get_components_by_class(unreal.StaticMeshComponent)
		if comps and comps[0].static_mesh:
			return comps[0].static_mesh.get_name()
	except Exception:
		pass
	return ""


def should_drop(actor) -> bool:
	label = actor.get_actor_label() or ""
	if any(label.startswith(p) for p in DROP_LABEL_PREFIXES):
		return True
	cls = actor.get_class().get_name() if actor.get_class() else ""
	if cls in ("EngineerEnergyConsole", "DamagedHullPanel", "EngineerTrainingScenario", "CrewWorkstation"):
		return True
	name = mesh_name(actor)
	return any(tok in name for tok in DROP_MESH_TOKENS)


def main() -> None:
	world = unreal.EditorLevelLibrary.get_editor_world()
	current = world.get_path_name() if world else ""
	if MAP_PATH not in current:
		if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
			unreal.log_error("failed to load %s" % MAP_PATH)
			return
	removed = 0
	for actor in list(unreal.EditorLevelLibrary.get_all_level_actors()):
		if should_drop(actor):
			unreal.log("destroy %s (%s)" % (actor.get_actor_label(), mesh_name(actor) or actor.get_class().get_name()))
			unreal.EditorLevelLibrary.destroy_actor(actor)
			removed += 1
	unreal.EditorLevelLibrary.save_current_level()
	unreal.log("removed %d prop actors from EngineerTraining" % removed)


main()
