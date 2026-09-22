# Shared ship-builder panel grid (cm). Keep in sync with ShipBuilderGridConstants.h
PANEL_XY = 800.0
PANEL_Z = 400.0
FLOOR_THICK = 16.0
WALL_THICK = 20.0
WALL_INSET = 10.0  # panel face inset from cell edge

HALF_XY = PANEL_XY * 0.5
HALF_Z = PANEL_Z * 0.5
FLOOR_HALF = FLOOR_THICK * 0.5
# Walkable top ≈ FloorTopLocalZ in SpaceshipCrewTrainingGameMode
FLOOR_REL_Z = -(HALF_Z - FLOOR_HALF)  # -192
FLOOR_TOP_Z = FLOOR_REL_Z + FLOOR_HALF  # -184
CEILING_REL_Z = HALF_Z - 9.0  # 191
WALL_OFFSET = HALF_XY - WALL_INSET  # 390
FACE_OFFSET = HALF_XY - 9.0  # 391


def wall_offset_for_cells(cells_along_axis: int) -> float:
	return float(cells_along_axis) * HALF_XY - WALL_INSET


def face_offset_for_cells(cells_along_axis: int) -> float:
	return float(cells_along_axis) * HALF_XY - 9.0
