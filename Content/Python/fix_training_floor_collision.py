# Ensure walkable floors have simple box collision (ISM-safe) after rescale/reimport.
# Also pads XY box slightly so neighboring module floors overlap at seams.
# Output Log: py "H:/Projects/SpaceshipCrew/Content/Python/fix_training_floor_collision.py"

import unreal
import sys

sys.path.insert(0, unreal.Paths.project_content_dir() + "Python")
from ship_builder_grid import PANEL_XY, FLOOR_THICK

FLOOR_PATHS = [
	"/Game/Meshes/CorridorPanels/SM_Floor_400",
	"/Game/Meshes/TrainingShip/SM_Airlock_Floor",
	"/Game/Meshes/TrainingShip/SM_Cargo_Floor",
	"/Game/Meshes/TrainingShip/SM_Bridge_Floor",
	"/Game/Meshes/TrainingShip/SM_Cargo_Pad",
]

# Extra half-extent on each XY side (cm) so floors overlap at module seams.
SEAM_OVERLAP = 4.0


def ensure_box_collision(mesh, pad_xy=0.0):
	name = mesh.get_name()
	try:
		unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
	except Exception as exc:
		unreal.log_warning("%s remove_collisions: %s" % (name, exc))

	try:
		added = unreal.EditorStaticMeshLibrary.add_simple_collisions(
			mesh, unreal.ScriptingCollisionShapeType.BOX
		)
		unreal.log("%s add_simple_collisions BOX -> %s" % (name, added))
	except Exception as exc:
		unreal.log_warning("%s add_simple_collisions: %s" % (name, exc))

	bs = mesh.get_editor_property("body_setup")
	if bs:
		bs.set_editor_property(
			"collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX
		)
		# Enlarge simple box XY for seam overlap (keep Z = mesh thickness).
		if pad_xy > 0.0:
			try:
				aggs = bs.get_editor_property("agg_geom")
				boxes = list(aggs.get_editor_property("box_elems") or []) if aggs else []
				if boxes:
					b = boxes[0]
					# FKBoxElem: X/Y/Z are full extents in UE Python
					try:
						x = float(b.get_editor_property("x"))
						y = float(b.get_editor_property("y"))
						b.set_editor_property("x", x + pad_xy * 2.0)
						b.set_editor_property("y", y + pad_xy * 2.0)
						aggs.set_editor_property("box_elems", [b])
						bs.set_editor_property("agg_geom", aggs)
						unreal.log("%s padded box XY +%.1f cm each side" % (name, pad_xy))
					except Exception as exc:
						unreal.log_warning("%s pad box: %s" % (name, exc))
			except Exception as exc:
				unreal.log_warning("%s enlarge: %s" % (name, exc))

	unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
	bs = mesh.get_editor_property("body_setup")
	aggs = bs.get_editor_property("agg_geom") if bs else None
	boxes = aggs.get_editor_property("box_elems") if aggs else None
	unreal.log(
		"%s flag=%s boxes=%d size_expect_xy≈%.0f thick≈%.0f"
		% (
			name,
			bs.get_editor_property("collision_trace_flag") if bs else None,
			len(boxes) if boxes else 0,
			PANEL_XY,
			FLOOR_THICK,
		)
	)


def main():
	unreal.log("=== fix_training_floor_collision start ===")
	for path in FLOOR_PATHS:
		mesh = unreal.EditorAssetLibrary.load_asset(path)
		if mesh:
			pad = SEAM_OVERLAP if "Floor" in mesh.get_name() else 0.0
			ensure_box_collision(mesh, pad_xy=pad)
		else:
			unreal.log_error("missing %s" % path)
	unreal.log("=== fix_training_floor_collision done ===")


main()
