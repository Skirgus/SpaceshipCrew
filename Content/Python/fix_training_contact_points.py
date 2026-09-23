# Rebuild ContactPoints for training modules after grid change (600×600×400).
import unreal
import sys

sys.path.insert(0, unreal.Paths.project_content_dir() + "Python")
from ship_builder_grid import PANEL_XY, PANEL_Z

MODULES = [
	("/Game/Data/ShipModules/Airlock_Training_01", (1, 1, 1)),
	("/Game/Data/ShipModules/Corridor_CustomPanels_01", (1, 1, 1)),
	("/Game/Data/ShipModules/CargoHold_Training_01", (2, 1, 1)),
	("/Game/Data/ShipModules/Bridge_Training_01", (2, 1, 1)),
]


def make_panel_contacts(cell):
	cx, cy, cz = int(cell[0]), int(cell[1]), int(cell[2])
	half = unreal.Vector(cx * PANEL_XY * 0.5, cy * PANEL_XY * 0.5, cz * PANEL_Z * 0.5)
	points = []

	def add(face, ix, iy, iz, horizontal=True):
		gx = -half.x + (ix + 0.5) * PANEL_XY
		gy = -half.y + (iy + 0.5) * PANEL_XY
		gz = -half.z + (iz + 0.5) * PANEL_Z
		if face == "Back":
			loc = unreal.Vector(-half.x, gy, gz)
		elif face == "Front":
			loc = unreal.Vector(half.x, gy, gz)
		elif face == "Left":
			loc = unreal.Vector(gx, -half.y, gz)
		elif face == "Right":
			loc = unreal.Vector(gx, half.y, gz)
		elif face == "Bottom":
			loc = unreal.Vector(gx, gy, -half.z)
		else:
			loc = unreal.Vector(gx, gy, half.z)
		cp = unreal.ShipModuleContactPoint()
		cp.set_editor_property("socket_name", unreal.Name("%s_X%d_Y%d_Z%d" % (face, ix, iy, iz)))
		cp.set_editor_property("relative_location", loc)
		try:
			st = unreal.ShipModuleSocketType.HORIZONTAL if horizontal else unreal.ShipModuleSocketType.VERTICAL
			cp.set_editor_property("socket_type", st)
		except Exception:
			pass
		points.append(cp)

	for iy in range(cy):
		for iz in range(cz):
			add("Back", 0, iy, iz, True)
			add("Front", cx - 1, iy, iz, True)
	for ix in range(cx):
		for iz in range(cz):
			add("Left", ix, 0, iz, True)
			add("Right", ix, cy - 1, iz, True)
	for ix in range(cx):
		for iy in range(cy):
			add("Bottom", ix, iy, 0, False)
			add("Top", ix, iy, cz - 1, False)
	return points


def main():
	unreal.log("=== fix_training_contact_points start ===")
	for path, cell in MODULES:
		definition = unreal.EditorAssetLibrary.load_asset(path)
		if not definition:
			unreal.log_warning("missing %s" % path)
			continue
		definition.set_editor_property("cell_size", unreal.IntVector(cell[0], cell[1], cell[2]))
		# Size is derived/read-only from CellSize in editor
		try:
			definition.set_editor_property(
				"size", unreal.Vector(cell[0] * PANEL_XY, cell[1] * PANEL_XY, cell[2] * PANEL_Z)
			)
		except Exception:
			pass
		pts = make_panel_contacts(cell)
		definition.set_editor_property("contact_points", pts)
		unreal.EditorAssetLibrary.save_loaded_asset(definition)
		unreal.log("%s contacts=%d size=(%.0f,%.0f,%.0f)" % (
			path, len(pts), cell[0] * PANEL_XY, cell[1] * PANEL_XY, cell[2] * PANEL_Z
		))
	unreal.log("=== fix_training_contact_points done ===")


main()
