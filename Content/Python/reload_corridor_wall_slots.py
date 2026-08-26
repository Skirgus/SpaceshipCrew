# Reload VO from disk (if needed) and re-author wall opening slots.
# Output Log: py "H:/Projects/SpaceshipCrew/Content/Python/reload_corridor_wall_slots.py"
import importlib.util
import unreal

AUTHOR = r"H:/Projects/SpaceshipCrew/Content/Python/author_corridor_wall_slots.py"
VO = "/Game/Data/ShipModules/Corridor_CustomPanels_01_VisualOverride"


def _run_author():
    spec = importlib.util.spec_from_file_location("author_corridor_wall_slots", AUTHOR)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mod.main()


def main():
    unreal.log("=== reload_corridor_wall_slots ===")
    try:
        pkg = unreal.find_package("/Game/Data/ShipModules")
        if pkg:
            unreal.PackageTools.reload_packages([pkg])
            unreal.log("Reloaded /Game/Data/ShipModules")
    except Exception as exc:
        unreal.log_warning(f"Package reload skipped: {exc}")

    _run_author()

    vo = unreal.EditorAssetLibrary.load_asset(VO)
    parts = (vo.get_editor_property("visual_parts") if vo else None) or []
    tagged = sum(
        1
        for p in parts
        if p.get_editor_property("wall_socket_name")
        and str(p.get_editor_property("wall_socket_name")) not in ("None", "")
    )
    unreal.log(f"Tagged wall parts={tagged} (expect 4)")
    unreal.log("=== reload_corridor_wall_slots done ===")


if __name__ == "__main__":
    main()
