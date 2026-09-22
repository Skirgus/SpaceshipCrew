# Import TrainingShip FBX and author Airlock / CargoHold / Bridge modules.
# Meshes are exported local-centered; RelativeTransform places them (corridor-style).
# Grid: PanelUnitXY=600, PanelUnitZ=400.
# Output Log: py "H:/Projects/SpaceshipCrew/Content/Python/author_training_ship_modules.py"

import unreal
import os
import math
import json
import sys

sys.path.insert(0, unreal.Paths.project_content_dir() + "Python")
from ship_builder_grid import (
	PANEL_XY,
	PANEL_Z,
	FLOOR_REL_Z,
	FACE_OFFSET,
	face_offset_for_cells,
)

MESH_DIR = "/Game/Meshes/TrainingShip"
FBX_DIR = unreal.Paths.project_content_dir() + "Meshes/TrainingShip/"
MODULE_DIR = "/Game/Data/ShipModules"
CORRIDOR_DOOR_FB = "/Game/Meshes/CorridorPanels/SM_Wall_Door_FB_400x300"
CORRIDOR_FLOOR = "/Game/Meshes/CorridorPanels/SM_Floor_400"
PLACEMENTS_PATH = unreal.Paths.project_content_dir() + "Meshes/TrainingShip/part_placements.json"

SCALE = 1.0
PLACEMENTS = {}


def load_placements():
	global PLACEMENTS
	if os.path.isfile(PLACEMENTS_PATH):
		with open(PLACEMENTS_PATH, "r", encoding="utf-8") as f:
			raw = json.load(f)
		PLACEMENTS = {k: tuple(v) for k, v in raw.items()}
	unreal.log("placements loaded: %d" % len(PLACEMENTS))


def loc_of(mesh_name, fallback=(0.0, 0.0, 0.0)):
	return PLACEMENTS.get(mesh_name, fallback)


def import_mesh(mesh_name, uniform_scale):
	path = "%s/%s" % (MESH_DIR, mesh_name)
	fbx_path = FBX_DIR + mesh_name + ".fbx"
	if not os.path.isfile(fbx_path):
		unreal.log_warning("FBX missing: %s" % fbx_path)
		return unreal.EditorAssetLibrary.load_asset(path)

	task = unreal.AssetImportTask()
	task.filename = fbx_path
	task.destination_path = MESH_DIR
	task.destination_name = mesh_name
	task.replace_existing = True
	task.automated = True
	task.save = True

	options = unreal.FbxImportUI()
	options.import_mesh = True
	options.import_as_skeletal = False
	options.import_materials = False
	options.import_textures = False
	options.automated_import_should_detect_type = False
	options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
	options.static_mesh_import_data.set_editor_property("combine_meshes", True)
	options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
	options.static_mesh_import_data.set_editor_property("import_uniform_scale", uniform_scale)
	options.static_mesh_import_data.set_editor_property("convert_scene", True)
	task.options = options

	unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
	mesh = unreal.EditorAssetLibrary.load_asset(path)
	if mesh:
		e = mesh.get_bounds().box_extent * 2.0
		o = mesh.get_bounds().origin
		unreal.log(
			"Imported %s origin=(%.1f,%.1f,%.1f) size=(%.1f,%.1f,%.1f) scale=%s"
			% (mesh_name, o.x, o.y, o.z, e.x, e.y, e.z, uniform_scale)
		)
		unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
	return mesh


def pick_scale(sample_name, expect_xy):
	for scale in (1.0, 100.0, 0.01):
		mesh = import_mesh(sample_name, scale)
		if not mesh:
			continue
		e = mesh.get_bounds().box_extent * 2.0
		if expect_xy * 0.75 < max(e.x, e.y) < expect_xy * 1.35:
			return scale
	return 1.0


def make_tr(location=(0.0, 0.0, 0.0), yaw_deg=0.0):
	tr = unreal.Transform()
	tr.translation = unreal.Vector(float(location[0]), float(location[1]), float(location[2]))
	yaw_rad = math.radians(float(yaw_deg))
	tr.rotation = unreal.Quat(0.0, 0.0, math.sin(yaw_rad * 0.5), math.cos(yaw_rad * 0.5))
	tr.scale3d = unreal.Vector(1.0, 1.0, 1.0)
	return tr


def make_part(mesh, location=(0.0, 0.0, 0.0), yaw_deg=0.0, wall_socket=None, opening_mesh=None):
	part = unreal.ShipModuleVisualPart()
	if mesh:
		part.set_editor_property("mesh", mesh)
	part.set_editor_property("relative_transform", make_tr(location, yaw_deg))
	if wall_socket:
		part.set_editor_property("wall_socket_name", unreal.Name(wall_socket))
		part.set_editor_property("opening_kind", unreal.ShipModuleWallOpeningKind.PASSAGE)
		if opening_mesh:
			part.set_editor_property("opening_mesh", opening_mesh)
		part.set_editor_property("airtight_when_closed", True)
		part.set_editor_property("affects_oxygen_volume", True)
	return part


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


def create_or_load_asset(asset_name, package_path, asset_class, factory_name):
	full = "%s/%s" % (package_path, asset_name)
	existing = unreal.EditorAssetLibrary.load_asset(full)
	if existing:
		return existing
	factory_cls = getattr(unreal, factory_name, None)
	factory = factory_cls() if factory_cls else unreal.DataAssetFactory()
	created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
		asset_name, package_path, asset_class, factory
	)
	if created:
		return created
	src = {
		"ShipModuleDefinition": "/Game/Data/ShipModules/Corridor_CustomPanels_01",
		"ShipModuleVisualOverride": "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride",
	}.get(asset_class.__name__)
	if src and unreal.EditorAssetLibrary.does_asset_exist(src):
		return unreal.EditorAssetLibrary.duplicate_asset(src, "%s/%s" % (package_path, asset_name))
	return None


def author_module(module_id, display, module_type, cell, mesh_names, parts_builder, forced_side=None):
	meshes = {n: import_mesh(n, SCALE) for n in mesh_names}
	apply_training_collision_policy(meshes)
	definition = create_or_load_asset(module_id, MODULE_DIR, unreal.ShipModuleDefinition, "ShipModuleDefinitionFactory")
	override = create_or_load_asset(
		"%s_VisualOverride" % module_id, MODULE_DIR, unreal.ShipModuleVisualOverride, "ShipModuleVisualOverrideFactory"
	)
	if not definition or not override:
		unreal.log_error("Failed Definition/VO for %s" % module_id)
		return False

	parts = parts_builder(meshes)
	override.set_editor_property("visual_parts", parts)
	unreal.EditorAssetLibrary.save_asset(override.get_path_name(), only_if_is_dirty=False)

	definition.set_editor_property("module_id", unreal.Name(module_id))
	definition.set_editor_property("display_name", unreal.Text(display))
	definition.set_editor_property("module_type", module_type)
	definition.set_editor_property("cell_size", unreal.IntVector(cell[0], cell[1], cell[2]))
	definition.set_editor_property("has_interior", True)
	definition.set_editor_property("mass", float(cell[0] * cell[1] * 150))
	definition.set_editor_property("credit_cost", int(cell[0] * cell[1] * 200))
	definition.set_editor_property("visual_override", override)
	if forced_side is not None:
		try:
			definition.set_editor_property("forced_opening_side", forced_side)
		except Exception as exc:
			unreal.log_warning("forced_opening_side: %s" % exc)
	try:
		definition.set_editor_property(
			"size", unreal.Vector(cell[0] * PANEL_XY, cell[1] * PANEL_XY, cell[2] * PANEL_Z)
		)
	except Exception:
		pass

	try:
		definition.set_editor_property("contact_points", make_panel_contacts(cell))
	except Exception as exc:
		unreal.log_warning("contact_points: %s" % exc)

	unreal.EditorAssetLibrary.save_loaded_asset(definition)
	unreal.EditorAssetLibrary.save_loaded_asset(override)
	for i, p in enumerate(parts):
		tr = p.get_editor_property("relative_transform")
		sock = p.get_editor_property("wall_socket_name")
		t = tr.translation
		unreal.log(
			"%s[%d] loc=(%.0f,%.0f,%.0f) yaw=%.0f sock=%s"
			% (module_id, i, t.x, t.y, t.z, tr.rotation.rotator().yaw, sock)
		)
	unreal.log("%s: parts=%d cell=%s contacts=%d" % (
		module_id, len(parts), cell, len(definition.get_editor_property("contact_points") or [])
	))
	return True


def ensure_box_collision(mesh):
	if not mesh:
		return
	try:
		unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
		unreal.EditorStaticMeshLibrary.add_simple_collisions(
			mesh, unreal.ScriptingCollisionShapeType.BOX
		)
		bs = mesh.get_editor_property("body_setup")
		if bs:
			bs.set_editor_property(
				"collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX
			)
		unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
	except Exception as exc:
		unreal.log_warning("ensure_box %s: %s" % (mesh.get_name(), exc))


def clear_mesh_collision(mesh):
	if not mesh:
		return
	try:
		unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
		unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
	except Exception as exc:
		unreal.log_warning("clear_collision %s: %s" % (mesh.get_name(), exc))


def apply_training_collision_policy(meshes):
	"""Walls/floors/ceilings: simple box. Airlock door ring: no collision."""
	no_col = {
		"SM_Airlock_DoorRing",
	}
	for name, mesh in meshes.items():
		if not mesh:
			continue
		if name in no_col:
			clear_mesh_collision(mesh)
		elif "Wall" in name or "Floor" in name or "Ceiling" in name or name == "SM_Cargo_Pad":
			ensure_box_collision(mesh)
		elif "Crate" in name or "Seat" in name or "Dash" in name:
			ensure_box_collision(mesh)


def main():
	global SCALE
	unreal.log("=== author_training_ship_modules start ===")
	load_placements()
	if not unreal.EditorAssetLibrary.does_directory_exist(MESH_DIR):
		unreal.EditorAssetLibrary.make_directory(MESH_DIR)
	if not unreal.EditorAssetLibrary.does_directory_exist(MODULE_DIR):
		unreal.EditorAssetLibrary.make_directory(MODULE_DIR)

	SCALE = pick_scale("SM_Airlock_Floor", PANEL_XY)
	unreal.log("import scale=%s" % SCALE)

	door_fb = unreal.EditorAssetLibrary.load_asset(CORRIDOR_DOOR_FB)
	floor_tile = unreal.EditorAssetLibrary.load_asset(CORRIDOR_FLOOR)

	def airlock_parts(m):
		return [
			make_part(m.get("SM_Airlock_Floor"), loc_of("SM_Airlock_Floor")),
			make_part(m.get("SM_Airlock_Ceiling"), loc_of("SM_Airlock_Ceiling")),
			make_part(m.get("SM_Airlock_Wall_Left"), loc_of("SM_Airlock_Wall_Left"), 0.0),
			make_part(m.get("SM_Airlock_Wall_Right"), loc_of("SM_Airlock_Wall_Right"), 180.0),
			make_part(
				m.get("SM_Airlock_Wall_Back"),
				loc_of("SM_Airlock_Wall_Back"),
				0.0,
				"Back_X0_Y0_Z0",
				door_fb,
			),
			make_part(m.get("SM_Airlock_Wall_Front_L"), loc_of("SM_Airlock_Wall_Front_L"), 180.0),
			make_part(m.get("SM_Airlock_Wall_Front_R"), loc_of("SM_Airlock_Wall_Front_R"), 180.0),
			make_part(None, (FACE_OFFSET, 0.0, 0.0), 180.0, "Front_X0_Y0_Z0", door_fb),
			make_part(m.get("SM_Airlock_DoorRing"), loc_of("SM_Airlock_DoorRing")),
		]

	author_module(
		"Airlock_Training_01",
		"Airlock (training)",
		unreal.ShipModuleType.AIRLOCK,
		(1, 1, 1),
		[
			"SM_Airlock_Floor",
			"SM_Airlock_Ceiling",
			"SM_Airlock_Wall_Left",
			"SM_Airlock_Wall_Right",
			"SM_Airlock_Wall_Back",
			"SM_Airlock_Wall_Front_L",
			"SM_Airlock_Wall_Front_R",
			"SM_Airlock_DoorRing",
		],
		airlock_parts,
		forced_side=unreal.ShipModuleOpeningSide.BACK,
	)

	def cargo_parts(m):
		cargo_floor = m.get("SM_Cargo_Floor")
		if cargo_floor:
			floor_parts = [make_part(cargo_floor, loc_of("SM_Cargo_Floor", (0.0, 0.0, FLOOR_REL_Z)))]
		else:
			half_tile = PANEL_XY * 0.5
			floor_parts = [
				make_part(floor_tile, (-half_tile, 0.0, FLOOR_REL_Z)),
				make_part(floor_tile, (half_tile, 0.0, FLOOR_REL_Z)),
			]
		# Shell only — no pad/crates (decorative cargo props).
		return floor_parts + [
			make_part(m.get("SM_Cargo_Ceiling"), loc_of("SM_Cargo_Ceiling")),
			make_part(m.get("SM_Cargo_Wall_Left"), loc_of("SM_Cargo_Wall_Left"), 0.0),
			make_part(m.get("SM_Cargo_Wall_Right"), loc_of("SM_Cargo_Wall_Right"), 180.0),
			make_part(
				m.get("SM_Cargo_Wall_Back"),
				loc_of("SM_Cargo_Wall_Back"),
				0.0,
				"Back_X0_Y0_Z0",
				door_fb,
			),
			make_part(m.get("SM_Cargo_Wall_Front_L"), loc_of("SM_Cargo_Wall_Front_L"), 180.0),
			make_part(m.get("SM_Cargo_Wall_Front_R"), loc_of("SM_Cargo_Wall_Front_R"), 180.0),
			make_part(None, (face_offset_for_cells(2), 0.0, 0.0), 180.0, "Front_X1_Y0_Z0", door_fb),
		]

	author_module(
		"CargoHold_Training_01",
		"Cargo hold 2x1",
		unreal.ShipModuleType.CARGO_HOLD,
		(2, 1, 1),
		[
			"SM_Cargo_Floor",
			"SM_Cargo_Ceiling",
			"SM_Cargo_Wall_Left",
			"SM_Cargo_Wall_Right",
			"SM_Cargo_Wall_Back",
			"SM_Cargo_Wall_Front_L",
			"SM_Cargo_Wall_Front_R",
		],
		cargo_parts,
	)

	# Bridge is authored exclusively by author_bridge_module.py (unified shell + canopy).
	unreal.log("skip Bridge_Training_01 — use author_bridge_module.py")

	unreal.log("=== author_training_ship_modules done ===")


main()
