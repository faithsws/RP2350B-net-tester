#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测序算法验证（基于 PAIR_SCAN 三维电压）

算法:
  1. 解析 log → V[H][L][MUX]（单位 V）
  2. 对每个 H 通道:
       - 依次取 L != H，且仅用 MUX==L 的采样电压 Vmux
       - R_L = 7.4 / Vmux - 2  （单位 kΩ）
       - mean_R = 各 R_L 的均值
  3. 用 mean_R 匹配对端标识电阻（±0.2kΩ）→ 得到 H 映射到的对端通道

对端标识电阻:
  ch0=0.5k  ch1=1.0k  ch2=1.5k  ch3=2.0k
  ch4=2.5k  ch5=3.0k  ch6=3.9k  ch7=4.7k

用法:
  python seq_scan_analyze.py
  python seq_scan_analyze.py --log test-logs/测序1.log
"""

from __future__ import annotations

import argparse
import math
import re
from pathlib import Path
from typing import List, Optional, Tuple

import numpy as np

N_CH = 8
V_FORMULA = 7.4          # R = 7.4/Vmux - 2
R_OFFSET = 2.0
R_TOL_K = 0.2            # ±0.2 kΩ

# 对端标识电阻 (kΩ)，下标=对端通道号
ID_R_K = np.array([0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.9, 4.7], dtype=np.float64)

LINE_RE = re.compile(
    r"H=Y(?P<h>\d+)\s+L=Y(?P<l>\d+)\s+MUX=Y(?P<m>\d+).*?V=(?P<v>[0-9.]+)V"
)


def parse_log_to_voltage_cube(path: Path) -> Tuple[np.ndarray, int]:
    """解析 PAIR_SCAN 日志为 V[h,l,m]，h==l 为 NaN。"""
    V = np.full((N_CH, N_CH, N_CH), np.nan, dtype=np.float64)
    text = path.read_text(encoding="utf-8", errors="ignore")
    n = 0
    for line in text.splitlines():
        m = LINE_RE.search(line)
        if not m:
            continue
        h = int(m.group("h"))
        l = int(m.group("l"))
        mux = int(m.group("m"))
        v = float(m.group("v"))
        if not (0 <= h < N_CH and 0 <= l < N_CH and 0 <= mux < N_CH):
            continue
        if h == l:
            continue
        V[h, l, mux] = v
        n += 1
    return V, n


def calc_r_kohm(vmux: float) -> Optional[float]:
    """R = 7.4/Vmux - 2，Vmux 无效时返回 None。"""
    if vmux is None or (isinstance(vmux, float) and math.isnan(vmux)):
        return None
    if vmux <= 0.05:
        return None
    return V_FORMULA / vmux - R_OFFSET


def match_id_channel(mean_r: float, tol: float = R_TOL_K) -> Optional[int]:
    """按 ±tol 匹配标识电阻，返回对端通道号；无匹配返回 None。"""
    hits = [i for i, rk in enumerate(ID_R_K) if abs(mean_r - rk) <= tol]
    if not hits:
        return None
    if len(hits) == 1:
        return hits[0]
    # 多命中时取距离最近者
    return min(hits, key=lambda i: abs(mean_r - ID_R_K[i]))


def analyze_sequence(V: np.ndarray) -> dict:
    """
    测序判定。

    返回:
      r_per_h: list[list]，r_per_h[h][l] = 该 (H,L==MUX) 算出的 R（kΩ），无效为 nan
      mean_r:  shape (8,) 每个 H 的均值 R
      map_to:  shape (8,) 每个本端 H 映射到的对端通道，-1 表示未匹配
    """
    r_per_h = np.full((N_CH, N_CH), np.nan, dtype=np.float64)
    mean_r = np.full(N_CH, np.nan, dtype=np.float64)
    map_to = np.full(N_CH, -1, dtype=np.int32)

    for h in range(N_CH):
        rs: List[float] = []
        for l in range(N_CH):
            if l == h:
                continue
            # 只取 MUX == N(L) 通道
            vmux = V[h, l, l]
            r = calc_r_kohm(float(vmux) if not math.isnan(vmux) else float("nan"))
            if r is None:
                continue
            r_per_h[h, l] = r
            rs.append(r)

        if not rs:
            continue
        mean_r[h] = float(np.mean(rs))
        mid = match_id_channel(mean_r[h])
        if mid is not None:
            map_to[h] = mid

    return {
        "r_per_h": r_per_h,
        "mean_r": mean_r,
        "map_to": map_to,
    }


def print_report(path: Path, V: np.ndarray, n: int, result: dict) -> None:
    r_per_h = result["r_per_h"]
    mean_r = result["mean_r"]
    map_to = result["map_to"]

    print("=" * 64)
    print(f"测序分析: {path}")
    print(f"样本数: {n}  (期望 8*7*8=448)")
    print(f"公式: R = {V_FORMULA}/Vmux - {R_OFFSET}  (kΩ)")
    print(f"标识电阻(kΩ): {', '.join(f'ch{i}={ID_R_K[i]:g}' for i in range(N_CH))}")
    print(f"容差: ±{R_TOL_K} kΩ")
    print("=" * 64)

    for h in range(N_CH):
        print(f"\n--- H=Y{h} ---")
        print(f"{'L(=MUX)':>8}  {'Vmux(V)':>8}  {'R(kΩ)':>8}")
        for l in range(N_CH):
            if l == h:
                continue
            vmux = V[h, l, l]
            r = r_per_h[h, l]
            v_s = f"{vmux:8.3f}" if not math.isnan(vmux) else f"{'NaN':>8}"
            r_s = f"{r:8.3f}" if not math.isnan(r) else f"{'NaN':>8}"
            print(f"  Y{l}        {v_s}  {r_s}")

        mr = mean_r[h]
        mid = int(map_to[h])
        if math.isnan(mr):
            print("  均值 R:  (无有效样本)")
            print("  判定:    未匹配")
            continue

        id_r = ID_R_K[mid] if mid >= 0 else float("nan")
        if mid >= 0:
            delta = mr - id_r
            print(f"  均值 R:  {mr:.3f} kΩ")
            print(f"  判定:    对端通道 = ch{mid} "
                  f"(标识 {id_r:g}kΩ, Δ={delta:+.3f}kΩ)")
        else:
            print(f"  均值 R:  {mr:.3f} kΩ")
            print("  判定:    未落入任一标识电阻 ±0.2kΩ")

    print("\n" + "=" * 64)
    print("线序映射汇总 (本端 H → 对端通道)")
    print("-" * 64)
    ok = 0
    for h in range(N_CH):
        mid = int(map_to[h])
        mr = mean_r[h]
        if mid < 0:
            print(f"  本端 ch{h}  →  ?     (mean_R={mr:.3f})")
        else:
            mark = "直通" if mid == h else "交叉/错序"
            if mid == h:
                ok += 1
            print(f"  本端 ch{h}  →  ch{mid}  "
                  f"(mean_R={mr:.3f} ≈ {ID_R_K[mid]:g}kΩ)  [{mark}]")
    print("-" * 64)
    mapped = int(np.sum(map_to >= 0))
    print(f"已匹配: {mapped}/{N_CH}  直通数: {ok}/{N_CH}")
    print("=" * 64)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    default_log = root / "test-logs" / "测序1.log"

    ap = argparse.ArgumentParser(description="测序算法验证")
    ap.add_argument("--log", type=Path, default=default_log, help="PAIR_SCAN 日志路径")
    args = ap.parse_args()

    if not args.log.is_file():
        raise SystemExit(f"日志不存在: {args.log}")

    V, n = parse_log_to_voltage_cube(args.log)
    result = analyze_sequence(V)
    print_report(args.log, V, n, result)


if __name__ == "__main__":
    main()
