# Ship module creation — reference

## Definition fields (minimum)

- `ModuleId` (FName, unique)
- `ModuleType` (Corridor / Hab / …)
- `CellSize` → drives `Size` and default panel sockets
- `bHasInterior` — true for rooms that get dock openings toward neighbors
- `VisualOverride` → soft object to `UShipModuleVisualOverride`
- `ContactPoints` — non-empty panel sockets; `GatherContactPointsForPlacement` merges defaults at runtime, but **Validate** requires authored/non-empty `ContactPoints`

`UShipModuleDefinition::RegenerateDefaultContactPointsFromCellSize` is editor C++ only (often **not** exposed to Python). Build panel contacts in Python (see `author_training_ship_modules.make_panel_contacts`) or duplicate a known-good Definition then overwrite.

## FShipModuleVisualPart (dock)

| Property | Use |
|----------|-----|
| `Mesh` | Closed / always-on mesh (**local-centered** asset) |
| `RelativeTransform` | Local to module center — **required** for floors/walls/openings |
| `WallSocketName` | Panel socket; `None` = never swapped |
| `OpeningKind` | `Passage` \| `SlidingDoor` |
| `OpeningMesh` | Drawn when socket open; empty = hole; **same transform as Mesh** by default |
| `bUseOpeningRelativeTransform` | Optional separate transform for OpeningMesh |
| `OpeningRelativeTransform` | Only if flag true (e.g. mixed LR/FB without FB solid) |
| `bAirtightWhenClosed` / `bAffectsOxygenVolume` | Data only until O₂ sim |

Preview open sockets: `Draft.Connections` (+ domain fallback), interior neighbor check, `ForcedOpeningSide` (airlock).

### Known failure: door in room center

Cause: closed wall mesh vertices already offset in the asset **or** Blender `location` baked into FBX, while VisualPart `RelativeTransform` is identity **and** OpeningMesh is a kit door centered at origin. When the socket opens, OpeningMesh draws at module center.

Fix: export local-centered meshes + set RelativeTransform for both closed and opening paths (corridor / training author scripts).

## Blender kit conventions

- One object per export (`SM_*`).
- Mesh local bounds center ≈ `(0,0,0)`; object `location` = intended RelativeTransform for preview only.
- Export: clear object location → FBX → restore. Persist placements (`part_placements.json` or hardcoded tuples).
- LR wall: extent X=600, Y≈20, Z=400; detail faces +Y.
- FB wall: extent X≈20, Y=600, Z=400; rotate LR mesh +90° Z in mesh space and apply.
- Door meshes match solid extents with a cutout for the passage.
- Axis consistent with existing kit (`CorridorPanels_Kit.blend`). TrainingShip: `Content/Meshes/TrainingShip/`.

## Typical RelativeTransform (1×1×1 cell)

| Part | Location | Yaw |
|------|----------|-----|
| Floor | `(0, 0, -192)` | 0 | Mesh ~16 cm thick; top ≈ −184 (`FloorTopLocalZ`) |
| Ceiling | `(0, 0, 191)` | 0 | |
| Left | `(0, -390, 0)` | 0 |
| Right | `(0, 390, 0)` | 180 |
| Front | `(391, 0, 0)` | 180 |
| Back | `(-391, 0, 0)` | 0 |

For CellSize `(Cx, Cy, 1)`: Back/Front at `±(Cx*300 − 10)`, Left/Right at `±(Cy*300 − 10)`.

## Materials / PIE white or WorldGrid

1. Real textures (not 32×32 stubs).
2. `used_with_instanced_static_meshes` on materials used in builder preview.
3. Assign materials on StaticMesh slots; ISM pools copy them in `GetOrCreatePool`.

Scripts under `Content/Python/`: `apply_corridor_textures.py`, `force_pbr_ism_assign.py`, `enable_ism_on_corridor_mats.py`.

## Cmd vs open Editor

| Situation | Action |
|-----------|--------|
| Editor closed | `UnrealEditor-Cmd -ExecutePythonScript=...` |
| Editor open, assets locked (Error 32) | Stop-Process UnrealEditor* then Cmd, or `py` in Output Log |
| New UPROPERTY not in Python | Compile SpaceshipCrew (close Live Coding / Ctrl+Alt+F11) |
| Script exits with no logs / assets unchanged | File truncated or missing `main()`; check line count and `LastWriteTime` |

## Verify after author

```
Airlock_Training_01[0] loc=(0,0,-192) yaw=0 sock=None
…
Airlock_Training_01[4] loc=(-291,0,0) yaw=0 sock=Back_X0_Y0_Z0
Airlock_Training_01: parts=9 cell=(1, 1, 1) contacts=6
```

Reject if dock OpeningMesh / wall part logs `loc=(0,0,0)`.

Binary smoke check: ASCII strings in `.uasset` for socket names and mesh names (no `Door_FB` on Left/Right).

## Bridge / canopy module (TrainingShip)

Scripts: `tools/blender/generate_bridge_module.py`, `Content/Python/author_bridge_module.py`.  
Assets: `Bridge_Training_01`, meshes under `Content/Meshes/TrainingShip/SM_Bridge_*`.

### Geometry pipeline

1. Unified **aft shell** to `CANOPY_START_X` (`open_forward=True` — no forward bulkhead).
2. **Portholes:** bore shell; export rim + glass (not thruster stubs through the cabin).
3. Collar, cheeks, canopy glass/frames (shared node topology; cheeks = lower fairings only).
4. **Floor last:** raycast to inner faces of glass/frame/cheek; sill glass at `FLOOR_TOP_Z`.
5. `verify_hermetic()` before author — includes floor tip vs glass, glass Z vs floor, shell maxX vs canopy.

### Floor cut / boolean

```text
OK:  floor_cut box only under aft cabin (X in [BACK, CANOPY_START])
BAD: floor_cut length spanning into nose → leftover end-cap at tip (±Y wide) looks like floor lip
```

After booleans: delete shell verts with `X > CANOPY_START + 25` if debris remains.

### Collision authoring

```python
# Import hollow kits without auto convex
options.static_mesh_import_data.set_editor_property("auto_generate_collision", False)

# Floor: BOX + SIMPLE_AS_COMPLEX
# Shell, SideWalls, Canopy_Glass/Frame, Porthole_*: COMPLEX_AS_SIMPLE, remove_collisions first
# Collar: usually leave empty (blocks doorway if filled)
```

Preview C++: `GetOrCreatePool(..., bEnableCollision)` only for Floor / Shell / SideWalls / Glass / Frame / Porthole.

### Exterior vs interior

White “shelf” past glass from outside → check **shell boolean debris** and **glass Z / floor inner fit**, not only floor half-width.  
Dark cylinder on interior wall → thruster/gondola piercing the wall; replace with porthole.

## Related docs

- `docs/SHIP_BUILDER_GRID_RU.md` — grid, sockets, schema
- `docs/SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md` — BigModular pack path
- `docs/VISUAL_OVERRIDE_DOCK_OPENINGS_RU.md` — dock swap + O₂ roadmap
