# Auditoria de balanceamento — 23/11/2026

Foram simulados **1,200,960 combates**: 1,200,960 encerrados com vencedor e **0 sem ação válida**. Não houve erros de preparação/substituição nem luta que alcançasse o limite de 300 rodadas.

## Metodologia

- Duelos 1×1: seis iniciais, incluindo espelhos, 1080 sementes por par ordenado, com ataques diretos e com IA usando buffs. Cada confronto também é executado com os lados invertidos; o ranking exclui espelhos.
- Equipes 3×3: as 20 composições distintas contra as 20 composições, 108 sementes por par e por política. As 36 combinações de ordens dos dois times recebem o mesmo número de sementes. Os lados também são invertidos.
- Bosses: 20 composições × seis ordens × 1080 sementes, por boss e por política. São quatro políticas do jogador; o boss sempre usa a IA do jogo.
- Ataques diretos: escolhe o maior poder efetivo × precisão × multiplicador esperado de crítico, respeitando frutos e ações válidas. Não otimiza trocas ou efeitos futuros. IA: usa a implementação atual, incluindo buffs.
- Primeiro ataque: usa o primeiro golpe de dano disponível, como referência de uma estratégia simples; não representa todos os jogadores iniciantes.
- Golpe lógico: cenário favorável com um golpe disponível para cada uma das três plantas, fórmula **HP próprio até metade OU reserva viva** no ataque inicialmente preferido. Se não houver ataque válido, usa outro tipo de ação pela IA. Esse cenário não representa toda configuração possível de recompensa.
- Vida, atributos, habilidades, críticos, erros e status vêm do motor real. A equipe começa restaurada a cada luta, como na campanha. Substitui pela primeira reserva viva; não faz trocas voluntárias nem busca estratégia ótima.
- Empates de velocidade favorecem o lado 0 no motor. Duelos e 3×3 são medidos nos dois lados para reduzir esse viés. Bosses ficam no lado 1, como no jogo.
- A resolução usa as etapas públicas e finaliza rodadas canceladas como battle_screen.c. Contagens de seleção são decisões planejadas, inclusive golpes depois cancelados; não equivalem a golpes acertados.
- Sementes de 1 até N com srand/rand do compilador atual. Resultados são reproduzíveis neste ambiente; outra biblioteca C pode gerar sequências diferentes.

## Ranking das iniciais em duelos

Vitórias contra as outras cinco espécies, com ambos os lados incluídos.

| Planta | Ataques diretos | IA com buffs |
|---|---:|---:|
| Cacto | 53.7% | 40.2% |
| Pimenta | 42.8% | 44.7% |
| Planta Carnívora | 63.1% | 40.9% |
| Orquídea | 48.2% | 63.1% |
| Trevo | 49.7% | 68.5% |
| Hera Venenosa | 42.5% | 42.6% |

### Matriz 1×1 — Ataques diretos

Percentual de vitória da linha contra a coluna, reunindo os dois lados.

| Planta | Cacto | Pimenta | Planta Carnívora | Orquídea | Trevo | Hera Venenosa |
|---|---:|---:|---:|---:|---:|---:|
| Cacto | — | 49.4% | 50.7% | 51.2% | 46.3% | 70.8% |
| Pimenta | 50.6% | — | 42.6% | 58.8% | 1.3% | 60.5% |
| Planta Carnívora | 49.3% | 57.4% | — | 72.7% | 75.5% | 60.7% |
| Orquídea | 48.8% | 41.2% | 27.3% | — | 73.3% | 50.5% |
| Trevo | 53.7% | 98.7% | 24.5% | 26.7% | — | 45.0% |
| Hera Venenosa | 29.2% | 39.5% | 39.3% | 49.5% | 55.0% | — |

### Matriz 1×1 — IA com buffs

Percentual de vitória da linha contra a coluna, reunindo os dois lados.

| Planta | Cacto | Pimenta | Planta Carnívora | Orquídea | Trevo | Hera Venenosa |
|---|---:|---:|---:|---:|---:|---:|
| Cacto | — | 39.9% | 74.4% | 16.3% | 35.0% | 35.6% |
| Pimenta | 60.1% | — | 72.9% | 40.7% | 0.0% | 49.6% |
| Planta Carnívora | 25.6% | 27.1% | — | 72.7% | 18.1% | 60.7% |
| Orquídea | 83.7% | 59.3% | 27.3% | — | 77.1% | 68.1% |
| Trevo | 65.0% | 100.0% | 81.9% | 22.9% | — | 72.9% |
| Hera Venenosa | 64.4% | 50.4% | 39.3% | 31.9% | 27.1% | — |

## Contribuição em equipes 3×3

Vitórias de equipes com a espécie contra equipes sem ela. Ambas as equipes têm três plantas distintas; os outros membros variam por todas as composições. É uma associação nesta bateria, não uma medição isolada do valor da planta.

| Planta presente | Ataques diretos | IA com buffs |
|---|---:|---:|
| Cacto | 50.0% | 44.8% |
| Pimenta | 47.9% | 50.0% |
| Planta Carnívora | 49.3% | 40.0% |
| Orquídea | 52.1% | 56.9% |
| Trevo | 54.1% | 62.8% |
| Hera Venenosa | 46.6% | 45.5% |

## Bosses solo contra três plantas

Todos os percentuais abaixo são **vitórias do jogador**. A média não garante que toda composição seja viável. Sobreviventes são contados somente nas vitórias.

| Boss | Política do jogador | Vitórias | Pior ordem | Melhor ordem | Rodadas médias | Sobreviventes |
|---|---|---:|---:|---:|---:|---:|
| Babosa | Ataques diretos | 96.8% | 92.5% | 99.8% | 7.2 | 1.77 |
| Babosa | IA com buffs | 96.4% | 84.7% | 100.0% | 8.0 | 1.95 |
| Babosa | Primeiro ataque válido | 93.8% | 83.1% | 99.9% | 8.2 | 1.53 |
| Babosa | Ataques + golpe lógico | 99.7% | 98.3% | 100.0% | 4.9 | 2.44 |
| Girassol | Ataques diretos | 76.2% | 50.4% | 94.4% | 8.5 | 1.53 |
| Girassol | IA com buffs | 81.9% | 39.6% | 98.9% | 9.2 | 1.70 |
| Girassol | Primeiro ataque válido | 38.0% | 1.8% | 80.8% | 10.0 | 1.37 |
| Girassol | Ataques + golpe lógico | 98.4% | 91.8% | 100.0% | 5.1 | 2.40 |

**Babosa, Ataques diretos:** pior ordem Cacto / Planta Carnívora / Orquídea: 92.5%. Pior composição, reunindo as seis ordens: Orquídea / Trevo / Planta Carnívora (94.1%). Melhor composição: Cacto / Hera Venenosa / Trevo (99.2%).

**Babosa, IA com buffs:** pior ordem Pimenta / Planta Carnívora / Cacto: 84.7%. Pior composição, reunindo as seis ordens: Pimenta / Trevo / Planta Carnívora (90.4%). Melhor composição: Cacto / Hera Venenosa / Trevo (99.8%).

**Girassol, Ataques diretos:** pior ordem Cacto / Hera Venenosa / Trevo: 50.4%. Pior composição, reunindo as seis ordens: Cacto / Hera Venenosa / Planta Carnívora (58.6%). Melhor composição: Hera Venenosa / Pimenta / Trevo (90.9%).

**Girassol, IA com buffs:** pior ordem Hera Venenosa / Planta Carnívora / Cacto: 39.6%. Pior composição, reunindo as seis ordens: Cacto / Hera Venenosa / Planta Carnívora (68.6%). Melhor composição: Hera Venenosa / Pimenta / Trevo (90.0%).

## Resultados por composição contra bosses

IA com buffs, reunindo as seis ordens de cada composição.

| Composição | Babosa | Girassol |
|---|---:|---:|
| Cacto / Hera Venenosa / Pimenta | 99.4% | 84.6% |
| Cacto / Hera Venenosa / Orquídea | 99.3% | 81.5% |
| Cacto / Hera Venenosa / Trevo | 99.8% | 81.9% |
| Cacto / Hera Venenosa / Planta Carnívora | 98.2% | 68.6% |
| Cacto / Pimenta / Orquídea | 96.7% | 89.3% |
| Cacto / Pimenta / Trevo | 98.2% | 89.8% |
| Cacto / Pimenta / Planta Carnívora | 92.2% | 78.5% |
| Cacto / Orquídea / Trevo | 97.9% | 84.8% |
| Cacto / Orquídea / Planta Carnívora | 93.9% | 81.1% |
| Cacto / Trevo / Planta Carnívora | 93.1% | 70.7% |
| Hera Venenosa / Pimenta / Orquídea | 98.8% | 87.4% |
| Hera Venenosa / Pimenta / Trevo | 99.3% | 90.0% |
| Hera Venenosa / Pimenta / Planta Carnívora | 96.9% | 78.8% |
| Hera Venenosa / Orquídea / Trevo | 99.0% | 84.6% |
| Hera Venenosa / Orquídea / Planta Carnívora | 97.7% | 78.7% |
| Hera Venenosa / Trevo / Planta Carnívora | 97.4% | 69.7% |
| Pimenta / Orquídea / Trevo | 95.9% | 89.8% |
| Pimenta / Orquídea / Planta Carnívora | 91.5% | 85.2% |
| Pimenta / Trevo / Planta Carnívora | 90.4% | 81.7% |
| Orquídea / Trevo / Planta Carnívora | 92.4% | 80.9% |

## Casos sem ação válida

Houve **0 casos**, todos no cenário opcional com golpe lógico. Eles ficam no denominador das taxas e não são contados como vitória ou derrota. A simulação não inventa um comando de esperar que a interface não oferece.


## Limitações e decisões

Os percentuais descrevem políticas automatizadas, não dificuldade universal para pessoas. Não foram simulados busca ótima, trocas voluntárias, todas as fórmulas lógicas ou todas as atribuições possíveis de recompensas. Duelos 1×1 isolam parte do poder; a campanha real é 3×1 e pode valorizar suporte e reservas.

As sementes são reutilizadas para comparar cenários; os combates não são amostras humanas independentes. As métricas não são probabilidades exatas. Mais sementes reduzem ruído aleatório, mas não removem o viés das políticas.

## Reprodução

```powershell
cmake --build build --target balance_audit_check battle_check
.\build\battle_check.exe
.\build\balance_audit_check.exe 1080 > build/balance_audit.jsonl
python tools/summarize_balance_audit.py
```

O JSONL agregado fica em build/ (ignorado pelo Git). O resumo valida a quantidade de cenários e rejeita resultados com erros ou limite de rodadas. BALANCE_TRACE=1 habilita diagnóstico de falhas de preparação.

Impressões SHA-256 dos dados e das regras usados:

- `src/plant_data.c`: `baeebad9231910121a1af4ab052618f474428bff74f3f82187676ff5ee3cd7ab`
- `src/plant.c`: `fbb8499e455fe1fc908c8776d8680d5e3c90c5dafe954e20bc1d6eafaac60f51`
- `src/battle.c`: `d86ff6197ec3e2c955f6c1ab6bc3408ecaa82d5d81a5b933c07c4edbc18ccc8a`
- `src/ai.c`: `3ca24d888240040fa15f75e66abda6e1e60f7ce4046dfb5f281ae61d8194bf76`
