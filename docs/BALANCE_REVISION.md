# Correções de balanceamento — 23/11/2026

## Alterações aplicadas

| Planta | Antes | Depois |
|---|---|---|
| Cacto | HP 120, ATQ 60, DEF 105 | HP 100, ATQ 55, DEF 65 |
| Pimenta | ATQ 70; Crescer +0,5x até 2x; Fruto 125/100% | ATQ 60; Crescer +0,25x até 1,5x; Fruto 95/90% |
| Hera Venenosa | HP 78, ATQ 40 | HP 90, ATQ 55 |
| Orquídea | HP 45, ATQ 40, DEF 90 | HP 65, ATQ 48, DEF 80 |
| Trevo | HP 53, DEF 30 | HP 72, DEF 45 |
| Carnívora e bosses | Dados da bateria anterior | Sem alteração de atributos ou habilidades nesta revisão |

A IA prepara ataque/defesa uma vez antes de atacar e só usa velocidade quando ainda não age antes do rival. A mudança vale para ambos os lados, incluindo os bosses; por isso o comportamento deles muda mesmo com atributos preservados. Jogadores continuam podendo acumular buffs até seus limites.

Se uma fórmula falsa deixar a última planta sem nenhum golpe válido e sem reserva viva, o motor libera o golpe lógico pelo poder base, sem bônus. A necessidade de frutos é preservada. Havendo outro golpe ou uma troca disponível, o bloqueio lógico continua valendo. A interface identifica a exceção como “sem bônus”.

## Comparação reproduzível

Mesmas 1.080 sementes por confronto, composições, ordens e políticas da bateria anterior. As mudanças em atributos e decisões da IA foram avaliadas em conjunto, sem atribuir causalidade a um ajuste isolado.

### Duelos contra as outras iniciais

| Planta | Ataques antes | Ataques depois | IA antes | IA depois |
|---|---:|---:|---:|---:|
| Cacto | 98.5% | 53.7% | 80.6% | 40.2% |
| Pimenta | 65.1% | 42.8% | 91.2% | 44.7% |
| Planta Carnívora | 58.7% | 63.1% | 55.8% | 40.9% |
| Orquídea | 28.9% | 48.2% | 31.2% | 63.1% |
| Trevo | 29.4% | 49.7% | 18.7% | 68.5% |
| Hera Venenosa | 19.4% | 42.5% | 22.6% | 42.6% |

### Bosses — vitórias do jogador

| Boss | Política | Antes | Depois | Pior ordem antes | Pior ordem depois |
|---|---|---:|---:|---:|---:|
| Babosa | Ataques diretos | 95.7% | 96.8% | 75.0% | 92.5% |
| Babosa | IA com buffs | 95.1% | 96.4% | 46.7% | 84.7% |
| Babosa | Primeiro ataque | 88.0% | 93.8% | 60.4% | 83.1% |
| Babosa | Golpe lógico | 99.3% | 99.7% | 92.4% | 98.3% |
| Girassol | Ataques diretos | 73.7% | 76.2% | 6.8% | 50.4% |
| Girassol | IA com buffs | 75.8% | 81.9% | 3.5% | 39.6% |
| Girassol | Primeiro ataque | 34.7% | 38.0% | 0.0% | 1.8% |
| Girassol | Golpe lógico | 96.7% | 98.4% | 68.5% | 91.8% |

## Validação e limites

- Jogo e verificadores compilados com CMake/GCC em C11.
- `battle_check` passou, incluindo regressões para desbloqueio sem bônus, preservação da exigência de fruto, bloqueio com alternativa disponível e decisões de buffs da IA.
- Nova bateria: 1.200.960 vencedores, zero casos sem ação válida, zero erros de preparação/substituição e zero limites de 300 rodadas.
- `boss_balance_check --report` também executou suas verificações de teto e cura e mais 24.000 combates.
- Não foi executada uma sessão gráfica de campanha; a conferência da indicação “sem bônus” foi por código e compilação.

A distribuição melhorou, mas não ficou uniforme. Trevo (68,5%) e Orquídea (63,1%) ainda se destacam nos duelos da IA; isso não deve ser confundido com suas taxas usando ataques diretos (49,7% e 48,2%). A Carnívora tem 63,1% em ataques diretos e 40,9% com a IA. O Girassol ainda exige mais escolha de golpes: repetir o primeiro ataque vence 38,0% em média, enquanto a IA com buffs vence 81,9%. Não há garantia de 75% de vitória em toda ordem de equipe.

A Babosa está acessível após o nerf anterior. O Girassol continua sendo o confronto mais difícil, com resultados muito menos extremos entre equipes do que antes. Não foram feitos novos nerfs diretos nos bosses nesta revisão.

Consulte [BALANCE_AUDIT.md](BALANCE_AUDIT.md) para matrizes, resultados por composição, metodologia, limitações e comandos de reprodução.
