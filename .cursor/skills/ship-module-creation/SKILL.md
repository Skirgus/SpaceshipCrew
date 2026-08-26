---
name: ship-module-creation
description: >-
  Creates and authors SpaceshipCrew ship modules (UShipModuleDefinition +
  UShipModuleVisualOverride), Blender/FBX panel kits, PBR/ISM materials, and
  dock-aware wall openings. Use when adding a corridor/room module, VisualOverride
  VisualParts, WallSocketName/OpeningMesh, importing CorridorPanels, or fixing
  docking panel orientation in the ship builder.
  Also activate on Russian requests such as: создай модуль, сделать модуль,
  новый модуль, модуль коридора, VisualOverride, проёмы при стыковке,
  подстановка проёмов, панели коридора, импорт панелей, ShipModule.
---

# Ship module creation (SpaceshipCrew)

Grid: **400 × 400 × 300 cm** per cell. Module center = `(0,0,0)`. Faces: Front=+X, Back=−X, Left=−Y, Right=+Y.

## Assets (always pair)

| Asset | Path |
|-------|------|
| Definition | `/Game/Data/ShipModules/{Id}` |
| VisualOverride | `/Game/Data/ShipModules/{Id}_VisualOverride` |

Definition: `ModuleId`, `ModuleType`, `CellSize`, `bHasInterior=true` (for walkable rooms), soft ref → VisualOverride.

Docs (read only if needed): `docs/SHIP_BUILDER_GRID_RU.md`, `docs/SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md`, `docs/VISUAL_OVERRIDE_DOCK_OPENINGS_RU.md`.

## Workflow checklist

```
- [ ] Meshes in Blender (or BigModular) sized to cell; export FBX
- [ ] Import UE; verify bounds ~cm (not 100× off)
- [ ] Materials: used_with_instanced_static_meshes=true (preview uses ISM)
- [ ] Create/update Definition + VisualOverride VisualParts
- [ ] Tag dock walls: WallSocketName + OpeningMesh (same local axes)
- [ ] Apply via UnrealEditor-Cmd with Editor CLOSED (or Output Log `py`)
- [ ] PIE: alone = solid walls; docked = OpeningMesh on shared sockets
```

## Mesh orientations (critical)

LR panels (Left/Right): **thin in Y**, size ~400×20×300.  
FB panels (Front/Back): **thin in X**, size ~20×400×300.

| Face | Closed Mesh | OpeningMesh | Yaw |
|------|-------------|-------------|-----|
| Left | `*_Solid` (LR) | `*_Door` (LR) | 0 |
| Right | `*_Solid` (LR) | `*_Door` (LR) | 180 |
| Front | `*_Solid_FB` | `*_Door_FB` | 180 |
| Back | `*_Solid_FB` | `*_Door_FB` | 0 |

**Never** put LR mesh on Front/Back with yaw ±90 — UE Python Transform often stores yaw as pitch; panel stands across the corridor. OpeningMesh uses the **same** `RelativeTransform` as closed Mesh unless `bUseOpeningRelativeTransform` is set.

Floor/ceiling: no `WallSocketName`. Typical Z offsets ~−142 / +141 for 300-high cell.

Panel sockets: `Left_X0_Y0_Z0`, `Right_…`, `Front_…`, `Back_…` (1×1×1).

## Authoring Python

Prefer extending `Content/Python/author_corridor_wall_slots.py` over rewriting.

Run with **no** `UnrealEditor.exe` holding the project:

```powershell
# Kill leftovers if Cmd fails with Error 32 / Error saving
Get-Process UnrealEditor*,UnrealEditor-Cmd* -ErrorAction SilentlyContinue | Stop-Process -Force

& "Z:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "H:\Projects\SpaceshipCrew\SpaceshipCrew.uproject" `
  "-ExecutePythonScript=H:\Projects\SpaceshipCrew\Content\Python\author_corridor_wall_slots.py" `
  -unattended -nop4 -nosplash -stdout
```

In a running Editor Output Log: `py "H:/Projects/SpaceshipCrew/Content/Python/author_corridor_wall_slots.py"`.

After author, log must show `yaw` 0 or ±180 only (pitch≈0) and matching `opening=` mesh family.

## FBX import scale

Kit FBX from this project often needs `import_uniform_scale=0.01`.  
Newly exported Blender FBX (unit-scaled) may need `1.0`. **Always compare** `get_bounds().box_extent*2` to a known-good mesh (e.g. Door_FB ≈ 18×400×300). Helper: `Content/Python/fix_solid_fb_scale.py`.

## C++ preview behavior

`AShipBuilderModulePreviewActor`: if VisualOverride has VisualParts → draw parts; open `WallSocketName` → `OpeningMesh` (empty = hole). Needs compiled module reflection for new UPROPERTY fields (`ShipModuleWallOpeningKind`, etc.). Live Coding blocks external `Build.bat` — close Editor or Ctrl+Alt+F11.

## Do not

- Leave permanent door meshes in VisualParts without `WallSocketName` (doors always open).
- Use Unreal-MCP for asset create (only AIAssistant / BT toolsets).
- Rely on Remote Python unless user enabled it and nodes discover.
- Edit `.uasset` by hand; use Editor Python.

## Reference corridor

Example module: `Corridor_CustomPanels_01` + `_VisualOverride`.  
Meshes: `Content/Meshes/CorridorPanels/` (`SM_Floor_400`, `SM_Ceiling_400`, `SM_Wall_Solid_400x300`, `SM_Wall_Solid_FB_400x300`, `SM_Wall_Door_400x300`, `SM_Wall_Door_FB_400x300`).

More detail: [reference.md](reference.md).
