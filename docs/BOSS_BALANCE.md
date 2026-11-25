# Balanceamento dos guardiões solo

> Integração de 25/11/2026: prevalece o catálogo do commit remoto `89aa715`.
> As primeiras tabelas abaixo e os relatórios BALANCE_AUDIT/BALANCE_REVISION
> registram revisões anteriores. Os parâmetros mais recentes estão na seção
> “Chance de vitória com equipes aleatórias”; o catálogo em `plant_data.c` é a fonte atual.

A bateria ampliada de duelos, equipes e bosses está em [BALANCE_AUDIT.md](BALANCE_AUDIT.md).
A rodada de correções e a comparação antes/depois estão em [BALANCE_REVISION.md](BALANCE_REVISION.md).

A revisão reduz a resistência da Babosa e aumenta moderadamente a força do
Girassol. Os valores anteriores são os do catálogo imediatamente antes deste ajuste.

| Ajuste | Babosa | Girassol |
|---|---|---|
| Vida | 275 → 235 | 240, preservada |
| Ataque | 57, preservado | 65 → 70 |
| Defesa | 60 → 45 | 50, preservada |
| Buff | Casca Grossa: +1,0x até 3,0x → +0,25x até 1,5x | Fotossíntese: +0,25x até 1,5x, preservada |
| Atributo máximo com buff | 180 → 68 DEF | 98 → 105 ATQ |
| Raio Solar | Não se aplica | Poder 40 → 50; precisão 30% → 75% |

Com defesa base 45, cada uso de Casca Grossa acrescenta 11 pontos; a sequência
é 45 → 56 → 67 → 68, respeitando o teto arredondado de 1,5x. A Babosa mantém
seus golpes e a cura de 1/8 do dano recebido, arredondada para baixo. O Girassol
mantém a cura de 15% do HP máximo a cada três turnos próprios, os demais golpes,
a defesa e a velocidade. Não foram alteradas as regras compartilhadas de combate.

## Verificação executada

`boss_balance_check` utiliza o motor real e a IA do jogo: 120 equipes ordenadas
(três plantas distintas entre seis), 100 sementes por equipe, 12.000 encontros
por boss. Resolve rodadas pela velocidade, sem golpes lógicos de nenhum lado,
como uma campanha sem as recompensas opcionais. O jogador escolhe golpes de
dano válidos e substitui plantas derrotadas. Não usa buffs do jogador nem
troca voluntária. Também existe uma política que usa apenas o primeiro golpe válido.

O verificador foi atualizado para as rodadas atuais e para a cura do Girassol
no terceiro turno. A comparação anterior usa as mesmas políticas e o mesmo
motor, alterando somente os dados das espécies e omitindo a fixture de limites
(que pressupunha o teto de defesa agora aplicado).

| Política de maior dano esperado | Antes | Depois | Pior ordem depois | Sobreviventes por vitória depois |
|---|---|---|---|---|
| Vitórias do jogador contra Babosa | 27,5% | 95,1% | 70% | 2,09 |
| Vitórias do jogador contra Girassol | 80,3% | 72,6% | 9% | 1,81 |

Usando apenas o primeiro golpe válido, as vitórias depois do ajuste foram
87,0% contra Babosa e 34,6% contra Girassol; as piores ordens venceram 49% e 0%,
respectivamente. Não houve travamentos nas duas políticas. Os limites dos buffs,
a rejeição de aumento após o limite e as duas curas foram verificados por ações
reais do motor. `battle_check` passou, incluindo ações inválidas, trocas,
derrotas, substituição e vitória. O executável do jogo também compilou.

A meta histórica do verificador (toda ordem vencer ao menos 75% das sementes)
**não foi atingida**, inclusive já não era atingida antes deste ajuste. O modo
normal e `--first-move` continuam retornando falha quando essa meta é descumprida;
`--report` permite consultar as métricas sem exigir esse limiar. Os resultados
são amostras automatizadas, não probabilidades universais de vitória. Ainda
é necessário avaliar a sensação de dificuldade jogando a campanha.

```powershell
cmake --build build --target plants_x_plants boss_balance_check battle_check
.\build\boss_balance_check.exe --report
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
