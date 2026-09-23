---
name: usable-equipment-animation
description: >-
  Authors SpaceshipCrew usable equipment and crew sit animations (AUsableEquipment,
  BP_CrewSeat, Manny sit montage, crew-seat FBX). Use when sitting looks wrong,
  feet float or clip, knees stay tucked, a hand sticks to the helmet, the chair
  proportions do not fit the mannequin, or when changing UseAnchor, CharacterUseMontage,
  AS_CrewSeat_Sit, AM_CrewSeat_Sit, SM_CrewSeat, ik_foot, or Occupy. Also activate
  on Russian requests such as: кресло, сесть, посадка, анимация сидения, предмет,
  оборудование, монтаж, рука у шлема, ноги проваливаются, ступни в воздухе,
  неестественная поза, пропорции стула.
---

# Usable equipment and sit animation

Do not put this in the ship-module skill. Modules and props do not share axes or placement.

Scripts: `tools/blender/generate_crew_seat.py`, `Content/Python/author_crew_seat.py`, `Content/Python/author_crew_seat_sit.py`.

## Save only sticks if the editor is closed

`UnrealEditor-Cmd` can log `done` while the `.uasset` is unchanged. Windows error 32 (`Failed to move` / `Error saving`) means Unreal Editor or Live Coding still has the file.

1. `Get-Process UnrealEditor,UnrealEditor-Cmd,LiveCodingConsole` must be empty. Do not kill the user's editor. Ask them to close it.
2. After the commandlet, the asset `LastWriteTime` must change and the log must not contain `Error saving`.
3. If the player says nothing changed, check the timestamp before changing the pose again.

```powershell
& "Z:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "H:\Projects\SpaceshipCrew\SpaceshipCrew.uproject" `
  "-ExecutePythonScript=H:\Projects\SpaceshipCrew\Content\Python\author_crew_seat_sit.py" `
  -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput
```

Do not author through user-unreal-mcp. Remote Python is off. Do not hand-edit `.uasset`.

`author_crew_seat.py` rewrites the Bridge visual-override placement every run. Reimport a mesh with `import_materials=False`, `convert_scene=True`, scale `1.0`, then put the previous material slots back. Do not re-run the full chair author just to refresh the FBX.

## Axes (this FBX, measured)

Export `axis_forward=-Y`, `axis_up=Z`. **UE `(x,y,z) = Blender `(bx, -by, bz)`.** The generator print that says Blender −Y → UE +X is wrong. Chair front (knees) = UE +Y. Backrest = UE −Y. 1 Blender unit = 1 cm. Mesh origin on the floor.

Manny (`SKM_Manny_Simple`): mesh relative `(0,0,-96)`, yaw −90. Capsule half-height 96, radius 42. Mesh point `(mx,my,mz)` → actor `(my, -mx, mz-96)`. UseAnchor yaw +90 (actor faces equipment +Y): equipment = anchor + `(mx, my, mz-96)`. Anchor Z stays 96. Skeleton faces mesh +Y. Do not take facing from head−pelvis; the head is behind the pelvis in the ref pose.

Component rotation: `unreal.Rotator(pitch=, yaw=, roll=)` and log the saved rotator. `Rotator.quaternion()` and `unreal.Quat(Vector, angle)` store yaw as pitch. For an `FTransform.rotation`, write an explicit Z-yaw quat `(0, 0, sin(yaw/2), cos(yaw/2))`.

## Sit that fits Manny

Thigh 43.3 cm, calf 42.2 cm, ankle bone Z ≈ 8.2 (sole on the floor). A normal sit is a horizontal thigh and a shin about 12 cm forward of the knee, foot on the floor. Seat top ≈ 43 cm. Seat depth must end at the knee: a longer cushion leaves the knee on the pad, and a vertical shin clips the front rail. Straightening the shin to ~32 cm looks like a lounge chair, not a sit.

`ABP_Unarmed` plays `DefaultSlot`, then `CR_Mannequin_FootIK` (PBIK). Effectors are `ik_foot_l` / `ik_foot_r`, parented to `ik_foot_root`, not to `foot_l`. An animation that does not key `ik_foot_*` keeps those bones in the standing pose, so the knees stay tucked no matter what the calf keys say. `ShouldDoIKTrace` only clears the Z offset; PBIK still runs. Key `ik_foot_*` to the planted feet (and `ik_hand_*` to the hands). Sample frame 20 with `AnimPoseEvaluationOptions` / `SOURCE` and compare to the solver. If the sample matches and the screenshot does not, the file was not saved or IK is still overriding.

Arm reach is about 55 cm. From the reclined shoulder the knee is out of reach; a clamped target pulls the hand up to the helmet. Put both hands on the upper thigh, pole the elbow out and down, and key `hand_l` / `hand_r`.

Changing seat depth recenters the mesh and slides the backrest. After centering, shift Y so the backrest front stays at the previous UE position (`BACKREST_FRONT_BLENDER_Y` in the generator). Do not move `SEAT_HIP_LOCKED_Y` or the approved recline unless the back contact log breaks.

## Animation assets (UE 5.8)

Do not use `AnimSequenceFactory` / `AnimMontageFactory` (`ConfigureProperties` nulls the skeleton and hangs `-unattended`). Duplicate `MF_Unarmed_Walk_Fwd` → `AS_CrewSeat_Sit` and `MM_Pistol_Fire_Montage` → `AM_CrewSeat_Sit`. Use `unreal.AnimationLibrary`. `AnimPoseSpaces.WORLD` is component space.

This Python build multiplies transforms backwards. Measure it: `make_relative_transform(child, parent)` plus `compose_transforms(local, parent)` reproduced the thigh with error 0. Do not “fix” that to parent-first.

Montage: slot `DefaultSlot`, one section `Default`, `enable_auto_blend_out` false, pistol notifies removed. Sequence: root motion off, force root lock on. `FAnimSegment.start_pos` is read-only. `notifies` is protected.

## Do not

- Treat a solver log as proof the player will see the pose.
- Re-run `author_crew_seat.py` to tweak a sit (it rewrites the bridge placement and, if the montage is empty, the stand anchor).
- Import the chair with `import_materials=True` (replaces the four seat materials).
- Derive the chair front from the generator's axis comment.
