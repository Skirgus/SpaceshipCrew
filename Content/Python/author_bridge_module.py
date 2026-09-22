# Author Bridge_Training_01 — unified shell + corridor floor/dock.
# Output Log: py "H:/Projects/SpaceshipCrew/Content/Python/author_bridge_module.py"

import unreal
import os
import math
import json
import sys

sys.path.insert(0, unreal.Paths.project_content_dir() + "Python")
from ship_builder_grid import PANEL_XY, PANEL_Z, FLOOR_REL_Z, face_offset_for_cells

MESH_DIR = "/Game/Meshes/TrainingShip"
FBX_DIR = unreal.Paths.project_content_dir() + "Meshes/TrainingShip/"
MODULE_DIR = "/Game/Data/ShipModules"
CORRIDOR_DOOR_FB = "/Game/Meshes/CorridorPanels/SM_Wall_Door_FB_400x300"
CORRIDOR_SOLID_FB = "/Game/Meshes/CorridorPanels/SM_Wall_Solid_FB_400x300"
CORRIDOR_FLOOR = "/Game/Meshes/CorridorPanels/SM_Floor_400"
PLACEMENTS_PATH = unreal.Paths.project_content_dir() + "Meshes/TrainingShip/part_placements.json"

CELL = (2, 1, 1)
BACK_X = -face_offset_for_cells(CELL[0])
SCALE = 1.0
PLACEMENTS = {}

BRIDGE_MESHES = [
	"SM_Bridge_Shell",
	"SM_Bridge_Floor",
	"SM_Bridge_Porthole_Rim",
	"SM_Bridge_Porthole_Glass",
	"SM_Bridge_Canopy_Collar",
	"SM_Bridge_Canopy_SideWalls",
	"SM_Bridge_Canopy_Glass",
	"SM_Bridge_Canopy_Frame",
]

DECO_NO_COLLISION = set(BRIDGE_MESHES)  # shell gets complex collision separately


def load_placements():
	global PLACEMENTS
	if os.path.isfile(PLACEMENTS_PATH):
		with open(PLACEMENTS_PATH, "r", encoding="utf-8") as f:
			raw = json.load(f)
		PLACEMENTS = {k: tuple(v) for k, v in raw.items()}
	print("placements:", len(PLACEMENTS))


def loc_of(name, fallback=(0.0, 0.0, 0.0)):
	v = PLACEMENTS.get(name, fallback)
	return (float(v[0]), float(v[1]), float(v[2]))


def pitch_of(name, fallback=0.0):
	v = PLACEMENTS.get(name)
	if v and len(v) >= 4:
		return float(v[3])
	return float(fallback)


def import_mesh(mesh_name, uniform_scale):
	path = "%s/%s" % (MESH_DIR, mesh_name)
	fbx_path = FBX_DIR + mesh_name + ".fbx"
	if not os.path.isfile(fbx_path):
		print("FBX missing", fbx_path)
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
	# Floor gets an explicit box later; auto convex fills hollow shell/collar and blocks walk.
	options.static_mesh_import_data.set_editor_property("auto_generate_collision", False)
	options.static_mesh_import_data.set_editor_property("import_uniform_scale", uniform_scale)
	options.static_mesh_import_data.set_editor_property("convert_scene", True)
	task.options = options

	unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
	mesh = unreal.EditorAssetLibrary.load_asset(path)
	if mesh:
		e = mesh.get_bounds().box_extent * 2.0
		print("Imported %s size=(%.0f,%.0f,%.0f)" % (mesh_name, e.x, e.y, e.z))
		unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
	return mesh


def pick_scale(sample_name, expect):
	for scale in (1.0, 100.0, 0.01):
		mesh = import_mesh(sample_name, scale)
		if not mesh:
			continue
		e = mesh.get_bounds().box_extent * 2.0
		if expect * 0.5 < max(e.x, e.y) < expect * 1.5:
			return scale
	return 1.0


def make_tr(location=(0.0, 0.0, 0.0), yaw_deg=0.0, pitch_deg=0.0):
	tr = unreal.Transform()
	tr.translation = unreal.Vector(float(location[0]), float(location[1]), float(location[2]))
	yaw_rad = math.radians(float(yaw_deg))
	pitch_rad = math.radians(float(pitch_deg))
	# Quat(X,Y,Z,W). Pitch around Y, then yaw around Z.
	qy = unreal.Quat(0.0, math.sin(pitch_rad * 0.5), 0.0, math.cos(pitch_rad * 0.5))
	qz = unreal.Quat(0.0, 0.0, math.sin(yaw_rad * 0.5), math.cos(yaw_rad * 0.5))
	tr.rotation = qz * qy
	tr.scale3d = unreal.Vector(1.0, 1.0, 1.0)
	return tr


def make_part(mesh, location=(0.0, 0.0, 0.0), yaw_deg=0.0, wall_socket=None, opening_mesh=None, pitch_deg=0.0):
	part = unreal.ShipModuleVisualPart()
	if mesh:
		part.set_editor_property("mesh", mesh)
	part.set_editor_property("relative_transform", make_tr(location, yaw_deg, pitch_deg))
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


def clear_collision(mesh):
	if not mesh:
		return
	try:
		unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
		bs = mesh.get_editor_property("body_setup")
		name = mesh.get_name()
		if bs:
			# Default: no collision shapes. Floor/Glass/Frame get setup in main().
			bs.set_editor_property(
				"collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_DEFAULT
			)
		unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
		print("collision cleared:", name)
	except Exception as exc:
		print("collision fail", mesh.get_name() if mesh else None, exc)


def setup_walk_and_canopy_collision(meshes):
	"""Floor = box walk; Shell/cheeks/glass/frames/portholes = complex (open nose stays open)."""
	floor = meshes.get("SM_Bridge_Floor")
	if floor:
		try:
			unreal.EditorStaticMeshLibrary.remove_collisions(floor)
			unreal.EditorStaticMeshLibrary.add_simple_collisions(
				floor, unreal.ScriptingCollisionShapeType.BOX
			)
			bs = floor.get_editor_property("body_setup")
			if bs:
				bs.set_editor_property(
					"collision_trace_flag",
					unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX,
				)
			unreal.EditorAssetLibrary.save_asset(floor.get_path_name(), only_if_is_dirty=False)
			print("floor walk collision: BOX")
		except Exception as exc:
			print("floor collision", exc)

	# Complex matches hollow/open geometry — do NOT use convex (fills cabin).
	complex_keys = (
		"SM_Bridge_Shell",
		"SM_Bridge_Canopy_SideWalls",
		"SM_Bridge_Canopy_Glass",
		"SM_Bridge_Canopy_Frame",
		"SM_Bridge_Porthole_Glass",
		"SM_Bridge_Porthole_Rim",
	)
	for key in complex_keys:
		mesh = meshes.get(key)
		if not mesh:
			continue
		try:
			unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
			bs = mesh.get_editor_property("body_setup")
			if bs:
				bs.set_editor_property(
					"collision_trace_flag",
					unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
				)
			unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
			print("wall/canopy collision complex:", key)
		except Exception as exc:
			print("complex collision", key, exc)


def ensure_color_material(mat_name, rgb):
	mat_dir = "/Game/Materials/TrainingShip"
	if not unreal.EditorAssetLibrary.does_directory_exist(mat_dir):
		unreal.EditorAssetLibrary.make_directory(mat_dir)
	path = "%s/%s" % (mat_dir, mat_name)
	mat = unreal.EditorAssetLibrary.load_asset(path)
	if not mat:
		mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
			mat_name, mat_dir, unreal.Material, unreal.MaterialFactoryNew()
		)
		c = unreal.MaterialEditingLibrary.create_material_expression(
			mat, unreal.MaterialExpressionConstant3Vector, -350, 0
		)
		c.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
		unreal.MaterialEditingLibrary.connect_material_property(
			c, "", unreal.MaterialProperty.MP_BASE_COLOR
		)
		unreal.MaterialEditingLibrary.recompile_material(mat)
	try:
		mat.set_editor_property("used_with_instanced_static_meshes", True)
	except Exception:
		pass
	unreal.EditorAssetLibrary.save_asset(mat.get_path_name(), only_if_is_dirty=False)
	return mat


def ensure_glass_material():
	"""Translucent blue-tinted glass for canopy + roof portholes."""
	mat_dir = "/Game/Materials/TrainingShip"
	if not unreal.EditorAssetLibrary.does_directory_exist(mat_dir):
		unreal.EditorAssetLibrary.make_directory(mat_dir)
	path = "%s/M_Bridge_Glass" % mat_dir
	mat = unreal.EditorAssetLibrary.load_asset(path)
	if not mat:
		mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
			"M_Bridge_Glass", mat_dir, unreal.Material, unreal.MaterialFactoryNew()
		)
	# Force translucent glass setup every author pass
	try:
		mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
	except Exception:
		try:
			mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_Translucent)
		except Exception:
			pass
	try:
		mat.set_editor_property("two_sided", True)
	except Exception:
		pass
	try:
		mat.set_editor_property("used_with_instanced_static_meshes", True)
	except Exception:
		pass

	# Clear old expressions and rebuild simple glass graph
	try:
		for expr in list(unreal.MaterialEditingLibrary.get_material_expressions(mat)):
			unreal.MaterialEditingLibrary.delete_material_expression(mat, expr)
	except Exception:
		pass

	base = unreal.MaterialEditingLibrary.create_material_expression(
		mat, unreal.MaterialExpressionConstant3Vector, -400, -80
	)
	base.set_editor_property("constant", unreal.LinearColor(0.15, 0.35, 0.45, 1.0))
	unreal.MaterialEditingLibrary.connect_material_property(
		base, "", unreal.MaterialProperty.MP_BASE_COLOR
	)

	opacity = unreal.MaterialEditingLibrary.create_material_expression(
		mat, unreal.MaterialExpressionConstant, -400, 40
	)
	opacity.set_editor_property("r", 0.35)
	unreal.MaterialEditingLibrary.connect_material_property(
		opacity, "", unreal.MaterialProperty.MP_OPACITY
	)

	rough = unreal.MaterialEditingLibrary.create_material_expression(
		mat, unreal.MaterialExpressionConstant, -400, 120
	)
	rough.set_editor_property("r", 0.05)
	unreal.MaterialEditingLibrary.connect_material_property(
		rough, "", unreal.MaterialProperty.MP_ROUGHNESS
	)

	spec = unreal.MaterialEditingLibrary.create_material_expression(
		mat, unreal.MaterialExpressionConstant, -400, 200
	)
	spec.set_editor_property("r", 0.85)
	unreal.MaterialEditingLibrary.connect_material_property(
		spec, "", unreal.MaterialProperty.MP_SPECULAR
	)

	unreal.MaterialEditingLibrary.recompile_material(mat)
	unreal.EditorAssetLibrary.save_asset(mat.get_path_name(), only_if_is_dirty=False)
	print("glass material ready", path)
	return mat


def assign_mats(meshes):
	mats = {
		"hull": ensure_color_material("M_Bridge_Hull", (0.78, 0.76, 0.72)),
		"glass": ensure_glass_material(),
		"metal": ensure_color_material("M_Bridge_Metal", (0.25, 0.26, 0.28)),
		"floor": ensure_color_material("M_Bridge_Floor", (0.18, 0.19, 0.21)),
	}
	for name, mesh in meshes.items():
		if not mesh:
			continue
		if "Glass" in name or "Porthole_Glass" in name or "Canopy_Glass" in name:
			mat = mats["glass"]
		elif "Thruster" in name or "Canopy_Frame" in name or "Porthole_Rim" in name:
			mat = mats["metal"]
		elif "Floor" in name:
			mat = mats.get("floor") or ensure_color_material("M_Bridge_Floor", (0.18, 0.19, 0.21))
		else:
			mat = mats["hull"]
		try:
			mesh.set_material(0, mat)
			unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
		except Exception as exc:
			print("mat", name, exc)


def main():
	global SCALE
	marker = unreal.Paths.project_saved_dir() + "Logs/author_bridge_marker.txt"
	print("=== author_bridge_module start (unified shell) ===")
	print("BACK_X=%.0f FLOOR_REL_Z=%.0f" % (BACK_X, FLOOR_REL_Z))
	try:
		load_placements()
		SCALE = pick_scale("SM_Bridge_Shell", PANEL_XY * 1.5)
		print("scale", SCALE)

		door_fb = unreal.EditorAssetLibrary.load_asset(CORRIDOR_DOOR_FB)
		solid_fb = unreal.EditorAssetLibrary.load_asset(CORRIDOR_SOLID_FB)
		if not door_fb or not solid_fb:
			raise RuntimeError("corridor panels missing")

		meshes = {n: import_mesh(n, SCALE) for n in BRIDGE_MESHES}
		for n, m in meshes.items():
			clear_collision(m)
		setup_walk_and_canopy_collision(meshes)
		assign_mats(meshes)

		parts = []
		# Dock face — corridor FB (aligns with neighbors)
		parts.append(make_part(solid_fb, (BACK_X, 0.0, 0.0), 0.0, "Back_X0_Y0_Z0", door_fb))
		# Hull, tapered floor (inside walls), canopy, thrusters — no full-width corridor floors
		for n in BRIDGE_MESHES:
			parts.append(make_part(meshes.get(n), loc_of(n), pitch_deg=pitch_of(n)))

		module_id = "Bridge_Training_01"
		definition = create_or_load_asset(module_id, MODULE_DIR, unreal.ShipModuleDefinition, "ShipModuleDefinitionFactory")
		override = create_or_load_asset(
			"%s_VisualOverride" % module_id, MODULE_DIR, unreal.ShipModuleVisualOverride, "ShipModuleVisualOverrideFactory"
		)
		if not definition or not override:
			raise RuntimeError("no Def/VO")

		override.set_editor_property("visual_parts", parts)
		unreal.EditorAssetLibrary.save_asset(override.get_path_name(), only_if_is_dirty=False)

		definition.set_editor_property("module_id", unreal.Name(module_id))
		definition.set_editor_property("display_name", unreal.Text("Bridge (bow)"))
		definition.set_editor_property("module_type", unreal.ShipModuleType.BRIDGE)
		definition.set_editor_property("cell_size", unreal.IntVector(CELL[0], CELL[1], CELL[2]))
		definition.set_editor_property("has_interior", True)
		definition.set_editor_property("mass", float(CELL[0] * CELL[1] * 150))
		definition.set_editor_property("credit_cost", int(CELL[0] * CELL[1] * 200))
		definition.set_editor_property("visual_override", override)
		try:
			definition.set_editor_property(
				"size", unreal.Vector(CELL[0] * PANEL_XY, CELL[1] * PANEL_XY, CELL[2] * PANEL_Z)
			)
		except Exception:
			pass
		definition.set_editor_property("contact_points", make_panel_contacts(CELL))
		unreal.EditorAssetLibrary.save_asset(definition.get_path_name(), only_if_is_dirty=False)
		unreal.EditorAssetLibrary.save_asset(override.get_path_name(), only_if_is_dirty=False)

		lines = []
		ok = True
		for i, p in enumerate(parts):
			tr = p.get_editor_property("relative_transform")
			sock = p.get_editor_property("wall_socket_name")
			mesh = p.get_editor_property("mesh")
			t = tr.translation
			line = "%s[%d] %s loc=(%.0f,%.0f,%.0f) sock=%s" % (
				module_id, i, mesh.get_name() if mesh else None, t.x, t.y, t.z, sock
			)
			print(line)
			lines.append(line)
			if sock and str(sock) not in ("None", "") and abs(t.x) < 1 and abs(t.y) < 1:
				ok = False
				print("BAD center dock", line)
			if str(sock) == "Back_X0_Y0_Z0" and abs(t.x - BACK_X) > 5:
				ok = False
				print("BAD back X", line)

		summary = "%s: parts=%d cell=%s ok=%s" % (module_id, len(parts), CELL, ok)
		print(summary)
		lines.append(summary)
		with open(marker, "w", encoding="utf-8") as f:
			f.write("\n".join(lines) + "\n")
		print("=== author_bridge_module done ===")
	except Exception as exc:
		import traceback
		tb = traceback.format_exc()
		print("FATAL", exc)
		print(tb)
		with open(marker, "w", encoding="utf-8") as f:
			f.write("FATAL\n%s\n%s\n" % (exc, tb))


main()
