# Reimport thickened TrainingShip floors and keep RelativeTransform tops at FloorTopLocalZ (-134).
# Output Log: py "H:/Projects/SpaceshipCrew/Content/Python/fix_training_floors.py"

import unreal
import os

MESH_DIR = "/Game/Meshes/TrainingShip"
FBX_DIR = unreal.Paths.project_content_dir() + "Meshes/TrainingShip/"
MODULE_DIR = "/Game/Data/ShipModules"

# Mesh local Z half-extent 8 → Rel Z -142 puts walkable top at -134 (matches GameMode FloorTopLocalZ).
FLOOR_LOCS = {
	"SM_Airlock_Floor": (0.0, 0.0, -142.0),
	"SM_Cargo_Floor": (0.0, 0.0, -142.0),
	"SM_Bridge_Floor": (0.0, 0.0, -142.0),
	"SM_Cargo_Pad": (0.0, 0.0, -135.0),
}


def import_mesh(mesh_name):
	path = "%s/%s" % (MESH_DIR, mesh_name)
	fbx_path = FBX_DIR + mesh_name + ".fbx"
	if not os.path.isfile(fbx_path):
		unreal.log_error("FBX missing: %s" % fbx_path)
		return unreal.EditorAssetLibrary.load_asset(path)

	task = unreal.AssetImportTask()
	task.filename = fbx_path
	task.destination_path = MESH_DIR
	task.destination_name = mesh_name
	task.replace_existing = True
	task.automated = True
	task.save = True

	options = unreal.FbxImportUI()
	options.import_mesh = True
	options.import_as_skeletal = False
	options.import_materials = False
	options.import_textures = False
	options.automated_import_should_detect_type = False
	options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
	options.static_mesh_import_data.set_editor_property("combine_meshes", True)
	options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
	options.static_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)
	options.static_mesh_import_data.set_editor_property("convert_scene", True)
	task.options = options

	unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
	mesh = unreal.EditorAssetLibrary.load_asset(path)
	if mesh:
		b = mesh.get_bounds()
		e = b.box_extent * 2.0
		o = b.origin
		unreal.log(
			"Imported %s origin=(%.1f,%.1f,%.1f) size=(%.1f,%.1f,%.1f) top_if_rel-142=%.1f"
			% (mesh_name, o.x, o.y, o.z, e.x, e.y, e.z, -142.0 + (o.z + b.box_extent.z))
		)
		unreal.EditorAssetLibrary.save_asset(mesh.get_path_name(), only_if_is_dirty=False)
	return mesh


def set_part_translation(part, location):
	tr = part.get_editor_property("relative_transform")
	tr.translation = unreal.Vector(float(location[0]), float(location[1]), float(location[2]))
	part.set_editor_property("relative_transform", tr)


def patch_vo(module_id):
	vo = unreal.EditorAssetLibrary.load_asset("%s/%s_VisualOverride" % (MODULE_DIR, module_id))
	if not vo:
		unreal.log_error("missing VO %s" % module_id)
		return
	parts = list(vo.get_editor_property("visual_parts") or [])
	changed = 0
	for p in parts:
		mesh = p.get_editor_property("mesh")
		if not mesh:
			continue
		name = mesh.get_name()
		if name in FLOOR_LOCS:
			set_part_translation(p, FLOOR_LOCS[name])
			changed += 1
			t = p.get_editor_property("relative_transform").translation
			unreal.log("%s: %s -> loc=(%.0f,%.0f,%.0f)" % (module_id, name, t.x, t.y, t.z))
	vo.set_editor_property("visual_parts", parts)
	unreal.EditorAssetLibrary.save_loaded_asset(vo)
	unreal.log("%s: patched %d floor parts" % (module_id, changed))


def main():
	unreal.log("=== fix_training_floors start ===")
	for name in FLOOR_LOCS:
		import_mesh(name)
	for mid in ("Airlock_Training_01", "CargoHold_Training_01", "Bridge_Training_01"):
		patch_vo(mid)
	unreal.log("=== fix_training_floors done ===")


if __name__ == "__main__":
	main()
