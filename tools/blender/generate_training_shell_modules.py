"""
Generate Airlock + CargoHold shell meshes for TrainingShip at 600×600×400 grid.

  blender --background --python tools/blender/generate_training_shell_modules.py

Furniture (crates) stay human-scale; shell walls/floors match PanelUnit.
Writes/merges Content/Meshes/TrainingShip/part_placements.json
"""

from __future__ import annotations

import json
from pathlib import Path

import bpy

PROJECT_ROOT = Path(__file__).resolve().parents[2]
EXPORT_DIR = PROJECT_ROOT / "Content" / "Meshes" / "TrainingShip"
PLACEMENTS_PATH = EXPORT_DIR / "part_placements.json"

PANEL_XY = 800.0
PANEL_Z = 400.0
FLOOR_THICK = 16.0
WALL_THICK = 20.0
HALF_XY = PANEL_XY * 0.5
HALF_Z = PANEL_Z * 0.5
FLOOR_HALF = FLOOR_THICK * 0.5
FLOOR_REL_Z = -(HALF_Z - FLOOR_HALF)  # -192
CEILING_REL_Z = HALF_Z - 9.0  # 191
WALL_OFF = HALF_XY - 10.0  # 390
FACE_OFF = HALF_XY - 9.0  # 391
# Front split walls: leave ~280 cm hatch, grow side stubs with cell
FRONT_STUB_W = 200.0
FRONT_STUB_Y = 300.0  # center of each stub

PLACEMENTS: dict[str, list[float]] = {}


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


def _box_at(name: str, size: tuple[float, float, float], loc: tuple[float, float, float]) -> bpy.types.Object:
	bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
	obj = bpy.context.active_object
	obj.name = name
	obj.scale = size
	_apply(obj)
	return obj


def _center_origin(obj: bpy.types.Object) -> tuple[float, float, float]:
	"""Set origin to bounds center; return world location to use as RelativeTransform."""
	bpy.ops.object.select_all(action="DESELECT")
	obj.select_set(True)
	bpy.context.view_layer.objects.active = obj
	bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
	loc = (float(obj.location.x), float(obj.location.y), float(obj.location.z))
	return loc


def _export_local(obj: bpy.types.Object, filename: str) -> tuple[Path, tuple[float, float, float]]:
	EXPORT_DIR.mkdir(parents=True, exist_ok=True)
	placement = _center_origin(obj)
	path = EXPORT_DIR / filename
	saved = tuple(obj.location)
	obj.location = (0.0, 0.0, 0.0)
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
	PLACEMENTS[obj.name] = list(placement)
	print(f"Exported {filename} placement={placement}")
	return path, placement


def build_airlock() -> None:
	"""1×1×1 airlock shell — Front open (two half-walls), Back dockable."""
	_clear_scene()
	_ensure_unit_cm()
	parts = [
		_box_at("SM_Airlock_Floor", (PANEL_XY, PANEL_XY, FLOOR_THICK), (0.0, 0.0, FLOOR_REL_Z)),
		_box_at("SM_Airlock_Ceiling", (PANEL_XY, PANEL_XY, FLOOR_THICK), (0.0, 0.0, CEILING_REL_Z)),
		_box_at("SM_Airlock_Wall_Left", (PANEL_XY, WALL_THICK, PANEL_Z), (0.0, -WALL_OFF, 0.0)),
		_box_at("SM_Airlock_Wall_Right", (PANEL_XY, WALL_THICK, PANEL_Z), (0.0, WALL_OFF, 0.0)),
		_box_at("SM_Airlock_Wall_Back", (WALL_THICK, PANEL_XY, PANEL_Z), (-FACE_OFF, 0.0, 0.0)),
		_box_at("SM_Airlock_Wall_Front_L", (WALL_THICK, FRONT_STUB_W, PANEL_Z), (FACE_OFF, -FRONT_STUB_Y, 0.0)),
		_box_at("SM_Airlock_Wall_Front_R", (WALL_THICK, FRONT_STUB_W, PANEL_Z), (FACE_OFF, FRONT_STUB_Y, 0.0)),
		# Hatch ring (human-scale visual)
		_box_at("SM_Airlock_DoorRing", (12.0, 260.0, 280.0), (120.0, 0.0, -10.0)),
	]
	for obj in parts:
		_export_local(obj, f"{obj.name}.fbx")


def build_cargo() -> None:
	"""2×1×1 cargo — long on X, one cell wide on Y."""
	_clear_scene()
	_ensure_unit_cm()
	cx, cy = 2, 1
	len_x = cx * PANEL_XY
	len_y = cy * PANEL_XY
	half_x = len_x * 0.5
	wall_y = cy * HALF_XY - 10.0
	face_x = cx * HALF_XY - 9.0

	parts = [
		_box_at("SM_Cargo_Floor", (len_x, len_y, FLOOR_THICK), (0.0, 0.0, FLOOR_REL_Z)),
		_box_at("SM_Cargo_Ceiling", (len_x, len_y, FLOOR_THICK), (0.0, 0.0, CEILING_REL_Z)),
		_box_at("SM_Cargo_Wall_Left", (len_x, WALL_THICK, PANEL_Z), (0.0, -wall_y, 0.0)),
		_box_at("SM_Cargo_Wall_Right", (len_x, WALL_THICK, PANEL_Z), (0.0, wall_y, 0.0)),
		_box_at("SM_Cargo_Wall_Back", (WALL_THICK, len_y, PANEL_Z), (-face_x, 0.0, 0.0)),
		_box_at("SM_Cargo_Wall_Front_L", (WALL_THICK, FRONT_STUB_W, PANEL_Z), (face_x, -FRONT_STUB_Y, 0.0)),
		_box_at("SM_Cargo_Wall_Front_R", (WALL_THICK, FRONT_STUB_W, PANEL_Z), (face_x, FRONT_STUB_Y, 0.0)),
	]

	for obj in parts:
		_export_local(obj, f"{obj.name}.fbx")


def _merge_placements() -> None:
	existing: dict = {}
	if PLACEMENTS_PATH.is_file():
		with open(PLACEMENTS_PATH, "r", encoding="utf-8") as f:
			existing = json.load(f)
	existing.update(PLACEMENTS)
	with open(PLACEMENTS_PATH, "w", encoding="utf-8") as f:
		json.dump(existing, f, indent=2)
	print(f"Wrote {PLACEMENTS_PATH} ({len(existing)} entries)")


def main() -> None:
	build_airlock()
	build_cargo()
	_merge_placements()
	print("Training shell modules done.")


if __name__ == "__main__":
	main()
