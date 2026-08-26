# Ship module creation — reference

## Definition fields (minimum)

- `ModuleId` (FName, unique)
- `ModuleType` (Corridor / Hab / …)
- `CellSize` → drives `Size` and default panel sockets
- `bHasInterior` — true for rooms that get dock openings toward neighbors
- `VisualOverride` → soft object to `UShipModuleVisualOverride`

## FShipModuleVisualPart (dock)

| Property | Use |
|----------|-----|
| `Mesh` | Closed / always-on mesh |
| `RelativeTransform` | Local to module center |
| `WallSocketName` | Panel socket; `None` = never swapped |
| `OpeningKind` | `Passage` \| `SlidingDoor` |
| `OpeningMesh` | Drawn when socket open; empty = hole |
| `bUseOpeningRelativeTransform` | Optional separate transform for OpeningMesh |
| `OpeningRelativeTransform` | Only if flag true (e.g. mixed LR/FB without FB solid) |
| `bAirtightWhenClosed` / `bAffectsOxygenVolume` | Data only until O₂ sim |

Preview open sockets: `Draft.Connections` (+ domain fallback), interior neighbor check, `ForcedOpeningSide` (airlock).

## Blender kit conventions

- One object per export (`SM_*`).
- LR wall: extent X=400, Y≈20, Z=300; detail faces +Y.
- FB wall: extent X≈20, Y=400, Z=300; rotate LR mesh +90° Z in mesh space and apply.
- Door meshes match solid extents with a cutout for the passage.
- Export FBX selected; keep axis consistent with existing kit (`CorridorPanels_Kit.blend`).

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

## Verify after author

```
Parts=6
[2] … Left_… opening=SM_Wall_Door_400x300 yaw=0.0 pitch=0.0
[3] … Right_… opening=SM_Wall_Door_400x300 yaw=180.0 pitch=0.0
[4] … Front_… opening=SM_Wall_Door_FB_400x300 yaw=180.0 pitch=0.0
[5] … Back_… opening=SM_Wall_Door_FB_400x300 yaw=0.0 pitch=0.0
```

Binary smoke check: ASCII strings in `.uasset` for socket names and mesh names (no `Door_FB` on Left/Right).

## Related docs

- `docs/SHIP_BUILDER_GRID_RU.md` — grid, sockets, schema
- `docs/SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md` — BigModular pack path
- `docs/VISUAL_OVERRIDE_DOCK_OPENINGS_RU.md` — dock swap + O₂ roadmap
