# Balanceamento dos guardiões solo

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
