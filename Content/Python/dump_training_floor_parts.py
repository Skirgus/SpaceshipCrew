import unreal

def dump(mid):
    vo = unreal.EditorAssetLibrary.load_asset("/Game/Data/ShipModules/%s_VisualOverride" % mid)
    if not vo:
        unreal.log_error("no vo %s" % mid)
        return
    parts = vo.get_editor_property("visual_parts") or []
    unreal.log("=== %s parts=%d ===" % (mid, len(parts)))
    for i, p in enumerate(parts):
        mesh = p.get_editor_property("mesh")
        tr = p.get_editor_property("relative_transform")
        sock = p.get_editor_property("wall_socket_name")
        name = mesh.get_name() if mesh else "None"
        t = tr.translation
        if "Floor" in name or "Ceiling" in name or abs(t.z) > 50 or abs(t.x) > 50 or abs(t.y) > 50 or sock:
            unreal.log("  [%d] %s loc=(%.1f,%.1f,%.1f) yaw=%.0f sock=%s" % (i, name, t.x, t.y, t.z, tr.rotation.rotator().yaw, sock))
        if mesh and "Floor" in name:
            e = mesh.get_bounds().box_extent * 2.0
            o = mesh.get_bounds().origin
            unreal.log("       mesh_origin=(%.1f,%.1f,%.1f) mesh_size=(%.1f,%.1f,%.1f)" % (o.x, o.y, o.z, e.x, e.y, e.z))

for mid in ["Airlock_Training_01", "CargoHold_Training_01", "Bridge_Training_01", "Corridor_CustomPanels_01"]:
    dump(mid)
unreal.log("=== dump_floor_parts done ===")
