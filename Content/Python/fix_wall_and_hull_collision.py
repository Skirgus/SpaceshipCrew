# Ensure blocking walls have simple box collision.
# Bridge walls are SM_Bridge_Shell (complex) — authored in author_bridge_module.py.
# Cmd: UnrealEditor-Cmd -ExecutePythonScript=.../fix_wall_and_hull_collision.py

import unreal

WALL_PATHS = [
	"/Game/Meshes/CorridorPanels/SM_Wall_Solid_400x300",
	"/Game/Meshes/CorridorPanels/SM_Wall_Solid_FB_400x300",
	"/Game/Meshes/CorridorPanels/SM_Wall_Door_400x300",
	"/Game/Meshes/CorridorPanels/SM_Wall_Door_FB_400x300",
	"/Game/Meshes/TrainingShip/SM_Airlock_Wall_Left",
	"/Game/Meshes/TrainingShip/SM_Airlock_Wall_Right",
	"/Game/Meshes/TrainingShip/SM_Airlock_Wall_Back",
	"/Game/Meshes/TrainingShip/SM_Airlock_Wall_Front_L",
	"/Game/Meshes/TrainingShip/SM_Airlock_Wall_Front_R",
	"/Game/Meshes/TrainingShip/SM_Cargo_Wall_Left",
	"/Game/Meshes/TrainingShip/SM_Cargo_Wall_Right",
	"/Game/Meshes/TrainingShip/SM_Cargo_Wall_Back",
	"/Game/Meshes/TrainingShip/SM_Cargo_Wall_Front_L",
	"/Game/Meshes/TrainingShip/SM_Cargo_Wall_Front_R",
	"/Game/Meshes/CorridorPanels/SM_Ceiling_400",
	"/Game/Meshes/TrainingShip/SM_Airlock_Ceiling",
	"/Game/Meshes/TrainingShip/SM_Cargo_Ceiling",
]

# Visual-only rings / props that must not block passages.
NO_COLLISION_PATHS = [
	"/Game/Meshes/TrainingShip/SM_Airlock_DoorRing",
]


def ensure_box(mesh):
	name = mesh.get_name()
	try:
		unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
	except Exception as exc:
		unreal.log_warning("%s remove: %s" % (name, exc))
	try:
		unreal.EditorStaticMeshLibrary.add_simple_collisions(
			mesh, unreal.ScriptingCollisionShapeType.BOX
		)
	except Exception as exc:
		unreal.log_warning("%s add box: %s" % (name, exc))
	bs = mesh.get_editor_property("body_setup")
	if bs:
		bs.set_editor_property(
			"collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX
		)
	unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
	e = mesh.get_bounds().box_extent * 2.0
	unreal.log("WALL BOX %s size=(%.0f,%.0f,%.0f)" % (name, e.x, e.y, e.z))


def clear_collision(mesh):
	name = mesh.get_name()
	try:
		unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
	except Exception as exc:
		unreal.log_warning("%s clear remove: %s" % (name, exc))
	bs = mesh.get_editor_property("body_setup")
	if bs:
		bs.set_editor_property(
			"collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX
		)
	unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
	unreal.log("NO COLLISION %s" % name)


def main():
	unreal.log("=== fix_wall_and_hull_collision start ===")
	for path in WALL_PATHS:
		mesh = unreal.EditorAssetLibrary.load_asset(path)
		if mesh:
			ensure_box(mesh)
		else:
			unreal.log_error("missing %s" % path)
	for path in NO_COLLISION_PATHS:
		mesh = unreal.EditorAssetLibrary.load_asset(path)
		if mesh:
			clear_collision(mesh)
		else:
			unreal.log_warning("missing deco %s" % path)
	unreal.log("=== fix_wall_and_hull_collision done ===")


main()
