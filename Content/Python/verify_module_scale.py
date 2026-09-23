# Verify mesh extents and VisualPart placements after 600×600×400 rescale.
import unreal
import sys

sys.path.insert(0, unreal.Paths.project_content_dir() + "Python")
from ship_builder_grid import PANEL_XY, PANEL_Z, FLOOR_REL_Z, WALL_OFFSET, FACE_OFFSET

MESHES = [
	("/Game/Meshes/CorridorPanels/SM_Floor_400", PANEL_XY, PANEL_XY, 16.0),
	("/Game/Meshes/CorridorPanels/SM_Wall_Solid_400x300", PANEL_XY, 20.0, PANEL_Z),
	("/Game/Meshes/CorridorPanels/SM_Wall_Door_FB_400x300", 20.0, PANEL_XY, PANEL_Z),
	("/Game/Meshes/TrainingShip/SM_Airlock_Floor", PANEL_XY, PANEL_XY, 16.0),
	("/Game/Meshes/TrainingShip/SM_Cargo_Floor", PANEL_XY * 2, PANEL_XY, 16.0),
]

VOS = [
	"/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride",
	"/Game/Data/ShipModules/Airlock_Training_01_VisualOverride",
	"/Game/Data/ShipModules/CargoHold_Training_01_VisualOverride",
	"/Game/Data/ShipModules/Bridge_Training_01_VisualOverride",
]


def main():
	unreal.log("=== verify_module_scale start ===")
	ok = True
	for path, ex, ey, ez in MESHES:
		mesh = unreal.EditorAssetLibrary.load_asset(path)
		if not mesh:
			unreal.log_error("missing %s" % path)
			ok = False
			continue
		e = mesh.get_bounds().box_extent * 2.0
		unreal.log("%s size=(%.0f,%.0f,%.0f) expect≈(%.0f,%.0f,%.0f)" % (
			path, e.x, e.y, e.z, ex, ey, ez
		))
		if abs(e.x - ex) > ex * 0.2 or abs(e.y - ey) > max(ey, 1.0) * 0.35:
			unreal.log_warning("SIZE MISMATCH %s" % path)
			ok = False

	for path in VOS:
		vo = unreal.EditorAssetLibrary.load_asset(path)
		parts = vo.get_editor_property("visual_parts") if vo else None
		unreal.log("%s parts=%d" % (path, len(parts) if parts else 0))
		if not parts:
			ok = False
			continue
		for i, p in enumerate(parts):
			tr = p.get_editor_property("relative_transform")
			sock = p.get_editor_property("wall_socket_name")
			t = tr.translation
			mesh = p.get_editor_property("mesh")
			unreal.log(
				"  [%d] %s loc=(%.0f,%.0f,%.0f) sock=%s"
				% (i, mesh.get_name() if mesh else None, t.x, t.y, t.z, sock)
			)
			if sock and sock != "None" and abs(t.x) < 1.0 and abs(t.y) < 1.0 and abs(t.z) < 1.0:
				unreal.log_error("dock part at origin: %s[%d]" % (path, i))
				ok = False

	unreal.log("=== verify_module_scale %s ===" % ("OK" if ok else "FAIL"))


main()
