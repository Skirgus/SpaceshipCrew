"""Remove decorative cargo crates/pad from CargoHold VisualOverride."""
import unreal

VO = "/Game/Data/ShipModules/CargoHold_Training_01_VisualOverride"
DROP = ("Crate", "Pad")


def main():
	vo = unreal.EditorAssetLibrary.load_asset(VO)
	if not vo:
		unreal.log_error("missing %s" % VO)
		return
	parts = list(vo.get_editor_property("visual_parts") or [])
	kept = []
	for p in parts:
		mesh = p.get_editor_property("mesh")
		name = mesh.get_name() if mesh else ""
		if any(tok in name for tok in DROP):
			unreal.log("drop VisualPart %s" % name)
			continue
		kept.append(p)
	vo.set_editor_property("visual_parts", kept)
	unreal.EditorAssetLibrary.save_asset(VO)
	unreal.log("CargoHold VisualParts %d -> %d" % (len(parts), len(kept)))


main()
