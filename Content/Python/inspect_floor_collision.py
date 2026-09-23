import unreal

def inspect(path):
    m = unreal.EditorAssetLibrary.load_asset(path)
    if not m:
        unreal.log_error("missing " + path)
        return
    b = m.get_bounds()
    e = b.box_extent * 2.0
    o = b.origin
    unreal.log("%s size=(%.1f,%.1f,%.1f) origin=(%.1f,%.1f,%.1f)" % (m.get_name(), e.x, e.y, e.z, o.x, o.y, o.z))
    bs = m.get_editor_property("body_setup")
    if not bs:
        unreal.log("  NO body_setup")
        return
    try:
        unreal.log("  collision_trace_flag=%s" % bs.get_editor_property("collision_trace_flag"))
    except Exception as ex:
        unreal.log("  flag err %s" % ex)
    try:
        aggs = bs.get_editor_property("agg_geom")
        if aggs:
            boxes = aggs.get_editor_property("box_elems") if aggs else None
            spheres = aggs.get_editor_property("sphere_elems") if aggs else None
            convex = aggs.get_editor_property("convex_elems") if aggs else None
            unreal.log("  boxes=%s spheres=%s convex=%s" % (
                len(boxes) if boxes else 0,
                len(spheres) if spheres else 0,
                len(convex) if convex else 0,
            ))
        else:
            unreal.log("  agg_geom empty")
    except Exception as ex:
        unreal.log("  agg err %s" % ex)

for p in [
    "/Game/Meshes/CorridorPanels/SM_Floor_400",
    "/Game/Meshes/TrainingShip/SM_Airlock_Floor",
    "/Game/Meshes/TrainingShip/SM_Cargo_Floor",
    "/Game/Meshes/TrainingShip/SM_Bridge_Floor",
    "/Game/Meshes/TrainingShip/SM_Cargo_Crate_0",
]:
    inspect(p)

# Also check cargo VO floor part
vo = unreal.EditorAssetLibrary.load_asset("/Game/Data/ShipModules/CargoHold_Training_01_VisualOverride")
parts = vo.get_editor_property("visual_parts") if vo else []
for i, p in enumerate(parts or []):
    mesh = p.get_editor_property("mesh")
    if mesh and "Floor" in mesh.get_name():
        tr = p.get_editor_property("relative_transform")
        unreal.log("Cargo VO floor part mesh=%s loc=%s" % (mesh.get_name(), tr.translation))
unreal.log("=== inspect_done ===")
