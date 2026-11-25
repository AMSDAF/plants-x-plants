# Balanceamento dos guardiões solo

O problema era a combinação de vida alta com buffs cumulativos e regeneração.
A Babosa alcançava 159 DEF com Casca Grossa e recuperava 25% do dano recebido.
O Girassol alcançava 150 ATQ e recuperava 30% do HP máximo a cada dois turnos.

| Ajuste | Babosa | Girassol |
|---|---|---|
| Vida | 275 → 235 | 225 → 220 |
| Ataque | 46 → 36 | 50 → 36 |
| Defesa | 53 → 38 | 45 → 38 |
| Buff | DEF +0,25x, até 1,5x (57 DEF) | ATQ +0,25x, até 1,5x (54 ATQ) |
| Cura | 1/8 do dano recebido, para baixo | 8% do HP máximo a cada dois turnos próprios |
| Golpes de dano | Poder 34/40, críticos 5%/8% | Poder 42/35, críticos 10%/5% |

Os guardiões continuam sozinhos. A equipe do jogador, os golpes lógicos,
as velocidades e as regras compartilhadas de combate foram preservados.

## Verificação executada

`boss_balance_check` utiliza o motor real e a IA do jogo: 120 equipes ordenadas
(três plantas distintas entre seis), 100 sementes por equipe, 12.000 encontros
por boss. O jogador começa sem golpe lógico, escolhe golpes de dano válidos e
substitui plantas derrotadas. O teste não usa buffs do jogador nem troca
voluntária; também existe uma política que usa apenas o primeiro golpe válido.

Antes, com a política de maior dano esperado, as vitórias eram 60,7% contra
Babosa e 9,6% contra Girassol; algumas equipes não venciam nenhuma semente.
Depois, foram 99,9% e 99,5%, respectivamente. A pior ordem venceu 99% contra
Babosa e 95% contra Girassol; a média de sobreviventes por vitória ficou em
1,75 e 2,00. Usando somente o primeiro golpe válido, ambas tiveram 100% nas
amostras, com médias de 1,56 e 1,71 sobreviventes. Não houve luta travada.

São resultados de políticas automatizadas e sementes fixas, não probabilidades
universais de vitória de jogadores. A meta desta revisão é permitir vencer com
ataques normais e tornar o golpe lógico uma vantagem opcional.

O teste verifica também o limite real dos buffs, a rejeição de aumento após
o limite e as duas curas, usando ações do motor com fixtures determinísticas.
Falha se qualquer ordem vencer menos de 75% das sementes ou se uma luta travar.

```powershell
cmake --build build --target plants_x_plants boss_balance_check battle_check
.\build\boss_balance_check.exe
.\build\boss_balance_check.exe --first-move
.\build\battle_check.exe
```

## Duelos 1x1 por HP e turnos

`boss_matchup_check` foi refeito para comparar apenas duelos 1x1: uma planta
inicial contra cada guardião, 500 sementes fixas por planta. Os parâmetros de
comparação são:

- **HP da planta:** média de vida restante ao fim do duelo (maior é melhor);
- **HP do boss:** média de vida restante ao fim do duelo (menor é melhor; zero
  indica vitória);
- **Turnos:** média até o fim do combate e, no desempate, média de turnos até
  vencer (menor é melhor).

A ordem do ranking é: menor HP do boss → maior HP da planta → menos turnos por
vitória. A contagem de vitórias aparece apenas como contexto. O jogador escolhe
o golpe de dano com maior dano esperado (poder × precisão, com crítico); o boss
usa a IA do jogo. As ações passam pela mesma API de rodadas da interface, sem
golpes lógicos. O teste falha se houver combate travado ou ação inválida.

```powershell
cmake --build build --target boss_matchup_check
.\build\boss_matchup_check.exe
```

Resultados da execução com sementes fixas (médias por duelo):

**Girassol**

| Pos | Planta | HP planta | HP boss | Turnos | Turnos/vitória | Vitórias |
| ---: | :--- | ---: | ---: | ---: | ---: | ---: |
| 1 | Pimenta | 0,1 | 92,6 | 5,6 | 5,9 | 7/500 |
| 2 | Planta Carnívora | 2,0 | 125,2 | 6,1 | 5,9 | 25/500 |
| 3 | Trevo | 0,0 | 153,3 | 3,6 | — | 0/500 |
| 4 | Orquídea | 0,0 | 167,4 | 4,8 | — | 0/500 |
| 5 | Cacto | 0,0 | 167,5 | 7,8 | — | 0/500 |
| 6 | Hera Venenosa | 0,0 | 226,8 | 5,6 | — | 0/500 |

**Babosa**

| Pos | Planta | HP planta | HP boss | Turnos | Turnos/vitória | Vitórias |
| ---: | :--- | ---: | ---: | ---: | ---: | ---: |
| 1 | Pimenta | 0,9 | 95,5 | 6,1 | 9,2 | 20/500 |
| 2 | Cacto | 0,0 | 99,4 | 9,9 | — | 0/500 |
| 3 | Planta Carnívora | 1,3 | 120,2 | 7,3 | 9,4 | 20/500 |
| 4 | Hera Venenosa | 0,2 | 120,5 | 6,1 | 8,8 | 11/500 |
| 5 | Orquídea | 0,9 | 120,6 | 5,7 | 11,1 | 26/500 |
| 6 | Trevo | 0,0 | 142,9 | 3,4 | — | 0/500 |

Contra Girassol, Pimenta deixa o boss com menos vida e é a mais rápida entre
as que vencem; Planta Carnívora termina com mais HP e vence mais vezes. Contra
Babosa, Pimenta também deixa o boss com menos vida, Orquídea tem mais vitórias
(26/500), Planta Carnívora termina com mais HP e Hera Venenosa é a mais rápida
entre as que vencem. As taxas baixas no formato 1x1 são esperadas porque os
guardiões foram ajustados para uma equipe de três.

## Chance de vitória com equipes aleatórias

`random_team_check` amostra 2000 equipes aleatórias de três plantas distintas,
em ordem aleatória, com cinco sementes por equipe: 10.000 duelos por boss e por
política. As ações das duas plantas são preparadas antes da resolução pela
velocidade, pela mesma API de rodadas da interface. A substituição obrigatória
escolhe a reserva viva com mais HP.

Foram comparadas três políticas de ataque:

- **Maior dano:** sempre o golpe de dano com maior dano esperado (poder ×
  precisão, com crítico), sem buff lógico.
- **Tático:** usa buff/debuff enquanto a planta ativa tem mais de 70% do HP e o
  boss mais de 30%; depois, aplica a política de maior dano. Sem buff lógico.
- **Tático + lógico:** mesma política tática, mas com o buff lógico configurado
  para cada planta. A fórmula e o golpe fortalecido **não são sorteados**: são
  escolhidos a partir dos atributos da equipe e do boss no início da batalha.
  São usadas apenas condições estáveis (ataque/defesa maiores e reserva viva)
  com `OR`, porque `XOR` e `AND` podem desativar o golpe quando os atributos
  mudam durante a rodada. A reserva viva recebe prioridade para manter o buff
  ativo. O golpe fortalecido é o de maior dano esperado da planta.

```powershell
cmake --build build --target random_team_check
.\build\random_team_check.exe
```

| Boss | Maior dano | Tático (buff/debuff) | Tático + lógico | Política recomendada |
| :--- | ---: | ---: | ---: | :--- |
| Girassol | 13,77% | 55,80% | 71,64% | Tático + lógico |
| Babosa | 61,00% | 69,11% | 83,84% | Tático + lógico |

O buff lógico escolhido pela equipe venceu nos dois bosses. Em relação ao
tático sem buff, o ganho foi de +15,84 pontos percentuais contra Girassol
(sobreviventes médios de 1,52 para 1,89) e +14,73 contra Babosa (1,29 para
1,71). Portanto, os ataques devem começar com buff/debuff enquanto a planta
estiver saudável, com um golpe fortalecido por uma fórmula estável, e só depois
alternar para o maior dano esperado.

O teste falha se houver combate travado, troca inválida ou ação inválida. As
condições voláteis (HP baixo, status do rival e velocidade) foram evitadas
porque uma ação planejada pode ser revalidada depois que o boss age no mesmo
turno; nenhuma falha ocorreu na execução final.

Esses números refletem o catálogo atual: Girassol tem 250 HP, 80 DEF, 41 SPD e
golpes de poder 80 e 50; Babosa tem 230 HP e `Corte` com 70% de precisão. As
alterações foram feitas fora destes testes e foram preservadas.
