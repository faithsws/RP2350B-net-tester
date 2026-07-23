#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
对线扫描日志分析

流程:
  1. 解析 log → 三维电压数组 V[H][L][MUX]（单位 V；非法 H==L 为 NaN）
  2. 状态数组 status[8] 初值全部为「断路」
  3. 按联通规则扫描 V，把联通通道标为「联通」，并记录联通对
  4. 再按短路规则，仅在已联通通道中找彼此短路，标为「短路」
  5. 输出 RJ45 状态示意图

用法:
  python pair_scan_analyze.py
  python pair_scan_analyze.py --log path/to/log.csv --out out.png
"""

from __future__ import annotations

import argparse
import math
import re
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager
from matplotlib.patches import Circle, FancyBboxPatch, Rectangle

# ---------- 电压典型点（V）与容差 ----------
TOL = 0.15  # 左右范围：正负 0.15V

OPEN_H_V = 9.2        # M==H ≈9.2 → 未成环（保持断路）
OPEN_LOW_V = 0.3      # M!=H 且 <0.3 → 断路
CONN_REMOTE_V = 5.5   # M!=H ≈5.5 → M 联通（映射）
CONN_H_V = 6.7        # M==H ≈6.7 → H/L 成环，双线联通
CONN_L_V = 2.6        # M==L ≈2.6 → H/L 成环，双线联通
SHORT_MH_HI_V = 7.3   # M!=H ≈7.3 → M 与 H 短路
SHORT_MH_MID_V = 4.7  # M!=H ≈4.7 → M 与 H 短路
SHORT_LH_LO_V = 2.0   # ≈2.0 → L 与 H 短路
SHORT_LH_MID_V = 3.6  # ≈3.6 → L 与 H 短路

STATUS_OPEN = "断路"
STATUS_CONN = "联通"
STATUS_SHORT = "短路"

N_CH = 8
LINE_RE = re.compile(
    r"H=Y(?P<h>\d+)\s+L=Y(?P<l>\d+)\s+MUX=Y(?P<m>\d+).*?V=(?P<v>[0-9.]+)V"
)


@dataclass
class AnalyzeResult:
    # V[h][l][m]
    voltage: np.ndarray
    # status[ch]
    channel_status: List[str]
    conn_pairs: Set[Tuple[int, int]] = field(default_factory=set)
    short_pairs: Set[Tuple[int, int]] = field(default_factory=set)
    sample_count: int = 0


def near(v: float, target: float, tol: float = TOL) -> bool:
    if v is None or (isinstance(v, float) and math.isnan(v)):
        return False
    return abs(v - target) <= tol


def parse_log_to_voltage_cube(path: Path) -> Tuple[np.ndarray, int]:
    """
    解析 PAIR_SCAN 日志为三维电压数组。

    返回:
      V: shape (8,8,8), V[h,l,m]；h==l 保持 NaN
      n: 成功写入的样本数
    """
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


def scan_mark_connected(V: np.ndarray) -> Tuple[List[str], Set[Tuple[int, int]]]:
    """
    步骤2: status 初值全断路；按联通规则扫描 V，填充联通。
    联通规则:
      - M==H 且 ≈6.7V → H、L 联通
      - M==L 且 ≈2.6V → H、L 联通
      - M!=H 且 ≈5.5V → M 联通，并记 H-M 联通对
    """
    status = [STATUS_OPEN] * N_CH
    conn_pairs: Set[Tuple[int, int]] = set()

    def mark_conn(*chs: int) -> None:
        for c in chs:
            if 0 <= c < N_CH:
                status[c] = STATUS_CONN

    def add_pair(a: int, b: int) -> None:
        if a == b:
            return
        conn_pairs.add(tuple(sorted((a, b))))
        mark_conn(a, b)

    for h in range(N_CH):
        for l in range(N_CH):
            if h == l:
                continue
            for m in range(N_CH):
                v = float(V[h, l, m])
                if math.isnan(v):
                    continue

                # H 端成环电压 → H/L 联通
                if m == h and near(v, CONN_H_V):
                    add_pair(h, l)
                    continue

                # L 端成环电压 → H/L 联通
                if m == l and near(v, CONN_L_V):
                    add_pair(h, l)
                    continue

                # 远端映射联通
                if m != h and near(v, CONN_REMOTE_V):
                    mark_conn(m)
                    add_pair(h, m)
                    continue

                # 其余（含 9.2V / <0.3V / 其它）保持断路，不改 status

    return status, conn_pairs


def scan_mark_shorts(
    V: np.ndarray,
    status: List[str],
) -> Tuple[List[str], Set[Tuple[int, int]]]:
    """
    步骤3: 仅在已联通通道中，按短路规则找彼此短路。
    短路规则:
      - M!=H 且 ≈7.3V / ≈4.7V → M 与 H 短路（两者均须已联通）
      - ≈2.0V / ≈3.6V → L 与 H 短路（两者均须已联通）
    """
    short_pairs: Set[Tuple[int, int]] = set()
    out = list(status)

    def try_short(a: int, b: int) -> None:
        if a == b:
            return
        if status[a] != STATUS_CONN or status[b] != STATUS_CONN:
            # 只从「联通」集合里挖短路
            return
        short_pairs.add(tuple(sorted((a, b))))
        out[a] = STATUS_SHORT
        out[b] = STATUS_SHORT

    for h in range(N_CH):
        for l in range(N_CH):
            if h == l:
                continue
            for m in range(N_CH):
                v = float(V[h, l, m])
                if math.isnan(v):
                    continue

                if near(v, SHORT_LH_LO_V) or near(v, SHORT_LH_MID_V):
                    try_short(h, l)
                    continue

                if m != h and (near(v, SHORT_MH_HI_V) or near(v, SHORT_MH_MID_V)):
                    try_short(m, h)
                    continue

    return out, short_pairs


def analyze(V: np.ndarray, sample_count: int) -> AnalyzeResult:
    # 1) 状态数组先全部断路（在 scan_mark_connected 内完成）
    # 2) 联通
    status, conn_pairs = scan_mark_connected(V)
    # 3) 从联通中找短路
    status, short_pairs = scan_mark_shorts(V, status)

    return AnalyzeResult(
        voltage=V,
        channel_status=status,
        conn_pairs=conn_pairs,
        short_pairs=short_pairs,
        sample_count=sample_count,
    )


def setup_chinese_font() -> Optional[str]:
    candidates = [
        "Microsoft YaHei",
        "SimHei",
        "SimSun",
        "Noto Sans CJK SC",
        "Source Han Sans SC",
        "Arial Unicode MS",
    ]
    available = {f.name for f in font_manager.fontManager.ttflist}
    for name in candidates:
        if name in available:
            plt.rcParams["font.sans-serif"] = [name]
            plt.rcParams["axes.unicode_minus"] = False
            return name
    return None


def status_color(st: str) -> str:
    return {
        STATUS_OPEN: "#8A9099",
        STATUS_CONN: "#2EBB55",
        STATUS_SHORT: "#E04545",
    }.get(st, "#888888")


def draw_result(result: AnalyzeResult, title: str, out_path: Path) -> None:
    setup_chinese_font()

    fig, ax = plt.subplots(figsize=(10, 6), dpi=140)
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 7)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title(title, fontsize=14, pad=12)

    body = FancyBboxPatch(
        (1.2, 0.8), 7.6, 5.0,
        boxstyle="round,pad=0.05,rounding_size=0.2",
        linewidth=1.5,
        edgecolor="#334155",
        facecolor="#0F172A",
    )
    ax.add_patch(body)
    ax.text(5.0, 5.5, "RJ45 水晶头 1~8", ha="center", va="center",
            color="#E2E8F0", fontsize=12)

    xs = [1.8 + i * 0.9 for i in range(N_CH)]
    y_pin = 3.2
    pin_r = 0.28

    for i in range(N_CH):
        st = result.channel_status[i]
        color = status_color(st)
        ax.add_patch(Circle((xs[i], y_pin), pin_r, facecolor=color,
                            edgecolor="#F8FAFC", linewidth=1.5, zorder=3))
        ax.text(xs[i], y_pin, str(i + 1), ha="center", va="center",
                color="white", fontsize=11, fontweight="bold", zorder=4)
        ax.text(xs[i], y_pin - 0.65, st, ha="center", va="top",
                color=color, fontsize=9)

    # 只在短路的两个通道间画连接线
    for a, b in sorted(result.short_pairs):
        ax.annotate(
            "",
            xy=(xs[b], y_pin + pin_r + 0.08),
            xytext=(xs[a], y_pin + pin_r + 0.08),
            arrowprops=dict(
                arrowstyle="-",
                color="#F87171",
                lw=2.2,
                connectionstyle=f"arc3,rad={0.18 + 0.03 * abs(b - a)}",
            ),
            zorder=2,
        )

    lx, ly = 1.5, 1.35
    for name, color in (
        (STATUS_CONN, status_color(STATUS_CONN)),
        (STATUS_OPEN, status_color(STATUS_OPEN)),
        (STATUS_SHORT, status_color(STATUS_SHORT)),
    ):
        ax.add_patch(Rectangle((lx, ly - 0.12), 0.25, 0.25,
                               facecolor=color, edgecolor="white"))
        ax.text(lx + 0.35, ly, name, va="center", color="#E2E8F0", fontsize=10)
        lx += 1.8

    cnt: Dict[str, int] = defaultdict(int)
    for st in result.channel_status:
        cnt[st] += 1
    summary = (
        f"样本 {result.sample_count}  |  "
        f"联通 {cnt[STATUS_CONN]}  "
        f"断路 {cnt[STATUS_OPEN]}  "
        f"短路 {cnt[STATUS_SHORT]}  |  "
        f"联通对 {len(result.conn_pairs)}  "
        f"短路对 {len(result.short_pairs)}"
    )
    ax.text(5.0, 0.35, summary, ha="center", va="center",
            color="#94A3B8", fontsize=9)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(out_path, bbox_inches="tight", facecolor="#020617")
    plt.close(fig)


def print_voltage_summary(V: np.ndarray) -> None:
    """打印三维数组概要，便于核对解析结果。"""
    valid = ~np.isnan(V)
    n = int(valid.sum())
    print(f"三维电压数组 V[H][L][MUX] shape={V.shape}, 有效样本={n}")
    if n == 0:
        return
    vals = V[valid]
    print(f"  Vmin={vals.min():.3f}V  Vmax={vals.max():.3f}V  Vmean={vals.mean():.3f}V")
    # 每个 (H,L) 是否齐全 8 个 MUX
    complete = 0
    for h in range(N_CH):
        for l in range(N_CH):
            if h == l:
                continue
            if np.all(~np.isnan(V[h, l, :])):
                complete += 1
    print(f"  完整 H/L 组合(含8路MUX)={complete}/56")


def print_report(result: AnalyzeResult, name: str) -> None:
    print(f"\n==== {name} ====")
    print_voltage_summary(result.voltage)
    print("状态数组 status[0..7] (针脚1..8):")
    for ch, st in enumerate(result.channel_status):
        print(f"  针脚{ch + 1}(Y{ch}): {st}")
    if result.conn_pairs:
        pairs = ", ".join(f"{a + 1}-{b + 1}" for a, b in sorted(result.conn_pairs))
        print(f"联通对: {pairs}")
    if result.short_pairs:
        pairs = ", ".join(f"{a + 1}-{b + 1}" for a, b in sorted(result.short_pairs))
        print(f"短路对: {pairs}")


def default_logs(root: Path) -> List[Path]:
    log_dir = root / "test-logs"
    preferred = ["对线全连接.csv", "全断开.csv"]
    found: List[Path] = []
    for name in preferred:
        p = log_dir / name
        if p.exists():
            found.append(p)
    for p in sorted(log_dir.glob("*.csv")):
        if p not in found:
            found.append(p)
    return found


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description="对线扫描日志 → 三维电压数组 → 状态图")
    parser.add_argument("--log", type=str, default="", help="单个日志文件")
    parser.add_argument("--out", type=str, default="", help="输出图片路径")
    parser.add_argument(
        "--outdir",
        type=str,
        default=str(root / "test-logs" / "analysis"),
        help="批量输出目录",
    )
    parser.add_argument(
        "--dump-npy",
        action="store_true",
        help="同时把 V 数组存成 .npy",
    )
    args = parser.parse_args()

    logs = [Path(args.log)] if args.log else default_logs(root)
    if not logs:
        print("未找到 test-logs/*.csv")
        return 1

    outdir = Path(args.outdir)
    for log in logs:
        V, n = parse_log_to_voltage_cube(log)
        if n == 0:
            print(f"警告: {log} 无有效样本")
            continue
        result = analyze(V, n)
        print_report(result, log.name)

        if args.dump_npy:
            npy_path = outdir / f"{log.stem}_V.npy"
            outdir.mkdir(parents=True, exist_ok=True)
            np.save(npy_path, V)
            print(f"电压数组已保存: {npy_path}")

        out_path = Path(args.out) if (args.out and len(logs) == 1) else (
            outdir / f"{log.stem}_status.png"
        )
        draw_result(result, f"对线结果 — {log.stem}", out_path)
        print(f"图片已保存: {out_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
