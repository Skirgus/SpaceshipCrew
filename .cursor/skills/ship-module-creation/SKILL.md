---
name: ship-module-creation
description: >-
  Creates and authors SpaceshipCrew ship modules (UShipModuleDefinition +
  UShipModuleVisualOverride), Blender/FBX panel kits, PBR/ISM materials, and
  dock-aware wall openings. Use when adding a corridor/room/bridge module,
  VisualOverride VisualParts, WallSocketName/OpeningMesh, RelativeTransform
  placement, wrong panel/door position, floating openings, docking misalignment,
  hermetic gaps/sky slits, floor lip past glazing, walk-through walls or blocked
  canopy, portholes, importing CorridorPanels/TrainingShip, or fixing docking
  panel orientation in the ship builder. Also activate on Russian requests such
  as: создай модуль, сделать модуль, новый модуль, модуль коридора, мостик,
  VisualOverride, проёмы при стыковке, подстановка проёмов, панели коридора,
  иллюминатор, коллизия стен, щели, пол торчит, импорт панелей, ShipModule,
  позиция частей, дверь в центре, съехали стены.
---

# Ship module creation (SpaceshipCrew)

Grid: **600 × 600 × 400 cm** per cell. Module center = `(0,0,0)`. Faces: Front=+X, Back=−X, Left=−Y, Right=+Y.

## Hermetic hull (mandatory — no gaps / щелей)

Pressure vessel: **any blue sky / exterior visible through a join = failed module.**  
Rule file: `.cursor/rules/hermetic-ship-modules.mdc`.

| Must | Detail |
|------|--------|
| No slits | Wall↔ceiling↔floor↔canopy↔collar↔frames: zero daylight. Overlap ≥ 2 cm, never a smaller insert in a larger hole. |
| One join profile | Shell passage, collar **hole**, canopy aft ring, and aft frames share the same opening (or canopy ≥ hole). |
| Seat into hull | Glazing/frames overlap collar/shell along X by ≥ 15 cm. |
| Walkable nose | Clear half-width ≥ 200 cm; portal chamfer ≤ 20 cm. |
| Floor continuous | Top = `FLOOR_TOP_Z` (−184) into the nose; no step under the sill. |
| Glass on sill | Canopy/glass bottom corners at **`FLOOR_TOP_Z`** (not floating above). Floating glass → white floor lip under the pane from outside. |
| Floor = inner face | Under glazing, floor outline = **inner** face of glass/frame/cheek (raycast after canopy exists). Node centerline / outer face → lip past module; big inset → sky gap. |
| Verify before done | After Blender export run hermetic bounds check (see `verify_hermetic` in bridge generator). Do **not** author until OK. |

**Do not ship** a module with circled “sky seams”, paper-thin wall ends floating next to glass, a canopy that sits inside a larger empty bevel, or a white floor/shell slab past the exterior silhouette.

## Collision (walkable hollow modules)

Preview ISMs default to **BlockAll**. Wrong collision = blocked nose **or** walk-through walls.

| Part | Collision | Why |
|------|-----------|-----|
| Floor | **Simple BOX** + `CTF_USE_SIMPLE_AS_COMPLEX` | Characters need a walk surface. |
| Shell / cheeks / glass / frames / porthole glass | **`CTF_USE_COMPLEX_AS_SIMPLE`**, no auto convex | Matches open/hollow mesh. |
| Collar / solid decor that is a holed flange | Often **no collision** | Convex/complex on a thick flange with a hole still blocks the doorway. |
| Import | `auto_generate_collision=False` for hollow kits | Auto convex **fills the cabin** and blocks the canopy threshold. |

**C++ preview** (`ShipBuilderModulePreviewActor`): enable override ISM collision only for parts that should block (Floor, Shell, SideWalls, Glass, Frame, Porthole) — not every VisualPart.

**Open nose:** loft shell with **no forward bulkhead** (`open_forward`) so complex collision does not wall off a walkable canopy. Door/threshold blocking after “open” visuals → check residual bulkhead or convex fill first.

## Canopy / bow modules (Bridge pattern)

Reference: `tools/blender/generate_bridge_module.py`, `Content/Python/author_bridge_module.py`, `Bridge_Training_01`.

1. **Aft shell only** up to `CANOPY_START_X`; nose = glass + frames + cheeks (not a second solid hull).
2. **Build order:** shell → portholes (bore shell) → collar/cheeks/glass/frames → **floor last** (raycast-fit to canopy inner faces).
3. **`floor_cut` boolean:** cut belly **only under the aft cabin**. A cutter that extends into the nose leaves an **end-cap slab** at the tip (looks like floor sticking past glass). Hermetic: `shell.maxX ≤ CANOPY_START + ~40`; strip debris verts past the join.
4. **Cheeks:** lower fairings at the doorway (diagonal top), not full side walls; L/R mirrors.
5. **Portholes:** rim + glass through the wall (cut shell). Do **not** use thruster/gondola stubs that pierce into the cabin (look like random cylinders inside).
6. Grid for TrainingShip authors: `PANEL_XY` / `PANEL_Z` from `ship_builder_grid` (often **800×400** cell unit — do not hardcode 600 if the project grid differs).

## Part placement (mandatory — never skip)

**Symptom of violation:** textured door / OpeningMesh floats in the **middle of the room**; walls/floors pile at module center; dock openings look “broken”.

| Rule | Detail |
|------|--------|
| Mesh local origin | Each FBX is **local-centered** (bounds center ≈ `(0,0,0)`). Do **not** bake Blender object `location` into the mesh as the only placement. |
| `RelativeTransform` | **Always** set translation (and yaw) on every VisualPart. Identity is only OK for a mesh that truly belongs at module center. |
| OpeningMesh | Uses the **same** `RelativeTransform` as the closed wall (unless `bUseOpeningRelativeTransform`). Never leave opening at identity while closed mesh is offset. |
| Floor / ceiling | Floor mesh ~**16 cm** thick, local-centered; Rel `(0,0,-192)` → walkable top ≈ **−184** (matches `FloorTopLocalZ` in TrainingGameMode). Ceiling `(0,0,191)`. Thin 4 cm floors cause step/collision issues vs corridor `SM_Floor_400`. |
| Walls 1×1 | Left `(0,-390,0)` yaw 0; Right `(0,390,0)` yaw 180; Front `(391,0,0)` yaw 180; Back `(-391,0,0)` yaw 0. |
| Wider cells | Scale offsets with half-extent (`±(CellX*300−10)` on X, `±(CellY*300−10)` on Y). Socket OpeningMesh sits on the **socket face**, not module center. |

### Blender export (required pattern)

1. Mesh data local-centered (object origin = geometry bounds center).
2. Keep authoring `location` in Blender only for preview layout.
3. **Before FBX export:** temporarily set `location=(0,0,0)`, export selection, restore location.
4. Write placements to JSON / hardcode the same coords into `make_part(..., location=...)`.
5. Reference: `Content/Meshes/TrainingShip/part_placements.json` + `Content/Python/author_training_ship_modules.py`.

### UE Python transforms

```python
# OK — explicit FQuat(X,Y,Z,W) yaw around Z
yaw_rad = math.radians(yaw_deg)
tr.rotation = unreal.Quat(0.0, 0.0, math.sin(yaw_rad * 0.5), math.cos(yaw_rad * 0.5))

# BROKEN in UE Python — do not use
# unreal.Quat(unreal.Vector(0,0,1), yaw_rad)
```

Prefer `Content/Python/author_corridor_wall_slots.py` / `author_training_ship_modules.py` patterns over new one-off scripts.

### After-author verify (blocking)

Log **every** VisualPart: `loc=(x,y,z) yaw=… sock=…`. Fail the task if:

- Dock wall / OpeningMesh has `loc≈(0,0,0)` (unless intentionally center decor).
- `yaw` not in `{0, ±180}` for panel walls (pitch must be ~0).
- OpeningMesh family mismatches face (LR door on Front/Back).

PIE: alone = solid walls; docked = OpeningMesh **on the shared wall**, not room center.

## Assets (always pair)

| Asset | Path |
|-------|------|
| Definition | `/Game/Data/ShipModules/{Id}` |
| VisualOverride | `/Game/Data/ShipModules/{Id}_VisualOverride` |

Definition: `ModuleId`, `ModuleType`, `CellSize`, `bHasInterior=true` (for walkable rooms), soft ref → VisualOverride, **non-empty `ContactPoints`** (panel sockets for CellSize).

Docs (read only if needed): `docs/SHIP_BUILDER_GRID_RU.md`, `docs/SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md`, `docs/VISUAL_OVERRIDE_DOCK_OPENINGS_RU.md`.

## Workflow checklist

```
- [ ] Hermetic OK (no sky slits; glass on FLOOR_TOP; floor ≤ canopy inner; shell not past canopy)
- [ ] Collision: floor box; hollow walls complex; no auto convex; preview enables BlockAll only on blocking parts
- [ ] Meshes local-centered; placements recorded (not identity VisualParts)
- [ ] Export FBX with object location cleared; import UE; verify bounds ~cm
- [ ] Materials: used_with_instanced_static_meshes=true (preview uses ISM)
- [ ] Definition + VisualOverride VisualParts with RelativeTransform on every part
- [ ] Dock walls: WallSocketName + OpeningMesh (same RelativeTransform / axes)
- [ ] ContactPoints populated for CellSize (Python helper if regenerate_* not exposed)
- [ ] Apply via UnrealEditor-Cmd with Editor CLOSED (or Output Log `py`)
- [ ] Log loc/yaw/sock; PIE: openings on walls, not room center; cannot walk through walls; can enter canopy if designed open
```

## Mesh orientations (critical)

LR panels (Left/Right): **thin in Y**, size ~600×20×400.  
FB panels (Front/Back): **thin in X**, size ~20×600×400.

| Face | Closed Mesh | OpeningMesh | Yaw | Location (1×1) |
|------|-------------|-------------|-----|----------------|
| Left | `*_Solid` (LR) | `*_Door` (LR) | 0 | `(0,-290,0)` |
| Right | `*_Solid` (LR) | `*_Door` (LR) | 180 | `(0,290,0)` |
| Front | `*_Solid_FB` | `*_Door_FB` | 180 | `(291,0,0)` |
| Back | `*_Solid_FB` | `*_Door_FB` | 0 | `(-291,0,0)` |

**Never** put LR mesh on Front/Back with yaw ±90 — UE Python Transform often stores yaw as pitch; panel stands across the corridor.

Panel sockets: `Left_X0_Y0_Z0`, `Right_…`, `Front_…`, `Back_…` (1×1×1). Multi-cell: `Front_X{cx-1}_Y{iy}_Z{iz}`, etc.

## Multi-cell ship chain layout

`GridPos` = footprint **min corner**. A **2×2** on a **1-wide** spine is offset by 300 cm in Y vs 1×1 neighbors — looks like broken docking. Prefer **2×1** (extend along +X) for linear ships, or accept/document the overhang.

## Authoring Python

Prefer extending `Content/Python/author_corridor_wall_slots.py` or `author_training_ship_modules.py` over rewriting.

Run with **no** `UnrealEditor.exe` holding the project:

```powershell
Get-Process UnrealEditor*,UnrealEditor-Cmd* -ErrorAction SilentlyContinue | Stop-Process -Force

& "Z:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "H:\Projects\SpaceshipCrew\SpaceshipCrew.uproject" `
  "-ExecutePythonScript=H:\Projects\SpaceshipCrew\Content\Python\author_corridor_wall_slots.py" `
  -unattended -nop4 -nosplash -stdout
```

In a running Editor Output Log: `py "H:/Projects/SpaceshipCrew/Content/Python/author_corridor_wall_slots.py"`.

Confirm Cmd actually ran the script (asset `LastWriteTime` updates; log contains start/done markers). Empty or truncated scripts (missing `main`) exit silently.

## FBX import scale

Kit FBX from this project often needs `import_uniform_scale=0.01`.  
Newly exported Blender FBX (1 BU = 1 cm, local-centered) usually `1.0`. **Always compare** `get_bounds().box_extent*2` to a known-good mesh (e.g. Door_FB ≈ 20×600×400). Helper: `Content/Python/fix_solid_fb_scale.py`.

## C++ preview behavior

`AShipBuilderModulePreviewActor`: if VisualOverride has VisualParts → draw parts; open `WallSocketName` → `OpeningMesh` (empty = hole). Override ISM pools that walk on must use **`BlockAll` + QueryAndPhysics** (`SetCollisionEnabled` alone is not enough).

Collision policy for hollow overrides: **Floor = box**; **Shell/Glass/Frame/cheeks/portholes = complex-as-simple**; skip collision on parts whose convex/complex fill blocks passages (e.g. collar). Never leave `auto_generate_collision` on hollow FBX imports. Needs compiled module reflection for new UPROPERTY fields. Live Coding blocks external `Build.bat` — close Editor or Ctrl+Alt+F11.

## Do not

- Call `make_part(mesh)` / `make_part(mesh, wall_socket=…, opening_mesh=…)` **without** wall/floor `location` (identity RelativeTransform).
- Rely on Blender object transforms alone so OpeningMesh (kit door at origin) spawns at module center when docked.
- Leave permanent door meshes in VisualParts without `WallSocketName` (doors always open).
- Use `unreal.Quat(Vector, angle)` for yaw.
- Clear `ContactPoints` to `[]` without rebuilding panel sockets (Validate fails; docking breaks).
- Use Unreal-MCP for asset create (only AIAssistant / BT toolsets).
- Rely on Remote Python unless user enabled it and nodes discover.
- Edit `.uasset` by hand; use Editor Python.
- Import hollow shells with **auto convex collision** (fills cabin / blocks nose).
- Cut shell belly with a boolean box that reaches into the canopy (leaves tip end-cap “floor lip”).
- Float glass above `FLOOR_TOP_Z` or end the floor at outer/node silhouette (exterior lip or interior gap).
- Pierce cabin walls with thruster stubs; use cut portholes instead.

## Reference corridor

Example module: `Corridor_CustomPanels_01` + `_VisualOverride`.  
Meshes: `Content/Meshes/CorridorPanels/` (`SM_Floor_400`, `SM_Ceiling_400`, `SM_Wall_Solid_400x300`, `SM_Wall_Solid_FB_400x300`, `SM_Wall_Door_400x300`, `SM_Wall_Door_FB_400x300`).

Training ship example: `Airlock_Training_01` / `CargoHold_Training_01` / `Bridge_Training_01` + `author_training_ship_modules.py`.

More detail: [reference.md](reference.md).
