"""
Generate CorridorPanels kit at current ship-builder grid (600×600×400 cm).

  blender --background --python tools/blender/generate_corridor_panels.py

Asset names keep legacy `_400` / `_400x300` suffixes for stable UE paths.
1 Blender unit = 1 cm.
"""

from __future__ import annotations

from pathlib import Path

import bpy

PROJECT_ROOT = Path(__file__).resolve().parents[2]
EXPORT_DIR = PROJECT_ROOT / "Content" / "Meshes" / "CorridorPanels"

PANEL_XY = 800.0
PANEL_Z = 400.0
FLOOR_THICK = 16.0
WALL_THICK = 20.0
# Passage cutout — Starfield-like hatch, fits UE capsule (~192 cm tall)
DOOR_W = 240.0
DOOR_H = 260.0
DOOR_BOTTOM = -PANEL_Z * 0.5 + FLOOR_THICK  # roughly floor top inside wall


def _clear_scene() -> None:
	bpy.ops.object.select_all(action="SELECT")
	bpy.ops.object.delete(use_global=False)
	for block in list(bpy.data.meshes):
		bpy.data.meshes.remove(block)


def _ensure_unit_cm() -> None:
	s = bpy.context.scene
	s.unit_settings.system = "METRIC"
	s.unit_settings.scale_length = 0.01


def _apply(obj: bpy.types.Object) -> None:
	bpy.context.view_layer.objects.active = obj
	obj.select_set(True)
	bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
	obj.select_set(False)


def _box(name: str, size: tuple[float, float, float]) -> bpy.types.Object:
	bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 0.0))
	obj = bpy.context.active_object
	obj.name = name
	obj.scale = size
	_apply(obj)
	bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
	obj.location = (0.0, 0.0, 0.0)
	_apply(obj)
	return obj


def _wall_with_door(name: str, extent: tuple[float, float, float], thin_axis: str) -> bpy.types.Object:
	"""Solid panel with rectangular passage — built as a frame (no boolean / NaN bounds)."""
	door_bottom = -PANEL_Z * 0.5 + FLOOR_THICK
	zc = door_bottom + DOOR_H * 0.5
	top_z = door_bottom + DOOR_H
	bot_h = max(8.0, door_bottom - (-PANEL_Z * 0.5))
	top_h = PANEL_Z * 0.5 - top_z
	side_w = (PANEL_XY - DOOR_W) * 0.5
	parts: list[bpy.types.Object] = []

	def add_box(n: str, size: tuple[float, float, float], loc: tuple[float, float, float]) -> None:
		bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
		obj = bpy.context.active_object
		obj.name = n
		obj.scale = size
		_apply(obj)
		parts.append(obj)

	if thin_axis == "Y":
		add_box("bot", (PANEL_XY, WALL_THICK, bot_h), (0.0, 0.0, -PANEL_Z * 0.5 + bot_h * 0.5))
		add_box("top", (PANEL_XY, WALL_THICK, top_h), (0.0, 0.0, top_z + top_h * 0.5))
		add_box("L", (side_w, WALL_THICK, DOOR_H), (-DOOR_W * 0.5 - side_w * 0.5, 0.0, zc))
		add_box("R", (side_w, WALL_THICK, DOOR_H), (DOOR_W * 0.5 + side_w * 0.5, 0.0, zc))
	else:
		add_box("bot", (WALL_THICK, PANEL_XY, bot_h), (0.0, 0.0, -PANEL_Z * 0.5 + bot_h * 0.5))
		add_box("top", (WALL_THICK, PANEL_XY, top_h), (0.0, 0.0, top_z + top_h * 0.5))
		add_box("L", (WALL_THICK, side_w, DOOR_H), (0.0, -DOOR_W * 0.5 - side_w * 0.5, zc))
		add_box("R", (WALL_THICK, side_w, DOOR_H), (0.0, DOOR_W * 0.5 + side_w * 0.5, zc))

	bpy.ops.object.select_all(action="DESELECT")
	for obj in parts:
		obj.select_set(True)
	bpy.context.view_layer.objects.active = parts[0]
	bpy.ops.object.join()
	joined = bpy.context.active_object
	joined.name = name
	bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
	joined.location = (0.0, 0.0, 0.0)
	_apply(joined)
	return joined


def _export(obj: bpy.types.Object, filename: str) -> Path:
	EXPORT_DIR.mkdir(parents=True, exist_ok=True)
	path = EXPORT_DIR / filename
	bpy.ops.object.select_all(action="DESELECT")
	obj.select_set(True)
	bpy.context.view_layer.objects.active = obj
	saved_loc = tuple(obj.location)
	obj.location = (0.0, 0.0, 0.0)
	bpy.ops.export_scene.fbx(
		filepath=str(path),
		use_selection=True,
		apply_unit_scale=True,
		apply_scale_options="FBX_SCALE_NONE",
		axis_forward="-Y",
		axis_up="Z",
		mesh_smooth_type="FACE",
	)
	obj.location = saved_loc
	print(f"Exported {path}")
	return path


def main() -> None:
	_clear_scene()
	_ensure_unit_cm()
	paths = []

	_clear_scene()
	paths.append(_export(_box("SM_Floor_400", (PANEL_XY, PANEL_XY, FLOOR_THICK)), "SM_Floor_400.fbx"))

	_clear_scene()
	paths.append(_export(_box("SM_Ceiling_400", (PANEL_XY, PANEL_XY, FLOOR_THICK)), "SM_Ceiling_400.fbx"))

	_clear_scene()
	paths.append(
		_export(_box("SM_Wall_Solid_400x300", (PANEL_XY, WALL_THICK, PANEL_Z)), "SM_Wall_Solid_400x300.fbx")
	)

	_clear_scene()
	paths.append(
		_export(_box("SM_Wall_Solid_FB_400x300", (WALL_THICK, PANEL_XY, PANEL_Z)), "SM_Wall_Solid_FB_400x300.fbx")
	)

	_clear_scene()
	paths.append(
		_export(
			_wall_with_door("SM_Wall_Door_400x300", (PANEL_XY, WALL_THICK, PANEL_Z), "Y"),
			"SM_Wall_Door_400x300.fbx",
		)
	)

	_clear_scene()
	paths.append(
		_export(
			_wall_with_door("SM_Wall_Door_FB_400x300", (WALL_THICK, PANEL_XY, PANEL_Z), "X"),
			"SM_Wall_Door_FB_400x300.fbx",
		)
	)

	print("Corridor panels done:")
	for p in paths:
		print(f"  {p}")


if __name__ == "__main__":
	main()
