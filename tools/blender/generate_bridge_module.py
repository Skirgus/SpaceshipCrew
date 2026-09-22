"""
Unified Bridge_Training_01 shell — one hollow HopeTech hull.

No nested “cone” cabin, no floating roof/walls as separate kits.
Grid: PanelUnitXY=800, PanelUnitZ=400. CellSize 2x1x1 = 1600x800x400.
Flat floor = corridor tiles in UE. Flat back dock = corridor FB panels.
No seats / props.

  blender --background --python tools/blender/generate_bridge_module.py
"""

from __future__ import annotations

import json
import math
from pathlib import Path

import bpy
import bmesh

PROJECT_ROOT = Path(__file__).resolve().parents[2]
EXPORT_DIR = PROJECT_ROOT / "Content" / "Meshes" / "TrainingShip"
PLACEMENTS_PATH = EXPORT_DIR / "part_placements.json"

PANEL_XY = 800.0
PANEL_Z = 400.0
CELL_X, CELL_Y = 2, 1
HALF_X = CELL_X * PANEL_XY * 0.5  # 800
HALF_Y = CELL_Y * PANEL_XY * 0.5  # 400
HALF_Z = PANEL_Z * 0.5  # 200
FLOOR_THICK = 16.0
FLOOR_REL_Z = -(HALF_Z - FLOOR_THICK * 0.5)  # -192
FLOOR_TOP_Z = FLOOR_REL_Z + FLOOR_THICK * 0.5  # -184
CEILING_Z = HALF_Z - 12.0  # 188
BACK_X = -(CELL_X * PANEL_XY * 0.5 - 9.0)  # -791
SHELL_THICK = 24.0

PLACEMENTS: dict[str, list[float]] = {}
MATS: dict[str, bpy.types.Material] = {}


def _clear_scene() -> None:
	bpy.ops.object.select_all(action="SELECT")
	bpy.ops.object.delete(use_global=False)
	for block in list(bpy.data.meshes):
		bpy.data.meshes.remove(block)
	for block in list(bpy.data.materials):
		bpy.data.materials.remove(block)


def _ensure_unit_cm() -> None:
	s = bpy.context.scene
	s.unit_settings.system = "METRIC"
	s.unit_settings.scale_length = 0.01


def _mat(name: str, color: tuple[float, float, float, float], rough=0.55, metallic=0.08, alpha=1.0):
	if name in MATS:
		return MATS[name]
	m = bpy.data.materials.new(name)
	m.use_nodes = True
	bsdf = next((n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
	if bsdf is None:
		bsdf = m.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
	bsdf.inputs["Base Color"].default_value = color
	bsdf.inputs["Roughness"].default_value = rough
	bsdf.inputs["Metallic"].default_value = metallic
	if alpha < 1.0:
		bsdf.inputs["Alpha"].default_value = alpha
		m.blend_method = "BLEND"
	MATS[name] = m
	return m


def _assign(obj, mat):
	if obj.data.materials:
		obj.data.materials[0] = mat
	else:
		obj.data.materials.append(mat)


def _apply_rs(obj):
	bpy.context.view_layer.objects.active = obj
	obj.select_set(True)
	bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
	obj.select_set(False)


def _origin_zero(obj):
	"""Local-center mesh; return world placement for RelativeTransform."""
	from mathutils import Vector

	bpy.ops.object.select_all(action="DESELECT")
	bpy.context.view_layer.objects.active = obj
	obj.select_set(True)
	bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

	me = obj.data
	if not me.vertices:
		obj.select_set(False)
		return (0.0, 0.0, 0.0)

	bm = bmesh.new()
	bm.from_mesh(me)
	bm.verts.ensure_lookup_table()
	xs = [v.co.x for v in bm.verts]
	ys = [v.co.y for v in bm.verts]
	zs = [v.co.z for v in bm.verts]
	center = Vector(
		(
			(min(xs) + max(xs)) * 0.5,
			(min(ys) + max(ys)) * 0.5,
			(min(zs) + max(zs)) * 0.5,
		)
	)
	bmesh.ops.translate(bm, verts=list(bm.verts), vec=-center)
	bm.to_mesh(me)
	bm.free()
	me.update()

	loc = (float(center.x), float(center.y), float(center.z))
	obj.location = loc
	obj.select_set(False)
	return loc


def _shade_smooth(obj):
	bpy.ops.object.select_all(action="DESELECT")
	bpy.context.view_layer.objects.active = obj
	obj.select_set(True)
	bpy.ops.object.shade_smooth()
	obj.select_set(False)


def _bevel(obj, width=5.0, segments=3):
	bpy.ops.object.select_all(action="DESELECT")
	bpy.context.view_layer.objects.active = obj
	obj.select_set(True)
	mod = obj.modifiers.new("Bevel", "BEVEL")
	mod.width = width
	mod.segments = segments
	mod.limit_method = "ANGLE"
	mod.angle_limit = math.radians(35.0)
	bpy.ops.object.modifier_apply(modifier=mod.name)
	obj.select_set(False)


def _box(name, size, loc):
	bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
	obj = bpy.context.active_object
	obj.name = name
	obj.scale = size
	_apply_rs(obj)
	return obj


def _cyl(name, radius, depth, loc, rot=(0, 0, 0), verts=24):
	bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc)
	obj = bpy.context.active_object
	obj.name = name
	obj.rotation_euler = rot
	_apply_rs(obj)
	obj.location = loc
	return obj


def _mesh(name, verts, faces):
	me = bpy.data.meshes.new(name)
	me.from_pydata(verts, [], faces)
	me.update()
	obj = bpy.data.objects.new(name, me)
	bpy.context.collection.objects.link(obj)
	bm = bmesh.new()
	bm.from_mesh(me)
	bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
	bm.to_mesh(me)
	bm.free()
	return obj


def _join(objects, name):
	bpy.ops.object.select_all(action="DESELECT")
	for o in objects:
		o.select_set(True)
	bpy.context.view_layer.objects.active = objects[0]
	if len(objects) > 1:
		bpy.ops.object.join()
	joined = bpy.context.active_object
	joined.name = name
	return joined


def _export(obj, filename):
	EXPORT_DIR.mkdir(parents=True, exist_ok=True)
	path = EXPORT_DIR / filename
	saved = obj.location.copy()
	obj.location = (0.0, 0.0, 0.0)
	bpy.context.view_layer.update()
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
	obj.location = saved
	bpy.context.view_layer.update()


def _register(obj, name, mat=None, pitch_deg=0.0):
	"""Export local-centered FBX; record placement."""
	obj.name = name
	if mat:
		_assign(obj, mat)
	# Always re-center — avoids double-offset when a later transform_apply hits multi-selection
	loc = _origin_zero(obj)
	entry = [round(loc[0], 3), round(loc[1], 3), round(loc[2], 3)]
	if abs(float(pitch_deg)) > 0.01:
		entry.append(round(float(pitch_deg), 3))
	PLACEMENTS[name] = entry
	_export(obj, f"{name}.fbx")
	obj.location = loc
	xs = [v.co.x for v in obj.data.vertices]
	print(f"export {name} @ {PLACEMENTS[name]} localX={min(xs):.1f}..{max(xs):.1f}")


def _lerp(a, b, t):
	return a + (b - a) * t


def _smooth(t):
	return t * t * (3.0 - 2.0 * t)


def _boolean(target, cutter, operation="DIFFERENCE"):
	bpy.context.view_layer.objects.active = target
	target.select_set(True)
	mod = target.modifiers.new("Bool", "BOOLEAN")
	mod.operation = operation
	mod.solver = "EXACT"
	mod.object = cutter
	bpy.ops.object.modifier_apply(modifier=mod.name)
	bpy.data.objects.remove(cutter, do_unlink=True)
	target.select_set(False)


# Outer silhouette stations: (x, half_y, z_bottom, z_top)
# Cabin stays wide through canopy threshold so the nose is walkable (hermetic + passable).
OUTER_STATIONS = [
	(BACK_X - 2.0, 392.0, -205.0, 198.0),
	(-400.0, 390.0, -205.0, 196.0),
	(-100.0, 385.0, -200.0, 192.0),
	(200.0, 360.0, -195.0, 175.0),
	(380.0, 320.0, -190.0, 160.0),  # join — stay wide
	(520.0, 280.0, -188.0, 145.0),
	(650.0, 240.0, -185.0, 125.0),  # keep nose wide for windshield
	(760.0, 200.0, -180.0, 105.0),
	(800.0, 110.0, -175.0, 65.0),
]

# Cabin cavity (walkable room). Wide into the canopy threshold.
CAVITY_STATIONS = [
	(BACK_X + 30.0, 340.0, FLOOR_TOP_Z - 2.0, CEILING_Z - 8.0),
	(-400.0, 340.0, FLOOR_TOP_Z - 2.0, CEILING_Z - 10.0),
	(-50.0, 335.0, FLOOR_TOP_Z - 2.0, CEILING_Z - 14.0),
	(200.0, 320.0, FLOOR_TOP_Z - 2.0, 170.0),
	(380.0, 295.0, FLOOR_TOP_Z - 2.0, 150.0),
	(520.0, 250.0, FLOOR_TOP_Z, 130.0),
	(650.0, 200.0, FLOOR_TOP_Z + 2.0, 110.0),
	(740.0, 150.0, FLOOR_TOP_Z + 4.0, 85.0),
]


def _densify(stations, steps=4):
	out = []
	for i in range(len(stations) - 1):
		a, b = stations[i], stations[i + 1]
		for k in range(steps):
			t = _smooth(k / steps)
			out.append(tuple(_lerp(a[j], b[j], t) for j in range(4)))
	out.append(stations[-1])
	return out


def _ring8(x, hy, zb, zt, soft=False):
	"""Chamfered rectangle ring in YZ at X. soft=True → small chamfer (walkable portals)."""
	if soft:
		c = min(hy * 0.06, (zt - zb) * 0.06, 18.0)
	else:
		c = min(hy * 0.28, (zt - zb) * 0.22, 55.0)
	return [
		(x, -hy + c, zb),
		(x, -hy, zb + c),
		(x, -hy, zt - c),
		(x, -hy + c, zt),
		(x, hy - c, zt),
		(x, hy, zt - c),
		(x, hy, zb + c),
		(x, hy - c, zb),
	]


def _loft_solid(name, stations, tip_x=None, pointed_tip=True, open_forward=False):
	"""Closed solid loft from stations (filled volume for boolean)."""
	rings = [_ring8(*st) for st in stations]
	n = len(rings[0])
	nr = len(rings)
	verts = []
	for ring in rings:
		verts.extend(ring)
	faces = []

	def vi(ri, i):
		return ri * n + i

	for ri in range(nr - 1):
		for i in range(n):
			j = (i + 1) % n
			faces.append((vi(ri, i), vi(ri, j), vi(ri + 1, j), vi(ri + 1, i)))

	# Aft cap
	aft = [vi(0, i) for i in range(n)]
	faces.append(tuple(aft))

	# Forward end
	if pointed_tip:
		if tip_x is None:
			tip_x = stations[-1][0] + 20.0
		zb, zt = stations[-1][2], stations[-1][3]
		tip = len(verts)
		verts.append((tip_x, 0.0, (zb + zt) * 0.5))
		for i in range(n):
			j = (i + 1) % n
			faces.append((vi(nr - 1, i), vi(nr - 1, j), tip))
	elif not open_forward:
		# Flat bulkhead at last ring
		fwd = [vi(nr - 1, i) for i in range(n)]
		faces.append(tuple(reversed(fwd)))
	# open_forward=True → no front face (walkable into canopy; complex collision stays open)

	return _mesh(name, verts, faces)


def _roof_z_at(x: float) -> float:
	"""Outer roof Z on the hull centerline at X (matches OUTER_STATIONS)."""
	sts = OUTER_STATIONS
	if x <= sts[0][0]:
		return sts[0][3]
	if x >= sts[-1][0]:
		return sts[-1][3]
	for i in range(len(sts) - 1):
		a, b = sts[i], sts[i + 1]
		if a[0] <= x <= b[0]:
			t = (x - a[0]) / (b[0] - a[0])
			t = _smooth(t)
			return _lerp(a[3], b[3], t)
	return CEILING_Z


def _roof_pitch_deg(x: float) -> float:
	"""Roof pitch (degrees around Y) so panes follow the sloping hull.

	Positive pitch tilts the forward (+X) edge downward to match HopeTech bow slope.
	"""
	dx = 60.0
	# Roof drops toward the nose (+X) → negative dZ/dX → Blender Y-rot that seats the pane
	return math.degrees(math.atan2(_roof_z_at(x - dx) - _roof_z_at(x + dx), 2.0 * dx))


def _sample_shell_roof(shell, x: float, y: float = 0.0):
	"""Raycast shell from above → world hit + roof pitch (deg) matching surface normal."""
	from mathutils import Vector

	mw = shell.matrix_world
	mwi = mw.inverted()
	origin_w = Vector((x, y, 600.0))
	dir_w = Vector((0.0, 0.0, -1.0))
	ok, loc_l, n_l, _idx = shell.ray_cast(mwi @ origin_w, (mwi.to_3x3() @ dir_w).normalized())
	if not ok:
		zt = _roof_z_at(x)
		return (x, y, zt), _roof_pitch_deg(x)
	loc_w = mw @ loc_l
	n_w = (mw.to_3x3() @ n_l).normalized()
	# Horizontal pane (+Z up) → rotate around Y so +Z aligns with roof normal
	pitch = math.degrees(math.atan2(n_w.x, n_w.z))
	return (float(loc_w.x), float(loc_w.y), float(loc_w.z)), pitch


def _half_y_at(x: float) -> float:
	"""Outer half-width Y at X from OUTER_STATIONS."""
	sts = OUTER_STATIONS
	if x <= sts[0][0]:
		return sts[0][1]
	if x >= sts[-1][0]:
		return sts[-1][1]
	for i in range(len(sts) - 1):
		a, b = sts[i], sts[i + 1]
		if a[0] <= x <= b[0]:
			t = _smooth((x - a[0]) / (b[0] - a[0]))
			return _lerp(a[1], b[1], t)
	return HALF_Y


CANOPY_START_X = 380.0
FRAME_THICK = 4.0  # thin frames like МостикОстекление


def _surface_point(x, y_frac, z_frac):
	"""Point on outer hull silhouette. y_frac in [-1..1], z_frac in [0..1].

	Sill (z_frac≈0) sits on FLOOR_TOP_Z so glass/frames seal to the floor — no underhang lip,
	no sky gap under the pane.
	"""
	hy = _half_y_at(x)
	zt = _roof_z_at(x)
	zb = FLOOR_TOP_Z
	return (x, y_frac * (hy - 6.0), zb + z_frac * (zt - zb - 4.0))


def _floor_half_y_at(x: float) -> float:
	"""Walkable floor half-width inside hull (aft of canopy)."""
	return max(50.0, min(335.0, _half_y_at(x) - 12.0))


def _glass_sill_half_y(x: float, nodes=None) -> float:
	"""Fallback sill width from canopy nodes (before raycast targets exist)."""
	N = nodes or _canopy_nodes()
	sill = [N["A_bl"], N["B_bl"], N["C_bl"]]
	# Inside frame half-thick (~3.5) + margin — no exterior lip past glazing
	inset = 10.0
	if x <= sill[0][0]:
		return max(40.0, abs(sill[0][1]) - inset)
	if x >= sill[-1][0]:
		return max(40.0, abs(sill[-1][1]) - inset)
	for i in range(len(sill) - 1):
		a, b = sill[i], sill[i + 1]
		if a[0] <= x <= b[0]:
			t = 0.0 if b[0] <= a[0] else (x - a[0]) / (b[0] - a[0])
			hy = abs(a[1]) * (1.0 - t) + abs(b[1]) * t
			return max(40.0, hy - inset)
	return max(40.0, abs(sill[-1][1]) - inset)


def _canopy_inner_half_y(x: float, z: float = None) -> float:
	"""
	Half-width to INNER face of glass/frame/cheek (ray from centerline outward).
	Floor edge stops here: flush from inside, no white lip outside the glazing.
	"""
	from mathutils import Vector

	if z is None:
		z = FLOOR_TOP_Z + 3.0
	hit_names = (
		"SM_Bridge_Canopy_Glass",
		"SM_Bridge_Canopy_Frame",
		"SM_Bridge_Canopy_SideWalls",
		"SM_Bridge_Canopy_Collar",
	)
	origin = Vector((x, 0.0, z))
	direction = Vector((0.0, -1.0, 0.0))
	best = None
	for name in hit_names:
		obj = bpy.data.objects.get(name)
		if not obj or not getattr(obj, "data", None):
			continue
		mwi = obj.matrix_world.inverted()
		o_l = mwi @ origin
		d_l = (mwi.to_3x3() @ direction).normalized()
		ok, loc, _n, _f = obj.ray_cast(o_l, d_l, distance=400.0)
		if not ok:
			continue
		hit_w = obj.matrix_world @ loc
		dist = abs(hit_w.y)
		if best is None or dist < best:
			best = dist
	if best is None:
		return _glass_sill_half_y(x)
	# Sit 1cm inside inner face — meets glazing, no exterior overhang
	return max(40.0, best - 1.0)


def build_bridge_floor(mat):
	"""
	Tapered floor fitted to canopy INNER faces (call AFTER glass/frames/cheeks).
	Hermetic: meets glazing from inside — no sky gap, no exterior white lip.
	"""
	from mathutils import Vector

	N = _canopy_nodes()
	tip_x = min(N["C_bl"][0], N["C_br"][0]) - 4.0
	glass = bpy.data.objects.get("SM_Bridge_Canopy_Glass")
	if glass:
		mwi = glass.matrix_world.inverted()
		o = Vector((CANOPY_START_X + 50.0, 0.0, FLOOR_TOP_Z + 8.0))
		d = Vector((1.0, 0.0, 0.0))
		ok, loc, _n, _f = glass.ray_cast(
			mwi @ o, (mwi.to_3x3() @ d).normalized(), distance=500.0
		)
		if ok:
			hit = glass.matrix_world @ loc
			tip_x = min(tip_x, hit.x - 2.0)

	xs = [
		BACK_X + 35.0,
		-450.0,
		-150.0,
		100.0,
		250.0,
		320.0,
		CANOPY_START_X - 10.0,
		CANOPY_START_X,
		400.0,
		440.0,
		480.0,
		520.0,
		560.0,
		600.0,
		640.0,
		680.0,
		720.0,
		tip_x,
	]
	z_top = FLOOR_TOP_Z
	z_bot = FLOOR_TOP_Z - FLOOR_THICK
	top = []
	bot = []

	def hy_at(x):
		if x >= CANOPY_START_X - 30.0:
			return _canopy_inner_half_y(x)
		return _floor_half_y_at(x)

	for x in xs:
		hy = hy_at(x)
		top.append((x, -hy, z_top))
		bot.append((x, -hy, z_bot))
	for x in reversed(xs):
		hy = hy_at(x)
		top.append((x, hy, z_top))
		bot.append((x, hy, z_bot))

	n = len(top)
	verts = top + bot
	faces = [tuple(range(n)), tuple(reversed(range(n, 2 * n)))]
	for i in range(n):
		j = (i + 1) % n
		faces.append((i, j, n + j, n + i))

	obj = _mesh("SM_Bridge_Floor", verts, faces)
	xs_v = [v.co.x for v in obj.data.vertices]
	ys_v = [v.co.y for v in obj.data.vertices]
	zs_v = [v.co.z for v in obj.data.vertices]
	center = Vector(
		(
			(min(xs_v) + max(xs_v)) * 0.5,
			(min(ys_v) + max(ys_v)) * 0.5,
			(min(zs_v) + max(zs_v)) * 0.5,
		)
	)
	bm = bmesh.new()
	bm.from_mesh(obj.data)
	bmesh.ops.translate(bm, verts=list(bm.verts), vec=-center)
	bm.to_mesh(obj.data)
	bm.free()
	obj.data.update()
	obj.location = center
	_assign(obj, mat)
	print(
		f"bridge floor @ {tuple(round(c,1) for c in center)} "
		f"tipX={tip_x:.0f} bowHY={hy_at(tip_x):.0f} (fitted to canopy inner faces)"
	)
	return obj


def _canopy_stations(steps=3):
	"""Outer stations from canopy start through tip — same silhouette as hull."""
	raw = [st for st in OUTER_STATIONS if st[0] >= CANOPY_START_X - 1.0]
	hy = _half_y_at(CANOPY_START_X)
	zt = _roof_z_at(CANOPY_START_X)
	start = (CANOPY_START_X, hy, FLOOR_TOP_Z - 8.0, zt)
	sts = [start] + [st for st in raw if st[0] > CANOPY_START_X + 1.0]
	return _densify(sts, steps)


def _loft_skin(name, stations, inset=2.0):
	"""Open skin loft on hull rings — glass follows hull slope exactly."""
	rings = []
	for st in stations:
		x, hy, zb, zt = st
		hy = max(20.0, hy - inset)
		zb = zb + inset * 0.5
		zt = zt - inset * 0.5
		rings.append(_ring8(x, hy, zb, zt))
	n = len(rings[0])
	nr = len(rings)
	verts = []
	for ring in rings:
		verts.extend(ring)
	faces = []

	def vi(ri, i):
		return ri * n + i

	for ri in range(nr - 1):
		for i in range(n):
			j = (i + 1) % n
			faces.append((vi(ri, i), vi(ri, j), vi(ri + 1, j), vi(ri + 1, i)))

	tip_x = stations[-1][0] + 12.0
	zb, zt = stations[-1][2] + inset, stations[-1][3] - inset
	tip = len(verts)
	verts.append((tip_x, 0.0, (zb + zt) * 0.5))
	for i in range(n):
		j = (i + 1) % n
		faces.append((vi(nr - 1, i), tip, vi(nr - 1, j)))

	return _mesh(name, verts, faces)


def _beam_between(name, p0, p1, thick=FRAME_THICK):
	"""Thin frame strut from p0 to p1."""
	from mathutils import Vector, Matrix

	a, b = Vector(p0), Vector(p1)
	mid = (a + b) * 0.5
	direction = b - a
	length = direction.length
	if length < 1.0:
		return None
	direction.normalize()
	bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0))
	obj = bpy.context.active_object
	obj.name = name
	obj.scale = (length, thick, thick)
	_apply_rs(obj)
	x_axis = direction
	up = Vector((0, 0, 1))
	if abs(x_axis.dot(up)) > 0.9:
		up = Vector((0, 1, 0))
	y_axis = up.cross(x_axis).normalized()
	z_axis = x_axis.cross(y_axis).normalized()
	rot = Matrix((x_axis, y_axis, z_axis)).transposed().to_4x4()
	obj.matrix_world = Matrix.Translation(mid) @ rot
	bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
	obj.location = mid
	return obj


def _canopy_join_profile():
	"""
	Walkable hermetic opening shared by shell passage, collar hole, canopy aft.
	half_Y ≥ 280 clear; sill on FLOOR_TOP_Z; soft chamfer via _ring8(..., soft=True).
	"""
	hy = max(280.0, _half_y_at(CANOPY_START_X) - 10.0)
	zt = max(FLOOR_TOP_Z + 270.0, _roof_z_at(CANOPY_START_X) - 8.0)
	zb = FLOOR_TOP_Z
	return hy, zb, zt


def _canopy_nodes():
	"""Symmetric canopy. Wide C windshield; lower side cheeks for protrusions only."""
	xa = CANOPY_START_X - 30.0
	xb = 540.0
	xc = 760.0
	hy, zb, zt = _canopy_join_profile()
	ring = _ring8(xa, hy, zb, zt, soft=True)
	nodes = {
		"A_bl": ring[0],
		"A_ml": ring[1],
		"A_lh": ring[2],
		"A_tl": ring[3],
		"A_tc": (
			xa,
			(ring[3][1] + ring[4][1]) * 0.5,  # centered — L/R symmetry
			(ring[3][2] + ring[4][2]) * 0.5,
		),
		"A_tr": ring[4],
		"A_mr": ring[5],
		"A_rh": ring[6],
		"A_br": ring[7],
	}
	raw = {
		"B_bl": (xb, -0.96, 0.0),  # sill on FLOOR_TOP — hermetic seat
		"B_ml": (xb, -0.94, 0.40),
		"B_cl": (xb, -0.97, 0.62),
		"B_tl": (xb, -0.52, 0.88),
		"B_tc": (xb, 0.0, 0.94),
		"B_tr": (xb, 0.52, 0.88),
		"B_cr": (xb, 0.97, 0.62),
		"B_mr": (xb, 0.94, 0.40),
		"B_br": (xb, 0.96, 0.0),
		"C_bl": (xc, -0.95, 0.0),
		"C_br": (xc, 0.95, 0.0),
		"C_tl": (xc, -0.62, 0.58),
		"C_tr": (xc, 0.62, 0.58),
		"C_tc": (xc, 0.0, 0.58),
	}
	nodes.update({k: _surface_point(*v) for k, v in raw.items()})
	# Aft cheek tops meet upper side glass rail (raised / neat per red mark)
	nodes["A_cl"] = _surface_point(xa, -0.99, 0.88)
	nodes["A_cr"] = _surface_point(xa, 0.99, 0.88)
	return nodes


def _thin_pane(name, corners, thick=2.5):
	"""Planar glass pane from shared world corners."""
	from mathutils import Vector

	pts = [Vector(p) for p in corners]
	c0, c1, c2, c3 = pts
	n = (c1 - c0).cross(c3 - c0)
	if n.length < 1e-6:
		n = Vector((1, 0, 0))
	else:
		n.normalize()
	inset = n * -0.5
	h = n * (thick * 0.5)
	verts = [
		tuple(c0 + h + inset), tuple(c1 + h + inset), tuple(c2 + h + inset), tuple(c3 + h + inset),
		tuple(c0 - h + inset), tuple(c1 - h + inset), tuple(c2 - h + inset), tuple(c3 - h + inset),
	]
	faces = [
		(0, 1, 2, 3), (7, 6, 5, 4),
		(0, 4, 5, 1), (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0),
	]
	return _mesh(name, verts, faces)


def build_canopy_collar(mat_hull):
	"""Opaque flange: outer > hull, hole == canopy aft (hermetic seat)."""
	x0 = CANOPY_START_X - 50.0
	x1 = CANOPY_START_X + 25.0
	hy_o = _half_y_at(CANOPY_START_X) + 16.0
	zt_o = _roof_z_at(CANOPY_START_X) + 16.0
	zb_o = FLOOR_TOP_Z - 24.0
	outer = _loft_solid(
		"collar_outer",
		[(x0, hy_o, zb_o, zt_o), (x1, hy_o, zb_o, zt_o)],
		pointed_tip=False,
	)

	hy_i, zb_i, zt_i = _canopy_join_profile()
	# Hole SMALLER than canopy aft so opaque flange overlaps glass edges (no side sky slit)
	hole = _loft_solid(
		"collar_hole",
		[
			(x0 - 10.0, hy_i - 12.0, zb_i + 6.0, zt_i - 10.0),
			(x1 + 10.0, hy_i - 12.0, zb_i + 6.0, zt_i - 10.0),
		],
		pointed_tip=False,
	)
	_boolean(outer, hole, "DIFFERENCE")
	outer.name = "SM_Bridge_Canopy_Collar"
	_assign(outer, mat_hull)
	_shade_smooth(outer)
	print(f"canopy collar X={x0:.0f}..{x1:.0f} openHY={hy_i:.0f} (walkable)")
	return outer


def build_canopy_glass(mat_glass):
	"""Front + roof + side glass. Lower cheeks are solid protrusions (not full walls)."""
	N = _canopy_nodes()
	pane_ids = [
		("front", ["C_bl", "C_br", "C_tr", "C_tl"]),
		("port_aft", ["A_ml", "B_bl", "B_tl", "A_lh"]),
		("port_fwd", ["B_bl", "C_bl", "C_tl", "B_tl"]),
		("stbd_aft", ["B_br", "A_rh", "A_mr", "B_tr"]),
		("stbd_fwd", ["C_br", "B_br", "B_tr", "C_tr"]),
		("roof_l", ["A_tl", "B_tl", "B_tc", "A_tc"]),
		("roof_r", ["B_tc", "B_tr", "A_tr", "A_tc"]),
		("roof_tip", ["B_tl", "C_tl", "C_tr", "B_tr"]),
		("port_top", ["A_lh", "B_tl", "A_tl", "A_tc"]),
		("stbd_top", ["B_tr", "A_mr", "A_tr", "A_tc"]),
	]
	parts = []
	for name, ids in pane_ids:
		p = _thin_pane(f"pane_{name}", [N[i] for i in ids], thick=2.5)
		_assign(p, mat_glass)
		parts.append(p)
	joined = _join(parts, "SM_Bridge_Canopy_Glass")
	_assign(joined, mat_glass)
	_shade_smooth(joined)
	print(f"canopy glass panes={len(parts)}")
	return joined


def build_canopy_side_walls(mat_hull):
	"""
	Neat symmetric cheek fairings: tall at hull join (~glass rail), slope to nose.
	Lower protrusions only — not full side walls. Exact L/R mirrors.
	"""
	N = _canopy_nodes()
	pane_ids = [
		("port_cheek", ["A_bl", "B_bl", "B_cl", "A_cl"]),
		("stbd_cheek", ["B_br", "A_br", "A_cr", "B_cr"]),
	]
	parts = []
	for name, ids in pane_ids:
		p = _thin_pane(f"protrusion_{name}", [N[i] for i in ids], thick=14.0)
		_assign(p, mat_hull)
		parts.append(p)
	joined = _join(parts, "SM_Bridge_Canopy_SideWalls")
	_assign(joined, mat_hull)
	_shade_smooth(joined)
	print(f"canopy cheeks={len(parts)} (raised diagonal, L/R mirror)")
	return joined


def build_canopy_frames(mat_dark):
	"""Symmetric frames. No diagonal through the windshield (black crossbar removed)."""
	N = _canopy_nodes()
	parts = []
	idx = 0

	def add(a, b, thick):
		nonlocal idx
		beam = _beam_between(f"frame_{idx}", N[a], N[b], thick)
		idx += 1
		if beam:
			_assign(beam, mat_dark)
			parts.append(beam)

	# Windshield outline only — edges of front glass, NOT C_bl→B_tl (that cut across the pane)
	add("C_bl", "C_br", 10.0)  # sill
	add("C_tl", "C_tr", 11.0)  # brow
	add("C_bl", "C_tl", 9.0)  # left glass edge
	add("C_br", "C_tr", 9.0)  # right glass edge

	# Upper rails along roof/side (follow glass seams, stay off windshield face)
	add("C_tl", "B_tl", 12.0)
	add("C_tr", "B_tr", 12.0)
	add("B_tl", "A_tl", 12.0)
	add("B_tr", "A_tr", 12.0)

	# Aft portal
	add("A_bl", "A_ml", 16.0)
	add("A_ml", "A_lh", 16.0)
	add("A_lh", "A_tl", 16.0)
	add("A_tl", "A_tc", 16.0)
	add("A_tc", "A_tr", 16.0)
	add("A_tr", "A_mr", 16.0)
	add("A_mr", "A_rh", 16.0)
	add("A_rh", "A_br", 16.0)
	add("A_br", "A_bl", 16.0)

	# Side sills + roof spine (symmetric)
	add("A_bl", "B_bl", 7.0)
	add("B_bl", "C_bl", 7.0)
	add("A_br", "B_br", 7.0)
	add("B_br", "C_br", 7.0)
	add("A_tc", "B_tc", 8.0)
	add("B_tc", "C_tc", 7.0)
	add("B_tl", "B_tc", 6.0)
	add("B_tc", "B_tr", 6.0)
	add("B_ml", "B_tl", 5.0)
	add("B_mr", "B_tr", 5.0)
	add("B_bl", "B_ml", 5.0)
	add("B_br", "B_mr", 5.0)

	joined = _join(parts, "SM_Bridge_Canopy_Frame")
	_assign(joined, mat_dark)
	_shade_smooth(joined)
	print(f"canopy frames beams={idx} (symmetric, clear windshield)")
	return joined


def verify_hermetic():
	"""Fail loud if sky-gap / narrow nose / floor step remains."""
	from mathutils import Vector

	bpy.context.view_layer.update()

	def world_xs(name):
		o = bpy.data.objects.get(name)
		if not o:
			raise RuntimeError(f"hermetic: missing {name}")
		# Force world matrix (location may lag after FBX export zeroing)
		bpy.context.view_layer.update()
		m = o.matrix_world.copy()
		pts = [m @ v.co for v in o.data.vertices]
		return pts

	hy, zb, zt = _canopy_join_profile()
	shell = world_xs("SM_Bridge_Shell")
	collar = world_xs("SM_Bridge_Canopy_Collar")
	glass = world_xs("SM_Bridge_Canopy_Glass")
	walls = world_xs("SM_Bridge_Canopy_SideWalls")
	frame = world_xs("SM_Bridge_Canopy_Frame")
	floor = world_xs("SM_Bridge_Floor")

	shell_max_x = max(p.x for p in shell)
	collar_min_x = min(p.x for p in collar)
	collar_max_x = max(p.x for p in collar)
	glass_min_x = min(p.x for p in glass)
	walls_min_x = min(p.x for p in walls)
	frame_min_x = min(p.x for p in frame)
	# aft seal = glass OR side walls (sides are opaque now)
	aft_g = [p for p in glass if p.x < glass_min_x + 25.0]
	aft_w = [p for p in walls if p.x < walls_min_x + 25.0]
	aft_f = [p for p in frame if p.x < frame_min_x + 25.0]
	g_hy = max(abs(p.y) for p in aft_g) if aft_g else 0.0
	w_hy = max(abs(p.y) for p in aft_w) if aft_w else 0.0
	seal_hy = max(g_hy, w_hy)
	f_hy = max(abs(p.y) for p in aft_f) if aft_f else 0.0
	floor_max_x = max(p.x for p in floor)
	floor_top = max(p.z for p in floor)
	glass_front_x = max(p.x for p in glass)
	glass_min_z = min(p.z for p in glass)
	# Tip band only — compare floor tip to glass near the same X (not whole canopy)
	floor_tip = [p for p in floor if p.x > floor_max_x - 25.0]
	glass_tip_sill = [
		p
		for p in glass
		if p.z < FLOOR_TOP_Z + 25.0 and p.x > glass_front_x - 100.0
	]
	tip_hy = max(abs(p.y) for p in floor_tip) if floor_tip else 0.0
	sill_hy = max(abs(p.y) for p in glass_tip_sill) if glass_tip_sill else tip_hy
	# Mid-canopy sample (~B station) — catch side lip/gap
	mid_x = 540.0
	floor_mid = [p for p in floor if abs(p.x - mid_x) < 30.0]
	glass_mid = [p for p in glass if abs(p.x - mid_x) < 40.0 and p.z < FLOOR_TOP_Z + 25.0]
	mid_fhy = max(abs(p.y) for p in floor_mid) if floor_mid else None
	mid_ghy = max(abs(p.y) for p in glass_mid) if glass_mid else None

	errs = []
	if hy < 200.0:
		errs.append(f"join halfY {hy:.0f} < 200 (not walkable)")
	if not (collar_min_x < min(glass_min_x, walls_min_x) < collar_max_x):
		errs.append(
			f"canopy aft X not inside collar {collar_min_x:.0f}..{collar_max_x:.0f}"
		)
	if not (collar_min_x < frame_min_x < collar_max_x):
		errs.append(f"frame aft X {frame_min_x:.0f} not inside collar")
	if seal_hy < hy - 5.0:
		errs.append(f"seal aft |Y| {seal_hy:.0f} << join {hy:.0f} (side sky slit)")
	if f_hy < hy - 5.0:
		errs.append(f"frame aft |Y| {f_hy:.0f} << join {hy:.0f}")
	if floor_max_x < CANOPY_START_X + 150.0:
		errs.append(f"floor tip X {floor_max_x:.0f} does not reach into nose")
	# Hermetic floor↔glass: no exterior lip, no interior gap under sill
	if floor_max_x > glass_front_x - 2.0:
		errs.append(
			f"floor tip X {floor_max_x:.0f} past glass front {glass_front_x:.0f} (lip)"
		)
	if floor_max_x < glass_front_x - 30.0:
		errs.append(
			f"floor tip X {floor_max_x:.0f} short of glass {glass_front_x:.0f} (gap)"
		)
	if tip_hy > sill_hy + 1.0:
		errs.append(f"floor tip |Y| {tip_hy:.0f} past glass sill |Y| {sill_hy:.0f} (lip)")
	if tip_hy < sill_hy - 18.0:
		errs.append(f"floor tip |Y| {tip_hy:.0f} << glass sill |Y| {sill_hy:.0f} (gap)")
	if mid_fhy is not None and mid_ghy is not None:
		if mid_fhy > mid_ghy + 1.0:
			errs.append(f"floor mid |Y| {mid_fhy:.0f} past glass |Y| {mid_ghy:.0f} (lip)")
		if mid_fhy < mid_ghy - 18.0:
			errs.append(f"floor mid |Y| {mid_fhy:.0f} << glass |Y| {mid_ghy:.0f} (gap)")
	if glass_min_z > FLOOR_TOP_Z + 3.0:
		errs.append(
			f"glass sill Z {glass_min_z:.1f} above FLOOR_TOP {FLOOR_TOP_Z} (underhang gap)"
		)
	if abs(floor_top - FLOOR_TOP_Z) > 2.0:
		errs.append(f"floor top Z {floor_top:.1f} != FLOOR_TOP {FLOOR_TOP_Z}")
	if abs(zb - FLOOR_TOP_Z) > 1.0:
		errs.append(f"join sill Z {zb:.1f} != FLOOR_TOP")
	if shell_max_x < CANOPY_START_X - 1.0:
		errs.append(f"shell maxX {shell_max_x:.0f} short of canopy start")
	if shell_max_x > CANOPY_START_X + 40.0:
		errs.append(
			f"shell maxX {shell_max_x:.0f} past canopy (floor_cut debris / nose slab)"
		)

	if errs:
		for e in errs:
			print("HERMETIC FAIL:", e)
		raise RuntimeError("hermetic checks failed: " + "; ".join(errs))
	print(
		f"HERMETIC OK joinHY={hy:.0f} sealAftY={seal_hy:.0f} frameAftY={f_hy:.0f} "
		f"collarX={collar_min_x:.0f}..{collar_max_x:.0f} "
		f"floorTip={floor_max_x:.0f}<glass={glass_front_x:.0f} "
		f"tipHY={tip_hy:.0f}~sill={sill_hy:.0f} glassZ={glass_min_z:.1f}"
	)


def build_unified_shell(mat_hull, mat_dark):
	"""
	Aft solid hull ending at canopy start — nose is frames + glass (no gap, no double skin).
	"""
	# Hull stations only up to canopy; seal the forward face at CANOPY_START_X
	aft_sts = [st for st in OUTER_STATIONS if st[0] < CANOPY_START_X - 1.0]
	hy = _half_y_at(CANOPY_START_X)
	zt = _roof_z_at(CANOPY_START_X)
	aft_sts.append((CANOPY_START_X, hy, FLOOR_TOP_Z - 8.0, zt))
	aft_sts = _densify(aft_sts, 3)

	# Open forward end — no bulkhead face (complex collision must not wall off the nose)
	outer = _loft_solid("hull_outer", aft_sts, pointed_tip=False, open_forward=True)
	hy_i, zb_i, zt_i = _canopy_join_profile()

	cavity_sts = [st for st in CAVITY_STATIONS if st[0] < CANOPY_START_X]
	cavity_sts.append(
		(
			CANOPY_START_X + 10.0,
			hy_i - 10.0,
			FLOOR_TOP_Z - 2.0,
			min(zt_i - 10.0, CEILING_Z - 10.0),
		)
	)
	# Open cavity through the join so interior is continuous into canopy
	cavity = _loft_solid(
		"cabin_cavity",
		_densify(cavity_sts, 3),
		pointed_tip=False,
		open_forward=True,
	)
	cavity.scale = (1.02, 1.0, 1.0)
	_apply_rs(cavity)
	_boolean(outer, cavity, "DIFFERENCE")

	door = _box(
		"door_cut",
		(80.0, 230.0, 270.0),
		(BACK_X + 20.0, 0.0, FLOOR_TOP_Z + 130.0),
	)
	_boolean(outer, door, "DIFFERENCE")

	# Open shell belly only under the AFT cabin so the separate floor mesh seats cleanly.
	# NEVER extend this cutter into the canopy — a long box left a solid end-cap at X≈+750
	# (Y±350) that looked like a white floor lip past the glass.
	cut_x0 = BACK_X + 40.0
	cut_x1 = CANOPY_START_X - 5.0
	cut_len = cut_x1 - cut_x0
	floor_cut = _box(
		"floor_cut",
		(cut_len, 640.0, 50.0),
		((cut_x0 + cut_x1) * 0.5, 0.0, FLOOR_TOP_Z - 20.0),
	)
	_boolean(outer, floor_cut, "DIFFERENCE")

	joined = outer
	joined.name = "SM_Bridge_Shell"
	_assign(joined, mat_hull)

	# Strip any boolean debris past the canopy join (hard fail if a tip-cap remains)
	bm = bmesh.new()
	bm.from_mesh(joined.data)
	bm.verts.ensure_lookup_table()
	# Still in world loft space (centering happens later in _register)
	kill = [v for v in bm.verts if v.co.x > CANOPY_START_X + 25.0]
	if kill:
		print(f"shell cleanup: removed {len(kill)} debris verts past canopy")
		bmesh.ops.delete(bm, geom=kill, context="VERTS")
		bm.to_mesh(joined.data)
	bm.free()
	joined.data.update()

	_bevel(joined, 3.5, 2)
	_shade_smooth(joined)

	bpy.context.view_layer.objects.active = joined
	joined.select_set(True)
	bpy.ops.object.mode_set(mode="EDIT")
	bpy.ops.mesh.select_all(action="SELECT")
	bpy.ops.mesh.remove_doubles(threshold=0.05)
	bpy.ops.mesh.normals_make_consistent(inside=False)
	bpy.ops.object.mode_set(mode="OBJECT")
	joined.select_set(False)
	return joined


def build_portholes(shell, mat_metal, mat_dark, mat_glass):
	"""
	Proper circular portholes through port/starboard walls (replaces thruster stubs).
	Cuts holes in shell; returns (rim_mesh, glass_mesh).
	"""
	# (x, z) — eye-height pairs along the cabin
	stations = [(-200.0, -5.0), (60.0, -5.0)]
	r_glass = 40.0
	r_inner = 44.0
	r_outer = 52.0
	rims = []
	glasses = []

	for xi, (x, z) in enumerate(stations):
		hy_o = _half_y_at(x)
		hy_i = max(220.0, hy_o - 52.0)
		for y_sign in (-1.0, 1.0):
			tag = f"{xi}_{'L' if y_sign < 0 else 'R'}"
			y_out = y_sign * (hy_o - 1.0)
			y_in = y_sign * (hy_i + 1.0)
			y_mid = 0.5 * (y_out + y_in)

			# Bore through hull wall
			cut = _cyl(
				f"ph_cut_{tag}",
				r_glass + 2.0,
				abs(y_out - y_in) + 40.0,
				(x, y_mid, z),
				rot=(math.radians(90.0), 0.0, 0.0),
				verts=32,
			)
			_boolean(shell, cut, "DIFFERENCE")

			# Exterior flange (HopeTech-style ring)
			rim_o = _cyl(
				f"ph_rim_o_{tag}",
				r_outer,
				8.0,
				(x, y_out - y_sign * 3.0, z),
				rot=(math.radians(90.0), 0.0, 0.0),
				verts=32,
			)
			# Punch glass hole in outer flange
			hole_o = _cyl(
				f"ph_hole_o_{tag}",
				r_glass,
				20.0,
				(x, y_out - y_sign * 3.0, z),
				rot=(math.radians(90.0), 0.0, 0.0),
				verts=32,
			)
			_boolean(rim_o, hole_o, "DIFFERENCE")
			_assign(rim_o, mat_metal)
			_bevel(rim_o, 1.5, 2)
			_shade_smooth(rim_o)
			rims.append(rim_o)

			# Interior flange (what crew sees — proper иллюминатор)
			rim_i = _cyl(
				f"ph_rim_i_{tag}",
				r_inner,
				7.0,
				(x, y_in + y_sign * 2.5, z),
				rot=(math.radians(90.0), 0.0, 0.0),
				verts=32,
			)
			hole_i = _cyl(
				f"ph_hole_i_{tag}",
				r_glass,
				20.0,
				(x, y_in + y_sign * 2.5, z),
				rot=(math.radians(90.0), 0.0, 0.0),
				verts=32,
			)
			_boolean(rim_i, hole_i, "DIFFERENCE")
			_assign(rim_i, mat_dark)
			_bevel(rim_i, 1.2, 2)
			_shade_smooth(rim_i)
			rims.append(rim_i)

			# Glass disc in the wall throat
			glass = _cyl(
				f"ph_glass_{tag}",
				r_glass - 1.0,
				4.0,
				(x, y_mid, z),
				rot=(math.radians(90.0), 0.0, 0.0),
				verts=32,
			)
			_assign(glass, mat_glass)
			_shade_smooth(glass)
			glasses.append(glass)

	rim_joined = _join(rims, "SM_Bridge_Porthole_Rim")
	_assign(rim_joined, mat_metal)
	glass_joined = _join(glasses, "SM_Bridge_Porthole_Glass")
	_assign(glass_joined, mat_glass)

	# Heal shell after porthole bores
	bpy.context.view_layer.objects.active = shell
	shell.select_set(True)
	bpy.ops.object.mode_set(mode="EDIT")
	bpy.ops.mesh.select_all(action="SELECT")
	bpy.ops.mesh.remove_doubles(threshold=0.05)
	bpy.ops.mesh.normals_make_consistent(inside=False)
	bpy.ops.object.mode_set(mode="OBJECT")
	shell.select_set(False)

	print(f"portholes stations={len(stations)} rims={len(rims)} glass={len(glasses)}")
	return rim_joined, glass_joined


def build_all():
	_clear_scene()
	_ensure_unit_cm()
	PLACEMENTS.clear()

	mat_hull = _mat("M_Bridge_Hull", (0.78, 0.76, 0.72, 1.0), rough=0.55, metallic=0.1)
	mat_dark = _mat("M_Bridge_Dark", (0.12, 0.12, 0.13, 1.0), rough=0.4, metallic=0.3)
	mat_glass = _mat("M_Bridge_Glass", (0.05, 0.07, 0.09, 1.0), rough=0.05, metallic=0.0, alpha=0.4)
	mat_metal = _mat("M_Bridge_Metal", (0.45, 0.46, 0.48, 1.0), rough=0.3, metallic=0.7)

	shell = build_unified_shell(mat_hull, mat_dark)
	porthole_rim, porthole_glass = build_portholes(shell, mat_metal, mat_dark, mat_glass)
	mat_floor = _mat("M_Bridge_Floor", (0.18, 0.19, 0.21, 1.0), rough=0.65, metallic=0.15)
	# Canopy first — floor is raycast-fitted to glass/frame/cheek inner faces
	exports = [(shell, "SM_Bridge_Shell", mat_hull, 0.0)]
	exports.append((porthole_rim, "SM_Bridge_Porthole_Rim", mat_metal, 0.0))
	exports.append((porthole_glass, "SM_Bridge_Porthole_Glass", mat_glass, 0.0))
	exports.append((build_canopy_collar(mat_hull), "SM_Bridge_Canopy_Collar", mat_hull, 0.0))
	exports.append((build_canopy_side_walls(mat_hull), "SM_Bridge_Canopy_SideWalls", mat_hull, 0.0))
	exports.append((build_canopy_glass(mat_glass), "SM_Bridge_Canopy_Glass", mat_glass, 0.0))
	exports.append((build_canopy_frames(mat_dark), "SM_Bridge_Canopy_Frame", mat_dark, 0.0))
	exports.append((build_bridge_floor(mat_floor), "SM_Bridge_Floor", mat_floor, 0.0))

	for name, loc in [
		("REF_Entry", (BACK_X + 150.0, 0.0, FLOOR_TOP_Z + 96.0)),
		("REF_Mid", (100.0, 0.0, FLOOR_TOP_Z + 96.0)),
		("REF_Bow", (500.0, 0.0, FLOOR_TOP_Z + 96.0)),
	]:
		bpy.ops.mesh.primitive_cylinder_add(radius=42.0, depth=192.0, location=loc)
		bpy.context.active_object.name = name

	for obj, name, mat, pitch in exports:
		_register(obj, name, mat, pitch_deg=pitch)

	verify_hermetic()

	existing = {}
	if PLACEMENTS_PATH.is_file():
		existing = json.loads(PLACEMENTS_PATH.read_text(encoding="utf-8"))
	for k in list(existing.keys()):
		if k.startswith("SM_Bridge_"):
			existing.pop(k, None)
	existing.update(PLACEMENTS)
	PLACEMENTS_PATH.write_text(json.dumps(existing, indent=2, sort_keys=True) + "\n", encoding="utf-8")

	bpy.ops.wm.save_as_mainfile(filepath=str(EXPORT_DIR / "TrainingShip_Modules.blend"))
	print(f"DONE parts={len(PLACEMENTS)} BACK_X={BACK_X} FLOOR_TOP={FLOOR_TOP_Z}")
	print("placements", PLACEMENTS)


if __name__ == "__main__":
	build_all()
