# Анимация посадки на BP_CrewSeat.
# Кадр 0 — опорная стойка, дальше таз уходит назад и вниз, ступни остаются на полу.
# Output Log: py "H:/Projects/SpaceshipCrew/Content/Python/author_crew_seat_sit.py"

from __future__ import annotations

import math

import unreal

MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
SEQ_SRC = "/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd"
MONTAGE_SRC = "/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire_Montage"
ANIM_DIR = "/Game/Characters/Mannequins/Anims/Unarmed"
SEQ_PATH = f"{ANIM_DIR}/AS_CrewSeat_Sit"
MONTAGE_PATH = f"{ANIM_DIR}/AM_CrewSeat_Sit"
BP_PATH = "/Game/Blueprints/Equipment/BP_CrewSeat"

# Импорт FBX зеркалит Blender Y: UE (x, y, z) = (bx, -by, bz).
# Перед сиденья (колени) — +Y, спинка — −Y, боковые пластины — ±X.
# Тазобедренный сустав над подушкой. По Y таз фиксирует SEAT_HIP_LOCKED_Y.
SEAT_HIP_X = 0.0
SEAT_HIP_Z = 54.0
# Актёр смотрит по своему +X. Якорь с yaw +90 направляет его на передний край (+Y).
ANCHOR_YAW = 90.0
# Передняя грань спинки в координатах кресла: низ подушки и её верх.
BACKREST_LOW = (34.4, -19.0)
BACKREST_HIGH = (79.2, -28.5)
# Сколько сантиметров от кости позвоночника до поверхности спины.
TORSO_BACK = 10.0
PELVIS_RECLINE = 12.0
# Грудь чуть возвращаем вперёд: на 12° плечи и шлем ещё выглядывали за верх спинки.
SPINE_RECLINE = -4.0
SPINE2_RECLINE = -2.0
NECK_COUNTER = 6.0
# Таз уже сидит на подушке. Наклон корпуса его не сдвигает.
SEAT_HIP_LOCKED_Y = -13.0
FPS = 30
FRAMES = 21  # ключей на один больше: кадры 0..21
SIT_KEYS = 16

# Меш персонажа: yaw -90, origin у ступней, смещение (0,0,-96).
# В системе актёра точка меша (x, y, z) -> (y, -x, z-96).
# Якорь кресла дополнительно повёрнут на +90, поэтому в системе кресла это (x, y, z-96).


def log(message: str) -> None:
    unreal.log("[CrewSeatSit] " + message)


def v(x: float, y: float, z: float) -> unreal.Vector:
    return unreal.Vector(float(x), float(y), float(z))


def v_add(a: unreal.Vector, b: unreal.Vector) -> unreal.Vector:
    return v(a.x + b.x, a.y + b.y, a.z + b.z)


def v_sub(a: unreal.Vector, b: unreal.Vector) -> unreal.Vector:
    return v(a.x - b.x, a.y - b.y, a.z - b.z)


def v_mul(a: unreal.Vector, scale: float) -> unreal.Vector:
    return v(a.x * scale, a.y * scale, a.z * scale)


def v_len(a: unreal.Vector) -> float:
    return math.sqrt(a.x * a.x + a.y * a.y + a.z * a.z)


def v_norm(a: unreal.Vector) -> unreal.Vector:
    length = v_len(a)
    if length < 1.0e-6:
        return v(0.0, 0.0, 1.0)
    return v_mul(a, 1.0 / length)


def v_dot(a: unreal.Vector, b: unreal.Vector) -> float:
    return a.x * b.x + a.y * b.y + a.z * b.z


def v_cross(a: unreal.Vector, b: unreal.Vector) -> unreal.Vector:
    return v(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    )


def v_lerp(a: unreal.Vector, b: unreal.Vector, alpha: float) -> unreal.Vector:
    return v_add(a, v_mul(v_sub(b, a), alpha))


def q_norm(q: unreal.Quat) -> unreal.Quat:
    length = math.sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w)
    if length < 1.0e-8:
        return unreal.Quat(0.0, 0.0, 0.0, 1.0)
    return unreal.Quat(q.x / length, q.y / length, q.z / length, q.w / length)


def q_mul(a: unreal.Quat, b: unreal.Quat) -> unreal.Quat:
    return q_norm(
        unreal.Quat(
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        )
    )


def q_axis_angle(axis: unreal.Vector, degrees: float) -> unreal.Quat:
    half = math.radians(degrees) * 0.5
    axis = v_norm(axis)
    s = math.sin(half)
    return unreal.Quat(axis.x * s, axis.y * s, axis.z * s, math.cos(half))


def q_from_to(src: unreal.Vector, dst: unreal.Vector) -> unreal.Quat:
    a = v_norm(src)
    b = v_norm(dst)
    dot = max(-1.0, min(1.0, v_dot(a, b)))
    if dot > 0.9999:
        return unreal.Quat(0.0, 0.0, 0.0, 1.0)
    if dot < -0.9999:
        axis = v_cross(a, v(1.0, 0.0, 0.0))
        if v_len(axis) < 1.0e-4:
            axis = v_cross(a, v(0.0, 1.0, 0.0))
        axis = v_norm(axis)
        return unreal.Quat(axis.x, axis.y, axis.z, 0.0)
    cross = v_cross(a, b)
    return q_norm(unreal.Quat(cross.x, cross.y, cross.z, 1.0 + dot))


def q_rot(q: unreal.Quat, vector: unreal.Vector) -> unreal.Vector:
    return unreal.MathLibrary.quat_rotate_vector(q, vector)


def xform(location: unreal.Vector, rotation: unreal.Quat, scale: unreal.Vector | None = None) -> unreal.Transform:
    transform = unreal.Transform()
    transform.translation = location
    transform.rotation = q_norm(rotation)
    transform.scale3d = scale if scale is not None else v(1.0, 1.0, 1.0)
    return transform


# Заполняется после проверки на бедре: как именно этот редактор умножает трансформы.
COMPOSE_PARENT_FIRST = True
RELATIVE_CHILD_FIRST = True


def compose(parent: unreal.Transform, local: unreal.Transform) -> unreal.Transform:
    if COMPOSE_PARENT_FIRST:
        return unreal.MathLibrary.compose_transforms(parent, local)
    return unreal.MathLibrary.compose_transforms(local, parent)


def relative(child: unreal.Transform, parent: unreal.Transform) -> unreal.Transform:
    if RELATIVE_CHILD_FIRST:
        return unreal.MathLibrary.make_relative_transform(child, parent)
    return unreal.MathLibrary.make_relative_transform(parent, child)


def choose_transform_convention(parent: unreal.Transform, child: unreal.Transform) -> float:
    global COMPOSE_PARENT_FIRST, RELATIVE_CHILD_FIRST
    best_error = 1.0e9
    best = (True, True)
    for child_first in (True, False):
        local = (
            unreal.MathLibrary.make_relative_transform(child, parent)
            if child_first
            else unreal.MathLibrary.make_relative_transform(parent, child)
        )
        for parent_first in (True, False):
            world = (
                unreal.MathLibrary.compose_transforms(parent, local)
                if parent_first
                else unreal.MathLibrary.compose_transforms(local, parent)
            )
            error = v_len(v_sub(world.translation, child.translation))
            log(
                "convention relative_child_first=%s compose_parent_first=%s err=%.3f"
                % (child_first, parent_first, error)
            )
            if error < best_error:
                best_error = error
                best = (child_first, parent_first)
    RELATIVE_CHILD_FIRST, COMPOSE_PARENT_FIRST = best
    return best_error


def smoothstep(alpha: float) -> float:
    alpha = max(0.0, min(1.0, alpha))
    return alpha * alpha * (3.0 - 2.0 * alpha)


def bone_name(value) -> str:
    return str(value)


def load_or_duplicate(source: str, destination: str):
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        return unreal.EditorAssetLibrary.load_asset(destination)
    if not unreal.EditorAssetLibrary.does_asset_exist(source):
        log(f"missing source {source}")
        return None
    unreal.EditorAssetLibrary.make_directory(destination.rsplit("/", 1)[0])
    if not unreal.EditorAssetLibrary.duplicate_asset(source, destination):
        log(f"duplicate failed {source} -> {destination}")
        return None
    return unreal.EditorAssetLibrary.load_asset(destination)


def reference_pose(skeleton):
    return unreal.AnimPoseExtensions.get_reference_pose(skeleton)


def bone_names(pose) -> list[str]:
    names = unreal.AnimPoseExtensions.get_bone_names(pose)
    if isinstance(names, tuple):
        names = names[-1] if names else []
    return [bone_name(name) for name in names]


def world_pose(pose, name: str) -> unreal.Transform:
    return unreal.AnimPoseExtensions.get_bone_pose(pose, name, unreal.AnimPoseSpaces.WORLD)


def pick(names: set[str], *candidates: str) -> str | None:
    for candidate in candidates:
        if candidate in names:
            return candidate
    return None


def parent_map(sequence, names: list[str]) -> dict[str, str | None]:
    parents: dict[str, str | None] = {}
    for name in names:
        path = unreal.AnimationLibrary.find_bone_path_to_root(sequence, name)
        if isinstance(path, tuple):
            path = path[-1] if path else []
        chain = [bone_name(item) for item in path]
        parents[name] = chain[1] if len(chain) > 1 else None
    return parents


def children_of(parents: dict[str, str | None]) -> dict[str, list[str]]:
    children: dict[str, list[str]] = {}
    for bone, parent in parents.items():
        if parent:
            children.setdefault(parent, []).append(bone)
    return children


class Pose:
    def __init__(self, comp: dict[str, unreal.Transform], local: dict[str, unreal.Transform], children, parents):
        self.comp = comp
        self.local = local
        self.children = children
        self.parents = parents

    def recompute(self, bone: str) -> None:
        for child in self.children.get(bone, []):
            self.comp[child] = compose(self.comp[bone], self.local[child])
            self.recompute(child)

    def set_rotation(self, bone: str, rotation: unreal.Quat) -> None:
        current = self.comp[bone]
        self.comp[bone] = xform(current.translation, rotation, current.scale3d)
        self.recompute(bone)

    def add_rotation(self, bone: str, delta: unreal.Quat) -> None:
        self.set_rotation(bone, q_mul(delta, self.comp[bone].rotation))

    def translate(self, bone: str, delta: unreal.Vector) -> None:
        current = self.comp[bone]
        self.comp[bone] = xform(v_add(current.translation, delta), current.rotation, current.scale3d)
        self.recompute(bone)

    def aim(self, bone: str, child: str, target: unreal.Vector) -> float:
        origin = self.comp[bone].translation
        current_dir = q_rot(self.comp[bone].rotation, self.local[child].translation)
        desired = v_sub(target, origin)
        self.set_rotation(bone, q_mul(q_from_to(current_dir, desired), self.comp[bone].rotation))
        return v_len(v_sub(self.comp[child].translation, target))


def solve_knee(hip: unreal.Vector, ankle: unreal.Vector, pole: unreal.Vector, length_a: float, length_b: float) -> unreal.Vector:
    to_ankle = v_sub(ankle, hip)
    distance = v_len(to_ankle)
    max_reach = length_a + length_b - 0.5
    min_reach = abs(length_a - length_b) + 0.5
    distance = max(min_reach, min(max_reach, distance))
    direction = v_norm(to_ankle)
    cos_hip = (length_a * length_a + distance * distance - length_b * length_b) / (2.0 * length_a * distance)
    cos_hip = max(-1.0, min(1.0, cos_hip))
    along = length_a * cos_hip
    height = math.sqrt(max(length_a * length_a - along * along, 0.0))
    pole_dir = v_norm(v_sub(pole, hip))
    side = v_sub(pole_dir, v_mul(direction, v_dot(pole_dir, direction)))
    if v_len(side) < 1.0e-4:
        side = v(1.0, 0.0, 0.0)
    side = v_norm(side)
    return v_add(hip, v_add(v_mul(direction, along), v_mul(side, height)))


def reachable_horiz(vertical: float, leg: float) -> float:
    reach = leg * 0.96
    return math.sqrt(max(reach * reach - vertical * vertical, 0.0))


def backrest_front_y(z: float) -> float:
    z0, y0 = BACKREST_LOW
    z1, y1 = BACKREST_HIGH
    t = (z - z0) / (z1 - z0)
    return y0 + t * (y1 - y0)


def build_sit(ref: Pose, bones: dict[str, str], forward: unreal.Vector, left: unreal.Vector) -> Pose:
    sit = Pose(dict(ref.comp), dict(ref.local), ref.children, ref.parents)
    up = v(0.0, 0.0, 1.0)
    thigh_l = bones["thigh_l"]
    thigh_r = bones["thigh_r"]
    calf_l = bones["calf_l"]
    calf_r = bones["calf_r"]
    foot_l = bones["foot_l"]
    foot_r = bones["foot_r"]

    hip_l = ref.comp[thigh_l].translation
    hip_r = ref.comp[thigh_r].translation
    ankle_l = ref.comp[foot_l].translation
    ankle_r = ref.comp[foot_r].translation
    mid_z = (hip_l.z + hip_r.z) * 0.5
    ankle_z = (ankle_l.z + ankle_r.z) * 0.5
    len_thigh = v_len(v_sub(ref.comp[calf_l].translation, hip_l))
    len_calf = v_len(v_sub(ankle_l, ref.comp[calf_l].translation))
    leg = len_thigh + len_calf

    hip_z = min(SEAT_HIP_Z, ankle_z + leg * 0.78)
    vertical = max(hip_z - ankle_z, 1.0)
    horiz = min(46.0, reachable_horiz(vertical, leg))
    if horiz < 28.0 and hip_z > ankle_z + 20.0:
        hip_z = ankle_z + leg * 0.62
        vertical = hip_z - ankle_z
        horiz = min(46.0, reachable_horiz(vertical, leg))

    delta = v_add(v_mul(forward, -horiz), v(0.0, 0.0, hip_z - mid_z))
    sit.translate(bones["pelvis"], delta)
    log(
        "leg thigh=%.1f calf=%.1f hip_z=%.1f ankle_z=%.1f horiz=%.1f"
        % (len_thigh, len_calf, hip_z, ankle_z, horiz)
    )

    def plant(thigh: str, calf: str, foot: str, ankle: unreal.Vector, side: float) -> None:
        hip = sit.comp[thigh].translation
        # Обычная посадка: бедро почти горизонтально, голень вниз к полу с небольшим выносом.
        floor_z = 8.5
        knee_z = min(hip.z - 4.0, floor_z + math.sqrt(max(len_calf * len_calf - 12.0 * 12.0, 1.0)))
        drop = hip.z - knee_z
        ahead = math.sqrt(max(len_thigh * len_thigh - drop * drop, 4.0))
        knee = v_add(hip, v_add(v_mul(forward, ahead), v_add(v(0.0, 0.0, knee_z - hip.z), v_mul(left, side * 3.0))))
        shin_drop = knee_z - floor_z
        shin_ahead = math.sqrt(max(len_calf * len_calf - shin_drop * shin_drop, 1.0))
        ankle_target = v_add(knee, v_add(v_mul(forward, shin_ahead), v(0.0, 0.0, -shin_drop)))
        err_knee = sit.aim(thigh, calf, knee)
        err_ankle = sit.aim(calf, foot, ankle_target)
        sit.set_rotation(foot, ref.comp[foot].rotation)
        log(
            "%s knee_err=%.2f ankle_err=%.2f knee=%s ankle=%s"
            % (foot, err_knee, err_ankle, fmt(sit.comp[calf].translation), fmt(sit.comp[foot].translation))
        )

    right = v_mul(left, -1.0)
    pelvis_bone = bones["pelvis"]
    head = bones.get("head")
    sign = 1.0
    if head:
        before = sit.comp[head].translation

        def pitched_head(degrees: float) -> unreal.Vector:
            trial = Pose(dict(sit.comp), dict(sit.local), sit.children, sit.parents)
            trial.add_rotation(pelvis_bone, q_axis_angle(right, degrees))
            return trial.comp[head].translation

        back = pitched_head(-12.0)
        forward_head = pitched_head(12.0)
        sign = -1.0 if v_dot(v_sub(back, before), forward) < v_dot(v_sub(forward_head, before), forward) else 1.0
    # Таз откидывается вместе со спиной, иначе корпус остаётся вертикальным перед спинкой.
    sit.add_rotation(pelvis_bone, q_axis_angle(right, sign * PELVIS_RECLINE))

    plant(thigh_l, calf_l, foot_l, ankle_l, 1.0)
    plant(thigh_r, calf_r, foot_r, ankle_r, -1.0)

    spine = bones.get("spine_01")
    if spine:
        sit.add_rotation(spine, q_axis_angle(right, sign * SPINE_RECLINE))
    spine_02 = bones.get("spine_02")
    if spine_02:
        sit.add_rotation(spine_02, q_axis_angle(right, sign * SPINE2_RECLINE))
    neck = bones.get("neck_01")
    if neck:
        sit.add_rotation(neck, q_axis_angle(right, -sign * NECK_COUNTER))
    if head:
        log("recline sign=%.0f head=%s" % (sign, fmt(sit.comp[head].translation)))

    def relax_arm(upper: str, lower: str, hand: str, thigh: str, calf: str, side: float) -> None:
        if upper not in sit.comp or lower not in sit.comp or hand not in sit.comp:
            return
        shoulder = sit.comp[upper].translation
        length_upper = v_len(ref.local[lower].translation)
        length_lower = v_len(ref.local[hand].translation)
        hip = sit.comp[thigh].translation
        knee = sit.comp[calf].translation
        # Кисть на бедре, не на колене: колено дальше вытянутой руки и IK уводил её к шлему.
        target = v_add(v_lerp(hip, knee, 0.62), v(0.0, 0.0, 6.0))
        reach = length_upper + length_lower
        dist = v_len(v_sub(target, shoulder))
        if dist > reach * 0.92:
            target = v_add(shoulder, v_mul(v_norm(v_sub(target, shoulder)), reach * 0.9))
        # Локоть в сторону и вниз, иначе цепь выворачивается вверх.
        pole = v_add(shoulder, v_add(v_mul(left, side * 45.0), v_mul(up, -25.0)))
        elbow = solve_knee(shoulder, target, pole, length_upper, length_lower)
        err_elbow = sit.aim(upper, lower, elbow)
        err_hand = sit.aim(lower, hand, target)
        sit.set_rotation(hand, ref.comp[hand].rotation)
        log(
            "%s shoulder=%s reach=%.1f dist=%.1f elbow_err=%.2f hand_err=%.2f hand=%s"
            % (hand, fmt(shoulder), reach, dist, err_elbow, err_hand, fmt(sit.comp[hand].translation))
        )

    relax_arm(
        bones.get("upperarm_l", ""), bones.get("lowerarm_l", ""), bones.get("hand_l", ""),
        thigh_l, calf_l, 1.0,
    )
    relax_arm(
        bones.get("upperarm_r", ""), bones.get("lowerarm_r", ""), bones.get("hand_r", ""),
        thigh_r, calf_r, -1.0,
    )
    return sit


def sample_written_pose(sequence) -> None:
    try:
        options = unreal.AnimPoseEvaluationOptions()
        options.set_editor_property("evaluation_type", unreal.AnimDataEvalType.SOURCE)
        options.set_editor_property("should_retarget", False)
        result = unreal.AnimPoseExtensions.get_anim_pose_at_frame(sequence, 20, options)
        pose = result[0] if isinstance(result, tuple) else result
        for bone in ("pelvis", "calf_l", "foot_l", "ik_foot_l", "hand_l", "hand_r", "ik_hand_l"):
            transform = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
            if isinstance(transform, tuple):
                transform = transform[0]
            log("sample %s %s" % (bone, fmt(transform.translation)))
    except Exception as exc:
        log("sample failed %s" % exc)


def fmt(vector: unreal.Vector) -> str:
    return "(%.1f, %.1f, %.1f)" % (vector.x, vector.y, vector.z)


def local_of(pose: Pose, bone: str) -> unreal.Transform:
    parent = pose.parents.get(bone)
    if not parent:
        return pose.comp[bone]
    return relative(pose.comp[bone], pose.comp[parent])


def write_sequence(sequence, keyed: dict[str, tuple[unreal.Transform, unreal.Transform]]) -> bool:
    controller = sequence.get_editor_property("controller")
    if controller is None:
        model = sequence.get_editor_property("data_model_interface")
        controller = unreal.new_object(unreal.AnimDataController, sequence)
        controller.set_model(model)
        log("created a new anim data controller")
    if controller is None:
        log("no animation controller")
        return False

    controller.open_bracket(unreal.Text("Crew seat sit"), False)
    controller.set_frame_rate(unreal.FrameRate(FPS, 1), False)
    controller.set_number_of_frames(unreal.FrameNumber(FRAMES), False)
    controller.remove_all_bone_tracks(False)

    key_count = FRAMES + 1
    scale = v(1.0, 1.0, 1.0)
    for bone, (start, end) in keyed.items():
        if not controller.add_bone_curve(bone, False):
            log(f"add_bone_curve failed for {bone}")
            controller.close_bracket(False)
            return False
        positions = []
        rotations = []
        scales = []
        for index in range(key_count):
            alpha = 1.0 if index >= SIT_KEYS else smoothstep(index / float(SIT_KEYS))
            positions.append(v_lerp(start.translation, end.translation, alpha))
            rotations.append(unreal.MathLibrary.quat_slerp(start.rotation, end.rotation, alpha))
            scales.append(scale)
        if not controller.set_bone_track_keys(bone, positions, rotations, scales, False):
            log(f"set_bone_track_keys failed for {bone}")
            controller.close_bracket(False)
            return False
    controller.close_bracket(False)
    sequence.set_editor_property("enable_root_motion", False)
    sequence.set_editor_property("force_root_lock", True)
    model = sequence.get_editor_property("data_model_interface")
    log(
        "sequence frames=%s keys=%s length=%.3f"
        % (model.get_number_of_frames(), model.get_number_of_keys(), sequence.get_play_length())
    )
    return True


def write_montage(montage, sequence) -> None:
    length = sequence.get_play_length()
    tracks = list(montage.get_editor_property("slot_anim_tracks"))
    track = tracks[0]
    track.set_editor_property("slot_name", "DefaultSlot")
    anim_track = track.get_editor_property("anim_track")
    segments = list(anim_track.get_editor_property("anim_segments"))
    segment = segments[0]
    segment.set_editor_property("anim_reference", sequence)
    segment.set_editor_property("anim_start_time", 0.0)
    segment.set_editor_property("anim_end_time", length)
    segment.set_editor_property("anim_play_rate", 1.0)
    segment.set_editor_property("looping_count", 1)
    segments[0] = segment
    anim_track.set_editor_property("anim_segments", segments)
    track.set_editor_property("anim_track", anim_track)
    tracks[0] = track
    montage.set_editor_property("slot_anim_tracks", tracks)
    unreal.AnimationLibrary.remove_all_animation_notify_tracks(montage)
    montage.set_editor_property("enable_auto_blend_out", False)

    section_count = montage.get_num_sections()
    section_names = [str(montage.get_section_name(index)) for index in range(section_count)]
    log("montage sections=%s" % section_names)

    controller = montage.get_editor_property("controller")
    if controller is not None:
        controller.set_frame_rate(unreal.FrameRate(FPS, 1), False)
        controller.set_number_of_frames(unreal.FrameNumber(FRAMES), False)
    log("montage play_length=%.3f slot=DefaultSlot" % montage.get_play_length())


def assign_blueprint(montage, anchor: unreal.Vector, release: unreal.Vector, yaw: float) -> None:
    blueprint = unreal.EditorAssetLibrary.load_asset(BP_PATH)
    if not blueprint:
        log(f"missing {BP_PATH}")
        return
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated = unreal.load_object(None, f"{BP_PATH}.BP_CrewSeat_C")
    cdo = unreal.get_default_object(generated)
    cdo.set_editor_property("character_use_montage", montage)
    cdo.set_editor_property("release_offset", release)
    use_anchor = cdo.get_editor_property("use_anchor")
    if use_anchor:
        use_anchor.set_editor_property("relative_location", anchor)
        # Rotator пишем напрямую. Rotator.quaternion() в этом UE кладёт yaw в pitch.
        use_anchor.set_editor_property(
            "relative_rotation", unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0)
        )
        saved = use_anchor.get_editor_property("relative_rotation")
        log(
            "anchor rot pitch=%.1f yaw=%.1f roll=%.1f"
            % (saved.pitch, saved.yaw, saved.roll)
        )
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    log("blueprint anchor=%s release=%s yaw=%.1f" % (fmt(anchor), fmt(release), yaw))


def main() -> None:
    log("start")
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if not mesh:
        log(f"missing {MESH_PATH}")
        return
    skeleton = mesh.get_editor_property("skeleton")
    pose = reference_pose(skeleton)
    names = bone_names(pose)
    if not names:
        log("reference pose has no bones")
        return
    name_set = set(names)

    bones = {
        "pelvis": pick(name_set, "pelvis"),
        "spine_01": pick(name_set, "spine_01"),
        "spine_02": pick(name_set, "spine_02"),
        "neck_01": pick(name_set, "neck_01", "neck_02"),
        "head": pick(name_set, "head"),
        "thigh_l": pick(name_set, "thigh_l"),
        "calf_l": pick(name_set, "calf_l"),
        "foot_l": pick(name_set, "foot_l"),
        "thigh_r": pick(name_set, "thigh_r"),
        "calf_r": pick(name_set, "calf_r"),
        "foot_r": pick(name_set, "foot_r"),
        "upperarm_l": pick(name_set, "upperarm_l"),
        "lowerarm_l": pick(name_set, "lowerarm_l"),
        "hand_l": pick(name_set, "hand_l"),
        "upperarm_r": pick(name_set, "upperarm_r"),
        "lowerarm_r": pick(name_set, "lowerarm_r"),
        "hand_r": pick(name_set, "hand_r"),
    }
    required = ("pelvis", "thigh_l", "calf_l", "foot_l", "thigh_r", "calf_r", "foot_r")
    missing = [key for key in required if not bones[key]]
    if missing:
        log("missing bones %s" % missing)
        log("sample %s" % names[:40])
        return
    bones = {key: value for key, value in bones.items() if value}

    sequence = load_or_duplicate(SEQ_SRC, SEQ_PATH)
    if sequence is None or not isinstance(sequence, unreal.AnimSequence):
        log("sit sequence is not an AnimSequence (%s)" % (type(sequence).__name__ if sequence else "None"))
        return

    parents = parent_map(sequence, names)
    if parents.get(bones["thigh_l"]) is None:
        log("bone parents were not resolved")
        return

    comp = {name: world_pose(pose, name) for name in names}
    sample = bones["thigh_l"]
    sample_parent = parents[sample]
    error = choose_transform_convention(comp[sample_parent], comp[sample])
    log("compose check %.3f (thigh parent %s)" % (error, sample_parent))
    if error > 1.0:
        log("could not match bone parent * local, aborting")
        return
    local = {}
    for name in names:
        parent = parents.get(name)
        local[name] = comp[name] if not parent else relative(comp[name], comp[parent])

    ref = Pose(comp, local, children_of(parents), parents)
    pelvis = comp[bones["pelvis"]].translation
    foot_l = comp[bones["foot_l"]].translation
    foot_r = comp[bones["foot_r"]].translation
    # Манекен смотрит по +Y. Сустав головы в опорной позе позади таза, по нему направление не брать.
    forward = v(0.0, 1.0, 0.0)
    up = v(0.0, 0.0, 1.0)
    left = v_norm(v_cross(forward, up))
    if v_dot(v_sub(foot_l, foot_r), left) < 0.0:
        forward = v_mul(forward, -1.0)
        left = v_mul(left, -1.0)
    log("forward=%s left=%s pelvis=%s" % (fmt(forward), fmt(left), fmt(pelvis)))

    sit = build_sit(ref, bones, forward, left)

    # Foot IK манекена (PBIK) тянет ноги к ik_foot_*. Без ключей эти кости остаются
    # в стойке, и голень на экране не двигается, как бы ни ставили calf.
    for ik_name, src_key in (
        ("ik_foot_l", "foot_l"),
        ("ik_foot_r", "foot_r"),
        ("ik_hand_l", "hand_l"),
        ("ik_hand_r", "hand_r"),
    ):
        ik = pick(name_set, ik_name)
        src = bones.get(src_key)
        if not ik or not src or ik not in sit.comp or src not in sit.comp:
            log("ik snap skipped %s" % ik_name)
            continue
        src_xf = sit.comp[src]
        sit.comp[ik] = xform(src_xf.translation, src_xf.rotation, src_xf.scale3d)
        bones[ik_name] = ik
        log("ik snap %s parent=%s pos=%s" % (ik, parents.get(ik), fmt(sit.comp[ik].translation)))

    keyed_names = [
        bones["pelvis"],
        bones["thigh_l"],
        bones["calf_l"],
        bones["foot_l"],
        bones["thigh_r"],
        bones["calf_r"],
        bones["foot_r"],
    ]
    for key in (
        "spine_01", "spine_02", "neck_01",
        "upperarm_l", "lowerarm_l", "hand_l",
        "upperarm_r", "lowerarm_r", "hand_r",
        "ik_foot_l", "ik_foot_r", "ik_hand_l", "ik_hand_r",
    ):
        if bones.get(key) and bones[key] not in keyed_names:
            keyed_names.append(bones[key])

    keyed = {}
    for name in keyed_names:
        keyed[name] = (local_of(ref, name), local_of(sit, name))

    if not write_sequence(sequence, keyed):
        return
    sample_written_pose(sequence)
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)

    hip = sit.comp[bones["thigh_l"]].translation
    hip_r = sit.comp[bones["thigh_r"]].translation
    hip_mid_x = (hip.x + hip_r.x) * 0.5
    hip_mid_y = (hip.y + hip_r.y) * 0.5
    contact_name = bones.get("spine_02") or bones.get("spine_01") or bones["pelvis"]
    contact = sit.comp[contact_name].translation
    seat_hip_y = SEAT_HIP_LOCKED_Y
    # Якорь yaw +90: точка меша (mx, my, mz) -> оборудование anchor + (mx, my, mz-96).
    anchor = v(SEAT_HIP_X - hip_mid_x, seat_hip_y - hip_mid_y, 96.0)
    back_y = seat_hip_y + (contact.y - hip_mid_y) - TORSO_BACK
    log(
        "back contact z=%.1f surface=%.1f backrest=%.1f seat_hip_y=%.1f"
        % (contact.z, back_y, backrest_front_y(contact.z), seat_hip_y)
    )
    foot_mid_y = (foot_l.y + foot_r.y) * 0.5
    foot_equip_y = anchor.y + foot_mid_y
    release = v(100.0, 0.0, 0.0)
    log(
        "hip mesh=%s foot mesh y=%.1f anchor=%s foot_equip_y=%.1f"
        % (fmt(hip), foot_mid_y, fmt(anchor), foot_equip_y)
    )

    montage = load_or_duplicate(MONTAGE_SRC, MONTAGE_PATH)
    if montage is None or not isinstance(montage, unreal.AnimMontage):
        log("sit montage is not an AnimMontage (%s)" % (type(montage).__name__ if montage else "None"))
        return
    write_montage(montage, sequence)
    unreal.EditorAssetLibrary.save_loaded_asset(montage)
    assign_blueprint(montage, anchor, release, ANCHOR_YAW)
    log("done")


if __name__ == "__main__":
    main()
