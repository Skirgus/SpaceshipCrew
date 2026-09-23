"""Генерация простых OBJ для тренировки инженера без запущенного Blender (fallback E0).

UE импортирует OBJ; при появлении Blender MCP предпочтителен
tools/blender/generate_engineer_training_meshes.py → FBX.
"""

from __future__ import annotations

from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "Content" / "Meshes" / "EngineerTraining"


def write_box_obj(path: Path, name: str, sx: float, sy: float, sz: float, z_lift: bool = True) -> None:
	"""Ось-aligned box, центр в XY, низ на Z=0 если z_lift."""
	hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5
	z0 = 0.0 if z_lift else -hz
	z1 = sz if z_lift else hz
	verts = [
		(-hx, -hy, z0),
		(hx, -hy, z0),
		(hx, hy, z0),
		(-hx, hy, z0),
		(-hx, -hy, z1),
		(hx, -hy, z1),
		(hx, hy, z1),
		(-hx, hy, z1),
	]
	faces = [
		(1, 2, 3, 4),
		(5, 8, 7, 6),
		(1, 5, 6, 2),
		(2, 6, 7, 3),
		(3, 7, 8, 4),
		(4, 8, 5, 1),
	]
	path.parent.mkdir(parents=True, exist_ok=True)
	with path.open("w", encoding="utf-8") as f:
		f.write(f"# {name}\n")
		f.write("o {}\n".format(name))
		for v in verts:
			f.write(f"v {v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")
		for face in faces:
			f.write("f {} {} {} {}\n".format(*face))


def write_console(path: Path) -> None:
	"""Грубый составной OBJ: несколько объектов-боксов."""
	path.parent.mkdir(parents=True, exist_ok=True)
	parts = [
		("Base", 80, 60, 10, 0),
		("Body", 70, 40, 90, 10),
		("Screen", 50, 4, 35, 55),
	]
	with path.open("w", encoding="utf-8") as f:
		f.write("# SM_EnergyConsole\no SM_EnergyConsole\n")
		vid = 1
		for name, sx, sy, sz, zoff in parts:
			hx, hy = sx * 0.5, sy * 0.5
			z0, z1 = zoff, zoff + sz
			verts = [
				(-hx, -hy, z0),
				(hx, -hy, z0),
				(hx, hy, z0),
				(-hx, hy, z0),
				(-hx, -hy, z1),
				(hx, -hy, z1),
				(hx, hy, z1),
				(-hx, hy, z1),
			]
			f.write(f"g {name}\n")
			for v in verts:
				f.write(f"v {v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")
			faces = [
				(0, 1, 2, 3),
				(4, 7, 6, 5),
				(0, 4, 5, 1),
				(1, 5, 6, 2),
				(2, 6, 7, 3),
				(3, 7, 4, 0),
			]
			for a, b, c, d in faces:
				f.write(f"f {vid+a} {vid+b} {vid+c} {vid+d}\n")
			vid += 8


def main() -> None:
	OUT.mkdir(parents=True, exist_ok=True)
	write_console(OUT / "SM_EnergyConsole.obj")
	write_box_obj(OUT / "SM_RepairTorch.obj", "SM_RepairTorch", 6, 6, 42)
	write_box_obj(OUT / "SM_HullPanel_Intact.obj", "SM_HullPanel_Intact", 120, 8, 180)
	write_box_obj(OUT / "SM_HullPanel_Damaged.obj", "SM_HullPanel_Damaged", 120, 10, 180)
	write_box_obj(OUT / "SM_TechBay_Interior_1x1x1.obj", "SM_TechBay_Interior_1x1x1", 400, 400, 300, z_lift=False)
	print(f"Wrote OBJ meshes to {OUT}")


if __name__ == "__main__":
	main()
