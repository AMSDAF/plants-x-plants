"""Resume os resultados reais da auditoria sem alterar atributos ou regras do jogo."""
import hashlib
import json
from pathlib import Path
import sys

NAMES = {0: "Cacto", 1: "Girassol", 2: "Babosa", 3: "Hera Venenosa",
         4: "Pimenta", 5: "Orquídea", 6: "Trevo", 7: "Planta Carnívora"}
STARTERS = [0, 4, 7, 5, 6, 3]
POLICIES = {"damage": "Ataques diretos", "ai": "IA com buffs",
            "first": "Primeiro ataque válido", "logic": "Ataques + golpe lógico"}


def rate(rows, plant=None):
    total = sum(r["n"] for r in rows)
    wins = sum(r["wins"] if plant is None or plant in r["left"] else r["losses"]
               for r in rows)
    return 100 * wins / total if total else 0


def team_name(ids):
    return " / ".join(NAMES[i] for i in ids)


def main():
    source = Path(sys.argv[1] if len(sys.argv) > 1 else "build/balance_audit.jsonl")
    destination = Path(sys.argv[2] if len(sys.argv) > 2 else "docs/BALANCE_AUDIT.md")
    rows = [json.loads(line) for line in source.read_text(encoding="utf-8-sig").splitlines()]
    assert len(rows) == 1832, "Auditoria incompleta: esperadas 1832 linhas agregadas."
    assert all(r["n"] == r["wins"] + r["losses"] + r["capped"] + r["errors"] + r["blocked"] for r in rows)
    errors, capped = (sum(r[key] for r in rows) for key in ("errors", "capped"))
    assert errors == capped == 0, f"Resultados incompletos: {errors} erros, {capped} limites."
    samples = rows[0]["n"]
    assert samples % 360 == 0, "Use múltiplo de 360 para equilibrar ordens do 3x3."
    total = sum(r["n"] for r in rows)
    blocked = sum(r["blocked"] for r in rows)
    text = ["# Auditoria de balanceamento — 23/11/2026", "",
            f"Foram simulados **{total:,} combates**: {total-blocked:,} encerrados com vencedor "
            f"e **{blocked} sem ação válida**. Não houve erros de preparação/substituição "
            "nem luta que alcançasse o limite de 300 rodadas.", "",
            "## Metodologia", "",
            f"- Duelos 1×1: seis iniciais, incluindo espelhos, {samples} sementes por par "
            "ordenado, com ataques diretos e com IA usando buffs. Cada confronto também "
            "é executado com os lados invertidos; o ranking exclui espelhos.",
            "- Equipes 3×3: as 20 composições distintas contra as 20 composições, "
            f"{samples//10} sementes por par e por política. As 36 combinações de ordens "
            "dos dois times recebem o mesmo número de sementes. Os lados também são invertidos.",
            f"- Bosses: 20 composições × seis ordens × {samples} sementes, por boss e por "
            "política. São quatro políticas do jogador; o boss sempre usa a IA do jogo.",
            "- Ataques diretos: escolhe o maior poder efetivo × precisão × multiplicador "
            "esperado de crítico, respeitando frutos e ações válidas. Não otimiza trocas "
            "ou efeitos futuros. IA: usa a implementação atual, incluindo buffs.",
            "- Primeiro ataque: usa o primeiro golpe de dano disponível, como referência "
            "de uma estratégia simples; não representa todos os jogadores iniciantes.",
            "- Golpe lógico: cenário favorável com um golpe disponível para cada uma das "
            "três plantas, fórmula **HP próprio até metade OU reserva viva** no ataque "
            "inicialmente preferido. Se não houver ataque válido, usa outro tipo de ação "
            "pela IA. Esse cenário não representa toda configuração possível de recompensa.",
            "- Vida, atributos, habilidades, críticos, erros e status vêm do motor real. "
            "A equipe começa restaurada a cada luta, como na campanha. Substitui pela "
            "primeira reserva viva; não faz trocas voluntárias nem busca estratégia ótima.",
            "- Empates de velocidade favorecem o lado 0 no motor. Duelos e 3×3 são "
            "medidos nos dois lados para reduzir esse viés. Bosses ficam no lado 1, como no jogo.",
            "- A resolução usa as etapas públicas e finaliza rodadas canceladas como "
            "battle_screen.c. Contagens de seleção são decisões planejadas, inclusive "
            "golpes depois cancelados; não equivalem a golpes acertados.",
            "- Sementes de 1 até N com srand/rand do compilador atual. Resultados são "
            "reproduzíveis neste ambiente; outra biblioteca C pode gerar sequências diferentes.", "",
            "## Ranking das iniciais em duelos", "",
            "Vitórias contra as outras cinco espécies, com ambos os lados incluídos.", "",
            "| Planta | Ataques diretos | IA com buffs |", "|---|---:|---:|"]
    for plant in STARTERS:
        values = []
        for policy in ("damage", "ai"):
            selected = [r for r in rows if r["suite"] == "duel" and r["policy"] == policy
                        and r["left"] != r["right"] and plant in r["left"] + r["right"]]
            values.append(rate(selected, plant))
        text.append(f"| {NAMES[plant]} | {values[0]:.1f}% | {values[1]:.1f}% |")
    for policy in ("damage", "ai"):
        text += ["", f"### Matriz 1×1 — {POLICIES[policy]}", "",
                 "Percentual de vitória da linha contra a coluna, reunindo os dois lados.", "",
                 "| Planta | " + " | ".join(NAMES[i] for i in STARTERS) + " |",
                 "|---|" + "---:|" * len(STARTERS)]
        for plant in STARTERS:
            values = []
            for opponent in STARTERS:
                selected = [r for r in rows if r["suite"] == "duel" and r["policy"] == policy
                            and sorted(r["left"] + r["right"]) == sorted([plant, opponent])]
                values.append("—" if plant == opponent else f"{rate(selected, plant):.1f}%")
            text.append(f"| {NAMES[plant]} | " + " | ".join(values) + " |")
    text += ["", "## Contribuição em equipes 3×3", "",
             "Vitórias de equipes com a espécie contra equipes sem ela. Ambas as equipes "
             "têm três plantas distintas; os outros membros variam por todas as composições. "
             "É uma associação nesta bateria, não uma medição isolada do valor da planta.", "",
             "| Planta presente | Ataques diretos | IA com buffs |", "|---|---:|---:|"]
    for plant in STARTERS:
        values = []
        for policy in ("damage", "ai"):
            selected = [r for r in rows if r["suite"] == "team" and r["policy"] == policy
                        and ((plant in r["left"]) != (plant in r["right"]))]
            values.append(rate(selected, plant))
        text.append(f"| {NAMES[plant]} | {values[0]:.1f}% | {values[1]:.1f}% |")
    text += ["", "## Bosses solo contra três plantas", "",
             "Todos os percentuais abaixo são **vitórias do jogador**. A média não garante "
             "que toda composição seja viável. Sobreviventes são contados somente nas vitórias.", "",
             "| Boss | Política do jogador | Vitórias | Pior ordem | Melhor ordem | "
             "Rodadas médias | Sobreviventes |", "|---|---|---:|---:|---:|---:|---:|"]
    boss_details = []
    for boss in (2, 1):
        for policy in POLICIES:
            selected = [r for r in rows if r["suite"] == "boss" and r["right"] == [boss]
                        and r["policy"] == policy]
            n, wins = (sum(r[key] for r in selected) for key in ("n", "wins"))
            text.append(f"| {NAMES[boss]} | {POLICIES[policy]} | {rate(selected):.1f}% | "
                        f"{min(rate([r]) for r in selected):.1f}% | "
                        f"{max(rate([r]) for r in selected):.1f}% | "
                        f"{sum(r['rounds'] for r in selected)/n:.1f} | "
                        f"{sum(r['survivors'] for r in selected)/wins:.2f} |")
        for policy in ("damage", "ai"):
            selected = [r for r in rows if r["suite"] == "boss" and r["right"] == [boss]
                        and r["policy"] == policy]
            worst = min(selected, key=lambda r: rate([r]))
            groups = {}
            for row in selected:
                groups.setdefault(tuple(sorted(row["left"])), []).append(row)
            ordered = sorted(groups.items(), key=lambda item: rate(item[1]))
            boss_details += ["", f"**{NAMES[boss]}, {POLICIES[policy]}:** pior ordem "
                     f"{team_name(worst['left'])}: {rate([worst]):.1f}%. "
                     f"Pior composição, reunindo as seis ordens: {team_name(ordered[0][0])} "
                     f"({rate(ordered[0][1]):.1f}%). Melhor composição: "
                     f"{team_name(ordered[-1][0])} ({rate(ordered[-1][1]):.1f}%)."]
    text += boss_details
    text += ["", "## Resultados por composição contra bosses", "",
             "IA com buffs, reunindo as seis ordens de cada composição.", "",
             "| Composição | Babosa | Girassol |", "|---|---:|---:|"]
    teams = sorted({tuple(sorted(r["left"])) for r in rows if r["suite"] == "boss"})
    for team in teams:
        values = []
        for boss in (2, 1):
            selected = [r for r in rows if r["suite"] == "boss" and r["policy"] == "ai"
                        and tuple(sorted(r["left"])) == team and r["right"] == [boss]]
            values.append(rate(selected))
        text.append(f"| {team_name(team)} | {values[0]:.1f}% | {values[1]:.1f}% |")
    text += ["", "## Casos sem ação válida", "",
             f"Houve **{blocked} casos**, todos no cenário opcional com golpe lógico. "
             "Eles ficam no denominador das taxas e não são contados como vitória ou "
             "derrota. A simulação não inventa um comando de esperar que a interface não oferece.", ""]
    for row in rows:
        if row["blocked"]:
            text.append(f"- {team_name(row['left'])} contra {team_name(row['right'])}: "
                        f"{row['blocked']} caso(s) em {row['n']} sementes.")
    text += ["", "## Limitações e decisões", "",
             "Os percentuais descrevem "
             "políticas automatizadas, não dificuldade universal para pessoas. Não foram "
             "simulados busca ótima, trocas voluntárias, todas as fórmulas lógicas ou "
             "todas as atribuições possíveis de recompensas. Duelos 1×1 isolam parte do "
             "poder; a campanha real é 3×1 e pode valorizar suporte e reservas.", "",
             "As sementes são reutilizadas para comparar cenários; os combates não são "
             "amostras humanas independentes. As métricas não são probabilidades exatas. "
             "Mais sementes reduzem ruído aleatório, mas não removem o viés das políticas.", "",
             "## Reprodução", "", "```powershell",
             "cmake --build build --target balance_audit_check battle_check",
             ".\\build\\battle_check.exe",
             f".\\build\\balance_audit_check.exe {samples} > build/balance_audit.jsonl",
             "python tools/summarize_balance_audit.py", "```", "",
             "O JSONL agregado fica em build/ (ignorado pelo Git). O resumo valida a "
             "quantidade de cenários e rejeita resultados com erros ou limite de rodadas. "
             "BALANCE_TRACE=1 habilita diagnóstico de falhas de preparação.", "",
             "Impressões SHA-256 dos dados e das regras usados:", ""]
    for filename in ("src/plant_data.c", "src/plant.c", "src/battle.c", "src/ai.c"):
        digest = hashlib.sha256(Path(filename).read_bytes()).hexdigest()
        text.append(f"- `{filename}`: `{digest}`")
    destination.write_text("\n".join(text) + "\n", encoding="utf-8")
    print(f"Relatório: {destination}; combates: {total}; erros: {errors}; limite: {capped}.")


if __name__ == "__main__":
    main()
