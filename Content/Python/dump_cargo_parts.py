"""Dump CargoHold VisualPart mesh names."""
import unreal

VO = "/Game/Data/ShipModules/CargoHold_Training_01_VisualOverride"
vo = unreal.EditorAssetLibrary.load_asset(VO)
parts = list(vo.get_editor_property("visual_parts") or [])
unreal.log("CargoHold parts=%d" % len(parts))
for i, p in enumerate(parts):
	mesh = p.get_editor_property("mesh")
	name = mesh.get_name() if mesh else "<opening-only>"
	tr = p.get_editor_property("relative_transform")
	t = tr.translation
	unreal.log("  [%d] %s loc=(%.1f,%.1f,%.1f)" % (i, name, t.x, t.y, t.z))
