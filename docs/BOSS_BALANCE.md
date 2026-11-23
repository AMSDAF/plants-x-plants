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
