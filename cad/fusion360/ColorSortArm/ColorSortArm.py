# -*- coding: utf-8 -*-
"""
6軸色分けアーム 部品生成スクリプト (Fusion 360)
使い方: Fusion 360 で [ユーティリティ] -> [スクリプトとアドイン] ->
        このフォルダ (ColorSortArm) を追加して実行する。
        各部品がコンポーネントとして生成され、タイムライン上で手直しできる。

単位: パラメータは mm で書く (API 内部は cm のため内部で変換)。
注意: 設計たたき台。MG946R ポケット寸法は実物で ±0.3mm 調整すること。
      このスクリプトは Fusion 実機未検証のため、エラー時はメッセージ内容を
      控えて共有してもらえると修正版を出せる。
"""
import math
import traceback

import adsk.core
import adsk.fusion

# ---------------- パラメータ (mm) ----------------
MG_POCKET_L = 41.5
MG_POCKET_W = 20.5
MG_POCKET_D = 33.0
WALL = 3.0
HORN_BC_R = 9.0
HORN_HOLE_R = 1.6
CENTER_HOLE_R = 2.6
L1 = 85.0
L2 = 75.0
LINK_W = 34.0
LINK_T = 6.0
SG_POCKET_L = 23.2
SG_POCKET_W = 12.6


def v(mm):
    """mm -> cm (Fusion API の内部単位)"""
    return mm / 10.0


def new_component(design, name):
    root = design.rootComponent
    occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
    comp = occ.component
    comp.name = name
    return comp


def _finish_extrude(comp, sketch, dist_mm, operation, start_mm=0.0, symmetric=False):
    extrudes = comp.features.extrudeFeatures
    prof = sketch.profiles.item(0)
    ein = extrudes.createInput(prof, operation)
    if start_mm != 0.0:
        ein.startExtent = adsk.fusion.OffsetStartDefinition.create(
            adsk.core.ValueInput.createByReal(v(start_mm)))
    ein.setDistanceExtent(symmetric, adsk.core.ValueInput.createByReal(v(dist_mm)))
    return extrudes.add(ein)


def add_box(comp, cx, cy, cz, sx, sy, sz, operation=None):
    """中心 (cx,cy) ・底面 z=cz の直方体を XY スケッチ+押し出しで作る"""
    if operation is None:
        operation = adsk.fusion.FeatureOperations.NewBodyFeatureOperation
    sk = comp.sketches.add(comp.xYConstructionPlane)
    sk.sketchCurves.sketchLines.addTwoPointRectangle(
        adsk.core.Point3D.create(v(cx - sx / 2.0), v(cy - sy / 2.0), 0),
        adsk.core.Point3D.create(v(cx + sx / 2.0), v(cy + sy / 2.0), 0))
    return _finish_extrude(comp, sk, sz, operation, start_mm=cz)


def add_cyl(comp, cx, cy, cz, r, h, operation=None):
    """底面 z=cz の円柱 (Z軸)"""
    if operation is None:
        operation = adsk.fusion.FeatureOperations.NewBodyFeatureOperation
    sk = comp.sketches.add(comp.xYConstructionPlane)
    sk.sketchCurves.sketchCircles.addByCenterRadius(
        adsk.core.Point3D.create(v(cx), v(cy), 0), v(r))
    return _finish_extrude(comp, sk, h, operation, start_mm=cz)


JOIN = adsk.fusion.FeatureOperations.JoinFeatureOperation
CUT = adsk.fusion.FeatureOperations.CutFeatureOperation


def cut_hole_z(comp, cx, cy, r, z_from=-20.0, depth=80.0):
    add_cyl(comp, cx, cy, z_from, r, depth, CUT)


def horn_pattern(comp, cx, cy):
    for i in range(4):
        a = math.radians(45 + 90 * i)
        cut_hole_z(comp, cx + HORN_BC_R * math.cos(a), cy + HORN_BC_R * math.sin(a), HORN_HOLE_R)
    cut_hole_z(comp, cx, cy, CENTER_HOLE_R)


def servo_block(comp, cz):
    """MG946R ブロック (底面 z=cz, ポケットは上面から開口)。外寸 47.5x26.5x36"""
    bw, bd, bh = MG_POCKET_L + 2 * WALL, MG_POCKET_W + 2 * WALL, MG_POCKET_D + WALL
    add_box(comp, 0, 0, cz, bw, bd, bh, JOIN)
    add_box(comp, 0, 0, cz + WALL, MG_POCKET_L, MG_POCKET_W, MG_POCKET_D + 5, CUT)
    return bh


# ---------------- 部品 ----------------
def base_plate(design):
    c = new_component(design, "01_base_plate")
    add_cyl(c, 0, 0, -4, 65, 8)
    for (x, y) in [(55, 0), (-55, 0), (0, 55), (0, -55)]:
        cut_hole_z(c, x, y, 1.7)
    cut_hole_z(c, 0, 0, 20)


def j1_holder(design):
    c = new_component(design, "02_j1_holder")
    add_box(c, 0, 0, 0, 70, 60, 4)                       # フランジ
    for (x, y) in [(30, 25), (-30, 25), (30, -25), (-30, -25)]:
        cut_hole_z(c, x, y, 1.7)
    # ブロックはフランジ上面 (z=4) から
    sk_blk = servo_block(c, 4)
    return c


def rot_plate(design):
    c = new_component(design, "03_rot_plate")
    add_cyl(c, 0, 0, -3, 42, 6)
    horn_pattern(c, 0, 0)


def link(design, name, length):
    c = new_component(design, name)
    add_box(c, length / 2.0, 0, -LINK_T / 2.0, length, LINK_W, LINK_T)   # 主板
    add_cyl(c, 0, 0, -LINK_T / 2.0, LINK_W / 2.0, LINK_T, JOIN)          # 根元端
    add_cyl(c, length, 0, -LINK_T / 2.0, LINK_W / 2.0, LINK_T, JOIN)     # 先端
    add_cyl(c, 0, 0, -3, 13, 10, JOIN)                                   # ボス
    horn_pattern(c, 0, 0)
    # 先端サーボブロック (中心を x=length, 底面 z=-3)
    # 一旦 XY 原点で作ってから移動はできないため、ブロック部品を相対配置する簡易版:
    # ブロック外形だけをオフセット位置に作る
    bw, bd, bh = MG_POCKET_L + 2 * WALL, MG_POCKET_W + 2 * WALL, MG_POCKET_D + WALL
    add_box(c, length, 0, -3, bw, bd, bh, JOIN)
    add_box(c, length, 0, -3 + WALL, MG_POCKET_L, MG_POCKET_W, MG_POCKET_D + 5, CUT)
    return c


def wrist_bracket(design):
    c = new_component(design, "06_wrist_bracket")
    add_box(c, 0, 0, -3, 30, 34, 6)
    horn_pattern(c, 0, 0)
    # J5 ブロック: ポケット開口を X+ 方向へ (YZ 平面スケッチで押し出し)
    bw, bd, bh = MG_POCKET_L + 2 * WALL, MG_POCKET_W + 2 * WALL, MG_POCKET_D + WALL
    sk = c.sketches.add(c.yZConstructionPlane)
    sk.sketchCurves.sketchLines.addTwoPointRectangle(
        adsk.core.Point3D.create(v(-bd / 2.0), v(-bh / 2.0), 0),
        adsk.core.Point3D.create(v(bd / 2.0), v(bh / 2.0), 0))
    extrudes = c.features.extrudeFeatures
    ein = extrudes.createInput(sk.profiles.item(0), JOIN)
    ein.startExtent = adsk.fusion.OffsetStartDefinition.create(
        adsk.core.ValueInput.createByReal(v(15)))
    ein.setDistanceExtent(False, adsk.core.ValueInput.createByReal(v(bw)))
    extrudes.add(ein)
    # ポケット (X方向に貫通気味に切る)
    sk2 = c.sketches.add(c.yZConstructionPlane)
    sk2.sketchCurves.sketchLines.addTwoPointRectangle(
        adsk.core.Point3D.create(v(-MG_POCKET_W / 2.0), v(-MG_POCKET_D / 2.0), 0),
        adsk.core.Point3D.create(v(MG_POCKET_W / 2.0), v(MG_POCKET_D / 2.0), 0))
    ein2 = extrudes.createInput(sk2.profiles.item(0), CUT)
    ein2.startExtent = adsk.fusion.OffsetStartDefinition.create(
        adsk.core.ValueInput.createByReal(v(15 + WALL)))
    ein2.setDistanceExtent(False, adsk.core.ValueInput.createByReal(v(MG_POCKET_L + 5)))
    extrudes.add(ein2)
    return c


def gripper_base(design):
    c = new_component(design, "07_gripper_base")
    add_box(c, 0, 0, -2.5, 44, 34, 5)
    horn_pattern(c, -12, 0)
    add_box(c, 8, 0, -3, SG_POCKET_L, SG_POCKET_W, 6, CUT)
    for (x, y) in [(18, 13), (18, -13)]:
        cut_hole_z(c, x, y, 1.7)


def gripper_finger(design):
    c = new_component(design, "08_gripper_finger")
    add_box(c, 19, 0, -2.5, 38, 8, 5)
    add_box(c, 38, 0, -13, 6, 8, 16, JOIN)
    cut_hole_z(c, 4, 0, 1.7)
    cut_hole_z(c, 12, 0, 2.2)


def cam_pan_base(design):
    c = new_component(design, "09_cam_pan_base")
    add_cyl(c, 0, 0, -3, 30, 6)
    add_box(c, 0, 0, -3.5, SG_POCKET_L, SG_POCKET_W, 7, CUT)
    for (x, y) in [(24, 0), (-24, 0), (0, 24), (0, -24)]:
        cut_hole_z(c, x, y, 1.7)


def cam_tilt_arm(design):
    c = new_component(design, "10_cam_tilt_arm")
    add_box(c, 0, 0, 0, 8, 14, 56)
    cut_hole_z(c, 0, 0, 2.6)                                  # 下端パン軸
    # 上端チルト軸穴 (Y方向): XZ 平面スケッチ
    sk = c.sketches.add(c.xZConstructionPlane)
    sk.sketchCurves.sketchCircles.addByCenterRadius(
        adsk.core.Point3D.create(0, v(50), 0), v(2.2))
    extrudes = c.features.extrudeFeatures
    ein = extrudes.createInput(sk.profiles.item(0), CUT)
    ein.setDistanceExtent(True, adsk.core.ValueInput.createByReal(v(30)))
    extrudes.add(ein)


def cam_holder(design):
    c = new_component(design, "11_cam_holder")
    add_box(c, 0, 0, 0, 46, 3, 36)                            # 背板 (y=0中心)
    add_box(c, 0, 1.5, 0, 46, 6, 4, JOIN)                     # 下端の受け
    add_box(c, 0, 0, 20, 30, 4, 12, CUT)                      # レンズ窓


def run(context):
    app = adsk.core.Application.get()
    ui = app.userInterface
    try:
        design = adsk.fusion.Design.cast(app.activeProduct)
        if not design:
            design = adsk.fusion.Design.cast(app.documents.add(
                adsk.core.DocumentTypes.FusionDesignDocumentType).products.item(0))
        design.designType = adsk.fusion.DesignTypes.ParametricDesignType

        base_plate(design)
        j1_holder(design)
        rot_plate(design)
        link(design, "04_link_L1_85", L1)
        link(design, "05_link_L2_75", L2)
        wrist_bracket(design)
        gripper_base(design)
        gripper_finger(design)
        cam_pan_base(design)
        cam_tilt_arm(design)
        cam_holder(design)

        ui.messageBox("ColorSortArm: 11 components created.")
    except Exception:
        ui.messageBox("Failed:\n" + traceback.format_exc())
