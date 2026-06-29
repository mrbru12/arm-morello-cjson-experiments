#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import os
import sys
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

COL = {"TOTAL_US": 0, "MEAN_US": 1, "MIN_US": 2, "MAX_US": 3}

MODES = [
    ("Baseline (sem capabilities)",        "#1f77b4",
        ["timings_backend.csv",
         "timings_backend_1.csv",
         "timings_backend_2.csv"]),
    ("Purecap",                            "#ff7f0e",
        ["timings_backend_purecap.csv",
         "timings_backend_purecap_1.csv",
         "timings_backend_purecap_2.csv"]),
    ("Purecap + compartimentalização",     "#2ca02c",
        ["timings_backend_purecap_shared.csv",
         "timings_backend_purecap_shared_1.csv",
         "timings_backend_purecap_shared_2.csv"]),
]

DPI = 200

def carregar_modo(input_dir, arquivos):
    runs = []
    for nome in arquivos:
        caminho = os.path.join(input_dir, nome)
        if not os.path.isfile(caminho):
            sys.exit(f"[erro] arquivo não encontrado: {caminho}")
        # pula o cabeçalho
        dados = np.genfromtxt(caminho, delimiter=",", skip_header=1)
        if dados.ndim == 1:
            dados = dados.reshape(1, -1)
        runs.append(dados)

    n = min(r.shape[0] for r in runs)
    runs = np.stack([r[:n] for r in runs], axis=0)   # (3, n, 4)
    media = runs.mean(axis=0)                          # (n, 4)

    return {
        "n": n,
        "TOTAL_US": media[:, COL["TOTAL_US"]],
        "MEAN_US":  media[:, COL["MEAN_US"]],
        "MIN_US":   media[:, COL["MIN_US"]],
        "MAX_US":   media[:, COL["MAX_US"]],
        # MEAN_US de cada run (para média/desvio globais)
        "MEAN_US_runs": runs[:, :, COL["MEAN_US"]],
    }


def plot_series(dados_por_modo, coluna, titulo, ylabel, saida):
    fig, ax = plt.subplots(figsize=(9, 4.5))
    for (rotulo, cor, _), d in dados_por_modo:
        x = np.arange(1, d["n"] + 1)
        ax.plot(x, d[coluna], color=cor, linewidth=1.4, label=rotulo)
    ax.set_title(titulo)
    ax.set_xlabel("Repetição")
    ax.set_ylabel(ylabel)
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.legend(frameon=True)
    ax.margins(x=0.01)
    fig.tight_layout()
    fig.savefig(saida, dpi=DPI)
    plt.close(fig)
    print(f"[ok] {saida}")


def estatisticas_modo(d):
    serie = d["MEAN_US"]
    bruto_m, bruto_s = float(serie.mean()), float(serie.std())

    s = serie[1:]
    q1, q3 = np.percentile(s, [25, 75])
    iqr = q3 - q1
    mask = s <= q3 + 1.5 * iqr
    ss = s[mask]
    return {
        "bruto_media": bruto_m, "bruto_desvio": bruto_s,
        "ss_media": float(ss.mean()), "ss_desvio": float(ss.std()),
        "n_outliers": int((~mask).sum()),
    }


def plot_overhead_bar(dados_por_modo, saida):
    rotulos, cores, medias, desvios = [], [], [], []
    for (rotulo, cor, _), d in dados_por_modo:
        rotulos.append(rotulo.replace(" + ", "\n+ "))
        cores.append(cor)
        est = estatisticas_modo(d)
        medias.append(est["ss_media"])
        desvios.append(est["ss_desvio"])

    base = medias[0]
    fig, ax = plt.subplots(figsize=(7, 4.5))
    barras = ax.bar(rotulos, medias, yerr=desvios, capsize=5,
                    color=cores, edgecolor="black", linewidth=0.6)
    ax.set_title("Tempo médio de parsing por payload e sobrecusto")
    ax.set_ylabel("Tempo médio por payload (µs)")
    ax.grid(True, axis="y", linestyle="--", alpha=0.4)

    # anota o valor e o sobrecusto sobre o baseline em cada barra
    for i, (b, m) in enumerate(zip(barras, medias)):
        if i == 0:
            txt = f"{m:.2f} µs\n(baseline)"
        else:
            txt = f"{m:.2f} µs\n(+{100.0 * (m - base) / base:.1f}%)"
        ax.text(b.get_x() + b.get_width() / 2, m + max(desvios) + 0.6,
                txt, ha="center", va="bottom", fontsize=9)
    ax.set_ylim(0, max(medias) * 1.30)
    fig.tight_layout()
    fig.savefig(saida, dpi=DPI)
    plt.close(fig)
    print(f"[ok] {saida}")


def imprimir_estatisticas(dados_por_modo):
    ests = [(rot, estatisticas_modo(d)) for (rot, _, _), d in dados_por_modo]

    def tabela(titulo, chave_m, chave_s):
        print(f"\n=== {titulo} (MEAN_US, µs por payload) ===")
        print(f"{'Modo':<34} {'média':>8} {'desvio':>8} {'overhead':>10}")
        base = None
        for rot, e in ests:
            m, s = e[chave_m], e[chave_s]
            if base is None:
                base, over = m, "—"
            else:
                over = f"+{100.0 * (m - base) / base:.1f}%"
            print(f"{rot:<34} {m:8.3f} {s:8.4f} {over:>10}")

    tabela("Todas as repetições", "bruto_media", "bruto_desvio")
    tabela("Regime permanente (sem warm-up e outliers)", "ss_media", "ss_desvio")
    print("\n  outliers descartados por modo (regra IQR):")
    for rot, e in ests:
        print(f"    {rot:<34} {e['n_outliers']}")
    print("\n  -> use os valores de REGIME PERMANENTE nas tabelas do TCC,")
    print("     mantendo o mesmo critério no texto, nas tabelas e nos gráficos.\n")


def main():
    ap = argparse.ArgumentParser(description="Gera gráficos do benchmark cJSON/CHERI.")
    aqui = os.path.dirname(os.path.abspath(__file__))
    ap.add_argument("--input", default=os.path.join(aqui, "..", "contexto",
                    "codigo", "TCC-Bruno", "TCC", "out"),
                    help="diretório com os CSVs (padrão: out/ de contexto)")
    ap.add_argument("--output", default=os.path.join(aqui, "graficos"),
                    help="diretório de saída das figuras")
    args = ap.parse_args()

    os.makedirs(args.output, exist_ok=True)

    dados_por_modo = [(modo, carregar_modo(args.input, modo[2])) for modo in MODES]

    imprimir_estatisticas(dados_por_modo)

    plot_series(dados_por_modo, "MEAN_US",
                "Tempo médio de parsing por payload ao longo das repetições",
                "Tempo médio por payload (µs)",
                os.path.join(args.output, "mean_us.png"))

    plot_series(dados_por_modo, "MAX_US",
                "Tempo máximo de parsing por repetição",
                "Tempo máximo (µs)",
                os.path.join(args.output, "max_us.png"))

    plot_overhead_bar(dados_por_modo,
                      os.path.join(args.output, "overhead_bar.png"))


if __name__ == "__main__":
    main()
