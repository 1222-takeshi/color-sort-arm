# -*- coding: utf-8 -*-
"""
6軸色分けアーム 部品生成スクリプト (CadQuery)
単位はすべて mm。パラメータを変えて再実行すれば STEP/STL が再生成される。
出力: cad/out/*.step, cad/out/*.stl

注意: これは設計たたき台。MG946R のポケット寸法は実物で ±0.3mm 調整すること。
"""
import os
import cadquery as cq

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
os.makedirs(OUT, exist_ok=True)

# ---------------- パラメータ ----------------
# MG946R 本体 (実測前提の公称値)
MG_BODY_L = 40.7      # 本体長さ
MG_BODY_W = 19.7      # 本体幅
MG_POCKET_L = 41.5    # ポケット長さ (公差 +0.8)
MG_POCKET_W = 20.5    # ポケット幅 (公差 +0.8)
MG_POCKET_D = 33.0    # ポケット深さ (フランジ耳より下)
WALL = 3.0            # ポケット周りの肉厚
HORN_BC_R = 9.0       # ホーン取付ねじの円半径 (M3想定・要実測)
HORN_HOLE_R = 1.6     # ねじ下穴半径
CENTER_HOLE_R = 2.6   # 出力軸センターホール半径

# リンク長
L1 = 85.0             # 肩->肘
L2 = 75.0             # 肘->手首ピッチ
LINK_W = 34.0         # リンク幅
LINK_T = 6.0          # リンク板厚

# 9G (SG90系)
SG_POCKET_L = 23.2
SG_POCKET_W = 12.6

# Adventurer 3 制約
BUILD_MAX = 140.0     # 一辺の上限 (マージン込み)


def box(x, y, z, cx=0.0, cy=0.0, cz=0.0):
    """中心 (cx,cy,cz) の直方体 Workplane"""
    return cq.Workplane("XY").box(x, y, z).translate((cx, cy, cz))


def cyl(r, h, cx=0.0, cy=0.0, cz=0.0, axis="Z"):
    w = cq.Workplane("XY").circle(r).extrude(h)
    if axis == "Z":
        return w.translate((cx, cy, cz - h / 2.0))
    if axis == "X":
        w = w.rotate((0, 0, 0), (0, 1, 0), 90)
        return w.translate((cx - 0, cy, cz))  # 回転後は原点基準が変わるため呼び出し側で調整
    raise ValueError(axis)


def hole_z(part, r, x, y, z0=-60.0, depth=120.0):
    cutter = cq.Workplane("XY").circle(r).extrude(depth).translate((x, y, z0))
    return part.cut(cutter)


def horn_pattern(part, cx=0.0, cy=0.0):
    """ホーン取付 4穴 + センター穴 (Z方向)"""
    import math
    for i in range(4):
        a = math.radians(45 + 90 * i)
        part = hole_z(part, HORN_HOLE_R, cx + HORN_BC_R * math.cos(a), cy + HORN_BC_R * math.sin(a))
    part = hole_z(part, CENTER_HOLE_R, cx, cy)
    return part


def servo_block(pocket_axis="Z"):
    """MG946R を受けるブロック (外寸は ポケット+壁)。
    pocket_axis='Z': ポケットが Z 方向に開く (出力軸が Z)
    戻り値は原点=ポケット中心・ブロック中心の Workplane
    """
    bw = MG_POCKET_L + 2 * WALL   # X
    bd = MG_POCKET_W + 2 * WALL   # Y
    bh = MG_POCKET_D + WALL       # Z (底付き)
    blk = box(bw, bd, bh)
    if pocket_axis == "Z":
        pocket = box(MG_POCKET_L, MG_POCKET_W, MG_POCKET_D + 1.0, 0, 0, WALL / 2.0 + 0.5)
        blk = blk.cut(pocket)
    return blk


# ---------------- 部品 1: ベース台 ----------------
def base_plate():
    p = cq.Workplane("XY").circle(65).extrude(8)              # Ø130 x 8
    p = p.translate((0, 0, -4))
    for (x, y) in [(55, 0), (-55, 0), (0, 55), (0, -55)]:      # 机固定穴 M3
        p = hole_z(p, 1.7, x, y)
    p = hole_z(p, 20, 0, 0)                                    # 中央の配線通し穴 Ø40
    return p


# ---------------- 部品 2: 第1軸ホルダ (MG946R 縦置き) ----------------
def j1_holder():
    blk = servo_block("Z")                                     # 47.5 x 26.5 x 36
    blk = blk.translate((0, 0, 18))                            # 底面 z=0
    base = box(70, 60, 4, 0, 0, 2)                             # 取付フランジ
    p = base.union(blk)
    for (x, y) in [(30, 25), (-30, 25), (30, -25), (-30, -25)]:
        p = hole_z(p, 1.7, x, y)
    return p


# ---------------- 部品 3: 回転プレート (第1軸ホーン接続) ----------------
def rot_plate():
    p = cq.Workplane("XY").circle(42).extrude(6)
    p = p.translate((0, 0, -3))
    p = horn_pattern(p, 0, 0)
    return p


# ---------------- 部品 4/5: リンク (肩->肘, 肘->手首) ----------------
def link(length):
    # 主板: 両端半円の板 (box + 両端シリンダ)
    plate = box(length, LINK_W, LINK_T, length / 2.0, 0, 0)
    end0 = cyl(LINK_W / 2.0, LINK_T, 0, 0, 0)
    end1 = cyl(LINK_W / 2.0, LINK_T, length, 0, 0)
    p = plate.union(end0).union(end1)
    # 根元ボス (ホーン接続)
    boss = cyl(13, 10, 0, 0, 2)
    p = p.union(boss)
    p = horn_pattern(p, 0, 0)
    # 先端: 次サーボのブロック (出力軸は Z = 自分の関節軸と平行)
    blk = servo_block("Z")
    blk = blk.translate((length, 0, LINK_T / 2.0 + (MG_POCKET_D + WALL) / 2.0 - 6))
    p = p.union(blk)
    return p


# ---------------- 部品 6: 手首ブラケット (J4->J5 ロール) ----------------
def wrist_bracket():
    # J4 ホーンに付く板 + J5 サーボを X 軸向きに受けるブロック
    plate = box(30, 34, 6, 0, 0, 0)
    plate = horn_pattern(plate, 0, 0)
    blk = servo_block("Z")
    blk = blk.rotate((0, 0, 0), (0, 1, 0), 90)                 # ポケット開口を X 方向へ
    blk = blk.translate((28, 0, 0))
    p = plate.union(blk)
    return p


# ---------------- 部品 7: グリッパー基部 (9G搭載) ----------------
def gripper_base():
    p = box(44, 34, 5, 0, 0, 0)
    p = horn_pattern(p, -12, 0)                                # J5 ホーン接続 (片寄せ)
    pocket = box(SG_POCKET_L, SG_POCKET_W, 6, 8, 0, 0)         # 9G ポケット (貫通)
    p = p.cut(pocket)
    for (x, y) in [(18, 13), (18, -13)]:                       # 固定指の取付穴
        p = hole_z(p, 1.7, x, y)
    return p


# ---------------- 部品 8: グリッパー指 ----------------
def gripper_finger():
    arm = box(38, 8, 5, 19, 0, 0)
    tip = box(6, 8, 16, 38, 0, -5)                             # 下向きの爪
    p = arm.union(tip)
    p = hole_z(p, 1.7, 4, 0)                                   # 根元ねじ穴
    p = hole_z(p, 2.2, 12, 0)                                  # サーボホーン/リンク用
    return p


# ---------------- 部品 9: カメラ雲台ベース (9G パン) ----------------
def cam_pan_base():
    p = cq.Workplane("XY").circle(30).extrude(6)
    p = p.translate((0, 0, -3))
    pocket = box(SG_POCKET_L, SG_POCKET_W, 7, 0, 0, 0)
    p = p.cut(pocket)
    for (x, y) in [(24, 0), (-24, 0), (0, 24), (0, -24)]:
        p = hole_z(p, 1.7, x, y)
    return p


# ---------------- 部品 10: カメラチルトアーム ----------------
def cam_tilt_arm():
    p = box(8, 14, 56, 0, 0, 28)
    p = p.cut(cq.Workplane("XY").circle(2.6).extrude(60).translate((0, 0, -2)))       # 下端: パン軸
    tilt = cq.Workplane("XY").circle(2.2).extrude(20).rotate((0, 0, 0), (1, 0, 0), 90)
    p = p.cut(tilt.translate((0, -10, 50)))                                            # 上端: チルト軸穴 (Y方向)
    return p


# ---------------- 部品 11: ESP32-CAM ホルダ ----------------
def cam_holder():
    back = box(46, 3, 36, 0, 0, 18)                            # 背板
    lip = box(46, 6, 4, 0, 1.5, 2)                             # 下端の受け
    p = back.union(lip)
    p = p.cut(box(30, 4, 12, 0, 0, 26))                        # レンズ窓 (上部中央)
    p = hole_z(p, 2.2, 0, 0) if False else p                   # (ダミー分岐は使わない)
    # チルト軸ねじ穴 (Y方向) は Fusion 側で追加しやすいよう背板下部に目印の浅穴
    mark = cq.Workplane("XY").circle(2.2).extrude(2).rotate((0, 0, 0), (1, 0, 0), 90)
    p = p.cut(mark.translate((0, 2.5, 6)))
    return p


PARTS = {
    "01_base_plate": base_plate,
    "02_j1_holder": j1_holder,
    "03_rot_plate": rot_plate,
    "04_link_L1_85": lambda: link(L1),
    "05_link_L2_75": lambda: link(L2),
    "06_wrist_bracket": wrist_bracket,
    "07_gripper_base": gripper_base,
    "08_gripper_finger": gripper_finger,
    "09_cam_pan_base": cam_pan_base,
    "10_cam_tilt_arm": cam_tilt_arm,
    "11_cam_holder": cam_holder,
}

if __name__ == "__main__":
    for name, fn in PARTS.items():
        part = fn()
        shape = part.val()
        bb = shape.BoundingBox()
        dx, dy, dz = bb.xlen, bb.ylen, bb.zlen
        warn = ""
        if max(dx, dy, dz) > BUILD_MAX:
            warn = "  <-- WARNING: Adventurer3 造形範囲超過"
        cq.exporters.export(part, os.path.join(OUT, name + ".step"))
        cq.exporters.export(part, os.path.join(OUT, name + ".stl"))
        print(f"{name}: {dx:.1f} x {dy:.1f} x {dz:.1f} mm, vol={shape.Volume()/1000.0:.1f} cm3{warn}")
    print("done ->", OUT)
