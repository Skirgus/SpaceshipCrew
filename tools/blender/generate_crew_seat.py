"""
Откидное кресло экипажа (jump seat) по референсу КреслоРеференс.

1 Blender unit = 1 cm.
Перед кресла (колени) = Blender −Y = UE +X.
Низ на Z=0, origin в центре пятна на полу.

Запуск: Blender MCP exec этого файла, либо
  blender --background --python tools/blender/generate_crew_seat.py
"""

from __future__ import annotations

import math
from pathlib import Path

import bmesh
import bpy
import mathutils

PROJECT_ROOT = Path(__file__).resolve().parents[2]
EXPORT_DIR = PROJECT_ROOT / "Content" / "Meshes" / "Equipment"
EXPORT_NAME = "SM_CrewSeat.fbx"

# Откинуто назад: верх спинки уходит в +Y (корма в Blender).
RECLINE = math.radians(-12.0)
HINGE = mathutils.Vector((0.0, 14.0, 50.0))
# Сиденье выше голени манекена. Верх кресла опускается, нога и платформа остаются на полу.
SIT_DROP = 22.0
# Передний край подушки. −30 уводил кромку на 10 см позади колена (UE y≈21 при колене ≈30).
SEAT_FRONT_Y = -37.0
# Низ передней грани спинки после центрирования: UE y = −эта величина. Спинка не должна уехать при укорочении сиденья.
BACKREST_FRONT_BLENDER_Y = 19.0
LOWER_RIG = {
	"FloorPlate",
	"Foot",
	"HingeLow",
	"HingeLowCap",
	"LinkLow",
	"HingeMid",
	"HingeMidCap",
	"LockLever",
	"LockKnob",
	"LinkUp",
}


def _clear_meshes() -> None:
	for obj in list(bpy.data.objects):
		if obj.type == "MESH":
			bpy.data.objects.remove(obj, do_unlink=True)
	for block in list(bpy.data.meshes):
		if block.users == 0:
			bpy.data.meshes.remove(block)


def _ensure_units() -> None:
	scene = bpy.context.scene
	scene.unit_settings.system = "METRIC"
	scene.unit_settings.scale_length = 0.01
	scene.view_settings.view_transform = "Standard"


def _mat(name: str, color: tuple[float, float, float], roughness: float, metallic: float) -> bpy.types.Material:
	existing = bpy.data.materials.get(name)
	mat = existing or bpy.data.materials.new(name)
	mat.use_nodes = True
	bsdf = next(node for node in mat.node_tree.nodes if node.type == "BSDF_PRINCIPLED")
	bsdf.inputs["Base Color"].default_value = (color[0], color[1], color[2], 1.0)
	bsdf.inputs["Roughness"].default_value = roughness
	bsdf.inputs["Metallic"].default_value = metallic
	mat.diffuse_color = (color[0], color[1], color[2], 1.0)
	return mat


def _apply_rot_scale(obj: bpy.types.Object) -> None:
	bpy.ops.object.select_all(action="DESELECT")
	obj.select_set(True)
	bpy.context.view_layer.objects.active = obj
	bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)


def _bevel(obj: bpy.types.Object, width: float, segments: int = 2) -> None:
	if width <= 0.05:
		return
	mod = obj.modifiers.new("Bevel", "BEVEL")
	mod.width = width
	mod.segments = segments
	try:
		mod.limit_method = "ANGLE"
		mod.angle_limit = math.radians(35.0)
	except Exception:
		pass
	bpy.ops.object.select_all(action="DESELECT")
	obj.select_set(True)
	bpy.context.view_layer.objects.active = obj
	bpy.ops.object.modifier_apply(modifier=mod.name)


def _shade(obj: bpy.types.Object) -> None:
	bpy.ops.object.select_all(action="DESELECT")
	obj.select_set(True)
	bpy.context.view_layer.objects.active = obj
	bpy.ops.object.shade_smooth()


def _box(
	name: str,
	size: tuple[float, float, float],
	location: tuple[float, float, float],
	rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
	bevel: float = 0.6,
	material: bpy.types.Material | None = None,
) -> bpy.types.Object:
	bpy.ops.mesh.primitive_cube_add(size=1.0, location=location, rotation=rotation)
	obj = bpy.context.active_object
	obj.name = name
	obj.scale = size
	_apply_rot_scale(obj)
	_bevel(obj, min(bevel, min(size) * 0.35))
	if material:
		obj.data.materials.append(material)
	_shade(obj)
	return obj


def _cyl(
	name: str,
	radius: float,
	depth: float,
	location: tuple[float, float, float],
	rotation: tuple[float, float, float],
	material: bpy.types.Material | None,
	vertices: int = 20,
) -> bpy.types.Object:
	bpy.ops.mesh.primitive_cylinder_add(
		vertices=vertices,
		radius=radius,
		depth=depth,
		location=location,
		rotation=rotation,
	)
	obj = bpy.context.active_object
	obj.name = name
	_apply_rot_scale(obj)
	if material:
		obj.data.materials.append(material)
	_shade(obj)
	return obj


def _recline(local: mathutils.Vector) -> tuple[tuple[float, float, float], tuple[float, float, float]]:
	rotated = mathutils.Matrix.Rotation(RECLINE, 4, "X") @ local
	world = HINGE + rotated
	return (world.x, world.y, world.z), (RECLINE, 0.0, 0.0)


def _back_box(
	name: str,
	size: tuple[float, float, float],
	local: tuple[float, float, float],
	bevel: float,
	material: bpy.types.Material,
) -> bpy.types.Object:
	location, rotation = _recline(mathutils.Vector(local))
	return _box(name, size, location, rotation, bevel, material)


def _annulus(
	name: str,
	outer_r: float,
	inner_r: float,
	depth: float,
	location: tuple[float, float, float],
	rotation: tuple[float, float, float],
	material: bpy.types.Material | None,
) -> bpy.types.Object:
	bpy.ops.mesh.primitive_cylinder_add(
		vertices=48,
		radius=outer_r,
		depth=depth,
		location=location,
		rotation=rotation,
	)
	outer = bpy.context.active_object
	outer.name = name
	bpy.ops.mesh.primitive_cylinder_add(
		vertices=40,
		radius=inner_r,
		depth=depth + 6.0,
		location=location,
		rotation=rotation,
	)
	hole = bpy.context.active_object
	hole.name = f"CUTTER_{name}"
	mod = outer.modifiers.new("Hole", "BOOLEAN")
	mod.operation = "DIFFERENCE"
	mod.object = hole
	try:
		mod.solver = "EXACT"
	except TypeError:
		pass
	bpy.ops.object.select_all(action="DESELECT")
	outer.select_set(True)
	bpy.context.view_layer.objects.active = outer
	bpy.ops.object.modifier_apply(modifier=mod.name)
	bpy.data.objects.remove(hole, do_unlink=True)
	outer.data.materials.clear()
	if material:
		outer.data.materials.append(material)
		for poly in outer.data.polygons:
			poly.material_index = 0
	_shade(outer)
	return outer


def _bar(
	name: str,
	start: tuple[float, float, float],
	end: tuple[float, float, float],
	size_xy: tuple[float, float],
	material: bpy.types.Material,
	bevel: float = 0.35,
) -> bpy.types.Object:
	a = mathutils.Vector(start)
	b = mathutils.Vector(end)
	delta = b - a
	length = max(delta.length, 0.1)
	mid = (a + b) * 0.5
	rotation = delta.to_track_quat("Z", "Y").to_euler()
	return _box(name, (size_xy[0], size_xy[1], length), (mid.x, mid.y, mid.z), rotation, bevel, material)


def _holed_plate(
	name: str,
	size: tuple[float, float, float],
	location: tuple[float, float, float],
	hole_radius: float,
	hole_location: tuple[float, float, float],
	material: bpy.types.Material | None,
) -> bpy.types.Object:
	bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
	plate = bpy.context.active_object
	plate.name = name
	plate.scale = size
	_apply_rot_scale(plate)
	bpy.ops.mesh.primitive_cylinder_add(
		vertices=48,
		radius=hole_radius,
		depth=size[0] + 8.0,
		location=hole_location,
		rotation=(0.0, math.pi / 2.0, 0.0),
	)
	hole = bpy.context.active_object
	hole.name = f"CUTTER_{name}"
	mod = plate.modifiers.new("Hole", "BOOLEAN")
	mod.operation = "DIFFERENCE"
	mod.object = hole
	try:
		mod.solver = "EXACT"
	except TypeError:
		pass
	bpy.ops.object.select_all(action="DESELECT")
	plate.select_set(True)
	bpy.context.view_layer.objects.active = plate
	bpy.ops.object.modifier_apply(modifier=mod.name)
	bpy.data.objects.remove(hole, do_unlink=True)
	_bevel(plate, 0.7)
	plate.data.materials.clear()
	if material:
		plate.data.materials.append(material)
		for poly in plate.data.polygons:
			poly.material_index = 0
	_shade(plate)
	return plate


def _assign_mat(obj: bpy.types.Object, material: bpy.types.Material | None) -> None:
	obj.data.materials.clear()
	if not material:
		return
	obj.data.materials.append(material)
	for poly in obj.data.polygons:
		poly.material_index = 0


def _bool_apply(target: bpy.types.Object, tool: bpy.types.Object, operation: str) -> bpy.types.Object:
	mod = target.modifiers.new("Bool", "BOOLEAN")
	mod.operation = operation
	mod.object = tool
	try:
		mod.solver = "EXACT"
	except TypeError:
		pass
	bpy.ops.object.select_all(action="DESELECT")
	target.select_set(True)
	bpy.context.view_layer.objects.active = target
	bpy.ops.object.modifier_apply(modifier=mod.name)
	bpy.data.objects.remove(tool, do_unlink=True)
	return target


def _profile_plate(
	name: str,
	points: list[tuple[float, float]],
	x: float,
	thickness: float,
	material: bpy.types.Material | None,
) -> bpy.types.Object:
	mesh = bpy.data.meshes.new(name)
	bm = bmesh.new()
	verts = [bm.verts.new((x, y, z)) for y, z in points]
	face = bm.faces.new(verts)
	extruded = bmesh.ops.extrude_face_region(bm, geom=[face])
	new_verts = [item for item in extruded["geom"] if isinstance(item, bmesh.types.BMVert)]
	direction = thickness if x >= 0.0 else -thickness
	bmesh.ops.translate(bm, verts=new_verts, vec=(direction, 0.0, 0.0))
	bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
	bm.to_mesh(mesh)
	bm.free()
	obj = bpy.data.objects.new(name, mesh)
	bpy.context.scene.collection.objects.link(obj)
	_assign_mat(obj, material)
	return obj


def _cutter_disc(
	radius: float,
	depth: float,
	location: tuple[float, float, float],
) -> bpy.types.Object:
	bpy.ops.mesh.primitive_cylinder_add(
		vertices=28,
		radius=radius,
		depth=depth,
		location=location,
		rotation=(0.0, math.pi / 2.0, 0.0),
	)
	cutter = bpy.context.active_object
	cutter.name = "CUTTER_temp"
	return cutter


def _mech_outline() -> list[tuple[float, float]]:
	"""Дуга механизма плюс клюв вперёд. Не прямоугольник и не голое кольцо."""
	cy, cz, radius = -2.0, 50.0, 21.0
	points: list[tuple[float, float]] = []
	steps = 28
	# Длинный путь: низ спереди, зад, верх. Передок закрывает клюв.
	for index in range(steps):
		angle = math.radians(-200.0 + 228.0 * index / (steps - 1))
		points.append((cy - radius * math.sin(angle), cz + radius * math.cos(angle)))
	points.extend([(-32.0, 68.0), (-48.0, 54.0), (-48.0, 38.0), (-34.0, 30.0)])
	return points


def build_crew_seat() -> Path:
	"""Механизм на одной шарнирной ноге, а не на двух палках."""
	_clear_meshes()
	_ensure_units()

	fabric = _mat("M_SeatFabric", (0.48, 0.42, 0.32), 0.82, 0.0)
	fabric_dark = _mat("M_SeatFabricDark", (0.28, 0.24, 0.18), 0.86, 0.0)
	metal = _mat("M_SeatMetal", (0.62, 0.61, 0.57), 0.38, 0.72)
	metal_dark = _mat("M_SeatMetalDark", (0.10, 0.10, 0.11), 0.7, 0.05)

	axis_x = (0.0, math.pi / 2.0, 0.0)
	ring_y, ring_z = -2.0, 50.0

	# Платформа и одна нога: зигзаг из двух толстых звеньев и трёх шарниров.
	_box("FloorPlate", (68.0, 58.0, 4.0), (0.0, -4.0, 2.0), bevel=0.35, material=metal_dark)
	_box("Foot", (26.0, 22.0, 6.0), (0.0, 2.0, 6.5), bevel=0.45, material=metal_dark)
	_cyl("HingeLow", 7.2, 18.0, (0.0, 4.0, 14.0), axis_x, metal, vertices=24)
	_cyl("HingeLowCap", 3.2, 20.0, (0.0, 4.0, 14.0), axis_x, metal_dark, vertices=16)
	_bar("LinkLow", (0.0, 4.0, 16.0), (0.0, -16.0, 32.0), (12.0, 9.0), metal_dark, bevel=0.6)
	_cyl("HingeMid", 8.4, 20.0, (0.0, -16.0, 32.0), axis_x, metal, vertices=28)
	_cyl("HingeMidCap", 3.6, 22.5, (0.0, -16.0, 32.0), axis_x, metal_dark, vertices=16)
	# Рычаг фиксации — читается как регулировка, торчит вбок.
	_bar("LockLever", (0.0, -16.0, 32.0), (16.0, -22.0, 36.0), (2.4, 2.4), metal, bevel=0.25)
	_cyl("LockKnob", 2.2, 2.4, (17.2, -22.6, 36.4), (0.0, math.pi / 2.0, 0.0), metal_dark, vertices=12)
	_bar("LinkUp", (0.0, -16.0, 34.0), (0.0, -2.0, 46.0 - SIT_DROP), (11.0, 8.5), metal_dark, bevel=0.55)
	_cyl("HingeTop", 6.8, 16.0, (0.0, -2.0, 46.0), axis_x, metal, vertices=24)
	_bar("Yoke", (-24.0, -2.0, 44.0), (24.0, -2.0, 44.0), (5.0, 6.0), metal_dark, bevel=0.4)

	for sign, label in ((-1.0, "L"), (1.0, "R")):
		x = sign * 27.0
		inner_x = sign * 24.4

		plate = _profile_plate(f"Mech_{label}", _mech_outline(), inner_x, 5.4, metal)
		_bool_apply(plate, _cutter_disc(11.5, 14.0, (x, ring_y, ring_z)), "DIFFERENCE")
		_bool_apply(plate, _cutter_disc(4.0, 14.0, (x, -38.0, 48.0)), "DIFFERENCE")
		_assign_mat(plate, metal)
		bpy.ops.object.select_all(action="DESELECT")
		plate.select_set(True)
		bpy.context.view_layer.objects.active = plate
		bpy.ops.object.shade_flat()

		web = _profile_plate(
			f"Web_{label}",
			[(-14.0, 40.0), (-15.0, 56.0), (2.0, 58.0), (8.0, 46.0), (2.0, 38.0), (-8.0, 38.0)],
			sign * 25.8,
			2.6,
			metal_dark,
		)
		_bool_apply(web, _cutter_disc(3.4, 8.0, (x, -10.0, 46.0)), "DIFFERENCE")
		_bool_apply(web, _cutter_disc(2.8, 8.0, (x, -2.0, 42.0)), "DIFFERENCE")
		_assign_mat(web, metal_dark)

		_cyl(
			f"ArmHub_{label}",
			radius=5.4,
			depth=6.2,
			location=(sign * 31.2, -38.0, 48.0),
			rotation=axis_x,
			material=metal,
			vertices=22,
		)
		_bar(
			f"Post_{label}",
			(x, 6.0, 66.0),
			(x, 16.0, 124.0),
			(6.6, 7.4),
			metal_dark,
			bevel=0.45,
		)

	_bar("HeadBar", (-30.0, 22.0, 112.0), (30.0, 22.0, 112.0), (4.6, 5.0), metal, bevel=0.4)

	for sign, label in ((-1.0, "L"), (1.0, "R")):
		_bar(
			f"SeatRail_{label}",
			(sign * 23.0, 6.0, 58.0),
			(sign * 23.0, SEAT_FRONT_Y, 56.0),
			(4.2, 5.0),
			metal,
			bevel=0.35,
		)
	_bar("SeatFrontRail", (-23.0, SEAT_FRONT_Y, 56.0), (23.0, SEAT_FRONT_Y, 56.0), (4.2, 4.2), metal, bevel=0.35)
	for index, y in enumerate((2.0, -8.0, -18.0, -29.0)):
		_box(
			f"Webbing_{index}",
			(44.0, 8.8, 6.4),
			(0.0, y, 62.0),
			bevel=0.8,
			material=fabric,
		)

	# Спинка между стойками, чуть откинута.
	_back_box("BackPad", (40.0, 6.4, 56.0), (0.0, -4.0, 30.0), 1.2, fabric)
	for index, x in enumerate((-8.0, 8.0)):
		_back_box(
			f"BackChannel_{index}",
			(1.8, 0.7, 46.0),
			(x, -6.8, 28.0),
			0.15,
			fabric_dark,
		)
	_back_box("Headrest", (28.0, 6.2, 16.0), (0.0, -6.0, 66.0), 1.8, fabric)

	# Рычаги подлокотника. Средний шарнир — точка выноски на виде сбоку.
	for sign, label in ((-1.0, "L"), (1.0, "R")):
		x = sign * 34.0
		hub = (x, -38.0, 50.0)
		rear = (x, -16.0, 74.0)
		front = (x, -38.0, 70.0)
		_bar(f"ArmUp_{label}", hub, rear, (4.0, 5.4), metal, bevel=0.4)
		_bar(f"ArmFore_{label}", hub, front, (4.0, 5.4), metal, bevel=0.4)
		_bar(f"ArmTop_{label}", rear, front, (3.6, 4.8), metal, bevel=0.35)
		_cyl(
			f"ArmRearJoint_{label}",
			radius=3.2,
			depth=5.0,
			location=rear,
			rotation=axis_x,
			material=metal,
			vertices=16,
		)
		_box(
			f"ArmPad_{label}",
			(8.5, 22.0, 3.0),
			(sign * 31.0, -22.0, 79.0),
			bevel=0.4,
			material=fabric,
		)

	for obj in list(bpy.context.scene.objects):
		if obj.type != "MESH" or obj.name in LOWER_RIG or obj.name.startswith("CUTTER_"):
			continue
		obj.location.z -= SIT_DROP

	return _join_and_export()


def _join_and_export() -> Path:
	meshes = [
		obj
		for obj in bpy.context.scene.objects
		if obj.type == "MESH" and not obj.name.startswith("CUTTER_")
	]
	active = meshes[0]
	active_inv = active.matrix_world.inverted()

	def _world_front(name: str) -> mathutils.Vector:
		obj = bpy.data.objects[name]
		points = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
		low_z = min(point.z for point in points)
		band = [point for point in points if point.z <= low_z + 4.0]
		return min(band, key=lambda point: point.y)

	back_local = active_inv @ _world_front("BackPad")
	rail_local = active_inv @ _world_front("SeatFrontRail")

	bpy.ops.object.select_all(action="DESELECT")
	for obj in meshes:
		obj.select_set(True)
	bpy.context.view_layer.objects.active = active
	bpy.ops.object.join()
	seat = bpy.context.active_object
	seat.name = "SM_CrewSeat"

	xs = [vertex.co.x for vertex in seat.data.vertices]
	ys = [vertex.co.y for vertex in seat.data.vertices]
	zs = [vertex.co.z for vertex in seat.data.vertices]
	cx = (min(xs) + max(xs)) * 0.5
	cy = (min(ys) + max(ys)) * 0.5
	min_z = min(zs)
	# Укорочение сиденья сдвигает центр bounds. Возвращаем низ спинки на прежнее место.
	y_shift = BACKREST_FRONT_BLENDER_Y - (back_local.y - cy)
	for vertex in seat.data.vertices:
		vertex.co.x -= cx
		vertex.co.y -= cy - y_shift
		vertex.co.z -= min_z
	seat.data.update()
	seat.location = (0.0, 0.0, 0.0)

	back_y = -(back_local.y - cy + y_shift)
	back_z = back_local.z - min_z
	rail_y = -(rail_local.y - cy + y_shift)
	rail_z = rail_local.z - min_z
	print(
		"[CrewSeat] locked backrest UE y=%.1f z=%.1f seat_front UE y=%.1f z=%.1f shift=%.1f"
		% (back_y, back_z, rail_y, rail_z, y_shift)
	)

	xs = [vertex.co.x for vertex in seat.data.vertices]
	ys = [vertex.co.y for vertex in seat.data.vertices]
	zs = [vertex.co.z for vertex in seat.data.vertices]
	print(
		"[CrewSeat] blender bounds",
		f"X {min(xs):.1f}..{max(xs):.1f}",
		f"Y {min(ys):.1f}..{max(ys):.1f}",
		f"Z {min(zs):.1f}..{max(zs):.1f}",
	)
	# Измеренное соответствие этого экспорта: UE (x, y, z) = (bx, -by, bz).
	print(
		"[CrewSeat] UE size",
		f"X {min(xs):.1f}..{max(xs):.1f}",
		f"Y {-max(ys):.1f}..{-min(ys):.1f}",
		f"Z {min(zs):.1f}..{max(zs):.1f}",
	)

	_frame_viewport(seat)
	return _export(seat)


def _export(obj: bpy.types.Object) -> Path:
	EXPORT_DIR.mkdir(parents=True, exist_ok=True)
	path = EXPORT_DIR / EXPORT_NAME
	bpy.ops.object.select_all(action="DESELECT")
	obj.select_set(True)
	bpy.context.view_layer.objects.active = obj
	bpy.ops.export_scene.fbx(
		filepath=str(path),
		use_selection=True,
		apply_unit_scale=True,
		apply_scale_options="FBX_SCALE_NONE",
		axis_forward="-Y",
		axis_up="Z",
		mesh_smooth_type="FACE",
	)
	print("[CrewSeat] exported", path)
	return path


def _frame_viewport(seat: bpy.types.Object) -> None:
	world = bpy.context.scene.world
	if world and world.use_nodes:
		bg = next((node for node in world.node_tree.nodes if node.type == "BACKGROUND"), None)
		if bg:
			bg.inputs["Color"].default_value = (0.04, 0.045, 0.05, 1.0)
			bg.inputs["Strength"].default_value = 0.4

	for obj in list(bpy.data.objects):
		if obj.type == "LIGHT":
			bpy.data.objects.remove(obj, do_unlink=True)
	bpy.ops.object.light_add(type="SUN", location=(0.0, 0.0, 300.0))
	sun = bpy.context.active_object
	sun.data.energy = 4.2
	sun.rotation_euler = (math.radians(52.0), math.radians(8.0), math.radians(-28.0))
	bpy.ops.object.light_add(type="SUN", location=(0.0, 0.0, 200.0))
	fill = bpy.context.active_object
	fill.data.energy = 1.4
	fill.rotation_euler = (math.radians(70.0), 0.0, math.radians(140.0))

	cam = bpy.data.objects.get("Camera")
	if cam is None:
		bpy.ops.object.camera_add()
		cam = bpy.context.active_object
	target = mathutils.Vector((0.0, 0.0, 58.0))
	cam.location = mathutils.Vector((-170.0, 0.0, 55.0))
	direction = target - cam.location
	cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
	cam.data.type = "ORTHO"
	cam.data.ortho_scale = 150.0
	cam.data.clip_start = 0.1
	cam.data.clip_end = 10000.0
	bpy.context.scene.camera = cam

	for area in bpy.context.screen.areas:
		if area.type != "VIEW_3D":
			continue
		for space in area.spaces:
			if space.type != "VIEW_3D":
				continue
			space.region_3d.view_perspective = "CAMERA"
			space.shading.type = "MATERIAL"
			space.shading.use_scene_lights = True
			space.clip_start = 1.0
			space.clip_end = 10000.0
	seat.select_set(False)


if __name__ == "__main__":
	build_crew_seat()
