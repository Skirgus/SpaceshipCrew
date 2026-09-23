"""
Генерация мешей для тренировки инженера в Blender (E0).

Запуск через Blender MCP execute_blender_code (exec файла) или:
  blender --background --python tools/blender/generate_engineer_training_meshes.py

Экспорт FBX в Content/Meshes/EngineerTraining/ (см. UE import_uniform_scale в скилле модулей).
Масштаб: 1 Blender unit = 1 cm (как UE).
"""

from __future__ import annotations

import math
import os
from pathlib import Path

import bpy

# Корень проекта SpaceshipCrew (…/tools/blender/ → parents[2])
PROJECT_ROOT = Path(__file__).resolve().parents[2]
EXPORT_DIR = PROJECT_ROOT / "Content" / "Meshes" / "EngineerTraining"


def _clear_scene() -> None:
	bpy.ops.object.select_all(action="SELECT")
	bpy.ops.object.delete(use_global=False)
	for block in bpy.data.meshes:
		bpy.data.meshes.remove(block)


def _ensure_unit_cm() -> None:
	scene = bpy.context.scene
	scene.unit_settings.system = "METRIC"
	scene.unit_settings.scale_length = 0.01  # 1 BU = 1 cm


def _apply_all(obj: bpy.types.Object) -> None:
	bpy.context.view_layer.objects.active = obj
	obj.select_set(True)
	bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
	obj.select_set(False)


def _box(name: str, size: tuple[float, float, float], location: tuple[float, float, float]) -> bpy.types.Object:
	bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
	obj = bpy.context.active_object
	obj.name = name
	obj.scale = size
	_apply_all(obj)
	return obj


def _join(objects: list[bpy.types.Object], name: str) -> bpy.types.Object:
	bpy.ops.object.select_all(action="DESELECT")
	if len(objects) == 1:
		obj = objects[0]
		obj.name = name
		bpy.context.view_layer.objects.active = obj
		obj.select_set(True)
		bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
		obj.location = (0.0, 0.0, 0.0)
		_apply_all(obj)
		return obj

	for obj in objects:
		obj.select_set(True)
	bpy.context.view_layer.objects.active = objects[0]
	bpy.ops.object.join()
	joined = bpy.context.active_object
	joined.name = name
	bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
	joined.location = (0.0, 0.0, 0.0)
	_apply_all(joined)
	return joined


def _export_fbx(obj: bpy.types.Object, filename: str) -> Path:
	EXPORT_DIR.mkdir(parents=True, exist_ok=True)
	path = EXPORT_DIR / filename
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
	return path


def build_energy_console() -> Path:
	"""Консоль энергосети: корпус + экран + постамент (occupy-friendly)."""
	_clear_scene()
	_ensure_unit_cm()
	parts = [
		_box("EC_Base", (80.0, 60.0, 10.0), (0.0, 0.0, 5.0)),
		_box("EC_Body", (70.0, 40.0, 90.0), (0.0, 10.0, 55.0)),
		_box("EC_Screen", (50.0, 4.0, 35.0), (0.0, -12.0, 70.0)),
		_box("EC_Panel", (55.0, 25.0, 4.0), (0.0, -5.0, 40.0)),
	]
	joined = _join(parts, "SM_EnergyConsole")
	# Поднять так, чтобы низ стоял на Z=0
	min_z = min((v.co.z for v in joined.data.vertices), default=0.0)
	joined.location.z -= min_z
	_apply_all(joined)
	return _export_fbx(joined, "SM_EnergyConsole.fbx")


def build_repair_torch() -> Path:
	"""Ручная горелка: рукоять + сопло."""
	_clear_scene()
	_ensure_unit_cm()
	handle = _box("Torch_Handle", (4.0, 4.0, 28.0), (0.0, 0.0, 14.0))
	nozzle = _box("Torch_Nozzle", (6.0, 6.0, 10.0), (0.0, 0.0, 33.0))
	tip = _box("Torch_Tip", (2.0, 2.0, 8.0), (0.0, 0.0, 42.0))
	joined = _join([handle, nozzle, tip], "SM_RepairTorch")
	min_z = min((v.co.z for v in joined.data.vertices), default=0.0)
	joined.location.z -= min_z
	_apply_all(joined)
	return _export_fbx(joined, "SM_RepairTorch.fbx")


def build_panel(name: str, cracked: bool) -> Path:
	"""Настенная панель 120×8×180; damaged = выдавленный «пробой»."""
	_clear_scene()
	_ensure_unit_cm()
	parts = [_box("Panel_Main", (120.0, 8.0, 180.0), (0.0, 0.0, 90.0))]
	if cracked:
		parts.append(_box("Panel_Scar", (40.0, 12.0, 50.0), (10.0, -2.0, 100.0)))
		parts.append(_box("Panel_Scar2", (25.0, 14.0, 30.0), (-20.0, -3.0, 70.0)))
	joined = _join(parts, name)
	min_z = min((v.co.z for v in joined.data.vertices), default=0.0)
	joined.location.z -= min_z
	_apply_all(joined)
	return _export_fbx(joined, f"{name}.fbx")


def build_tech_bay_shell() -> Path:
	"""
	Упрощённый tech-bay 1×1×1 (600×600×400): пол + потолок + 3 стены (Front открыт для стыка).
	Для тренировочного уровня; стыковка с CorridorPanels через Front.
	"""
	_clear_scene()
	_ensure_unit_cm()
	# Пол / потолок (толщина ~16)
	floor = _box("TB_Floor", (800.0, 800.0, 16.0), (0.0, 0.0, -192.0))
	ceil = _box("TB_Ceil", (800.0, 800.0, 16.0), (0.0, 0.0, 191.0))
	left = _box("TB_Left", (800.0, 20.0, 400.0), (0.0, -390.0, 0.0))
	right = _box("TB_Right", (800.0, 20.0, 400.0), (0.0, 390.0, 0.0))
	back = _box("TB_Back", (20.0, 800.0, 400.0), (-391.0, 0.0, 0.0))
	joined = _join([floor, ceil, left, right, back], "SM_TechBay_Interior_1x1x1")
	joined.location = (0.0, 0.0, 0.0)
	_apply_all(joined)
	return _export_fbx(joined, "SM_TechBay_Interior_1x1x1.fbx")


def main() -> None:
	paths = [
		build_energy_console(),
		build_repair_torch(),
		build_panel("SM_HullPanel_Intact", cracked=False),
		build_panel("SM_HullPanel_Damaged", cracked=True),
		build_tech_bay_shell(),
	]
	print("Exported:")
	for p in paths:
		print(f"  {p}")


if __name__ == "__main__":
	main()
