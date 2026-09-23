import unreal

def show(path):
    m = unreal.EditorAssetLibrary.load_asset(path)
    if not m:
        unreal.log_error("missing " + path)
        return
    b = m.get_bounds()
    e = b.box_extent * 2.0
    o = b.origin
    unreal.log("%s origin=(%.2f,%.2f,%.2f) size=(%.1f,%.1f,%.1f) minz=%.2f maxz=%.2f" % (
        path.split("/")[-1], o.x, o.y, o.z, e.x, e.y, e.z, o.z - b.box_extent.z, o.z + b.box_extent.z))

for p in [
    "/Game/Meshes/CorridorPanels/SM_Floor_400",
    "/Game/Meshes/TrainingShip/SM_Airlock_Floor",
    "/Game/Meshes/TrainingShip/SM_Cargo_Floor",
    "/Game/Meshes/TrainingShip/SM_Bridge_Floor",
    "/Game/Meshes/TrainingShip/SM_Airlock_Wall_Left",
    "/Game/Meshes/CorridorPanels/SM_Wall_Solid_400x300",
]:
    show(p)
unreal.log("=== mesh_bounds_done ===")
