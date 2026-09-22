import unreal

paths = [
	"/Game/Meshes/TrainingShip/SM_Airlock_Floor",
	"/Game/Meshes/TrainingShip/SM_Airlock_Wall_Back",
	"/Game/Meshes/TrainingShip/SM_Cargo_Wall_Back",
	"/Game/Meshes/TrainingShip/SM_Bridge_Shell",
	"/Game/Meshes/TrainingShip/SM_Bridge_Floor",
	"/Game/Meshes/CorridorPanels/SM_Wall_Door_FB_400x300",
	"/Game/Meshes/CorridorPanels/SM_Floor_400",
]
for p in paths:
	m = unreal.EditorAssetLibrary.load_asset(p)
	if not m:
		unreal.log_warning("MISSING %s" % p)
		continue
	e = m.get_bounds().box_extent * 2.0
	o = m.get_bounds().origin
	unreal.log(
		"%s origin=(%.1f,%.1f,%.1f) size=(%.1f,%.1f,%.1f)"
		% (p.split("/")[-1], o.x, o.y, o.z, e.x, e.y, e.z)
	)
