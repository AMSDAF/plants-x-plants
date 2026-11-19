# Relatório do protótipo de combate

Na campanha, interaja com um guardião para iniciar um encontro.
No primeiro encontro, monte a equipe e escolha um golpe para fortalecer antes de lutar.
A arena mostra a planta ativa, o HP das duas equipes, de quem é o turno e o
resultado da última ação. As plantas são desenhadas com formas da raylib.

| Comando | Ação |
| --- | --- |
| **1** ou **2** | Usar um golpe na batalha; durante a montagem, escolher qual recebe o bônus. |
| **Q/W/E/R** | Escolher uma das quatro posições da equipe para entrar em campo. |
| **ENTER** no resultado | Retornar à estufa. |
| **ESC** | Retornar à estufa, inclusive durante a montagem ou a batalha. |

Os golpes, as trocas e os botões do resultado também aceitam cliques.

## Fortaleça um golpe da equipe

Antes da luta, cada jogador humano combina duas condições distintas. Use
**Q/W** para mudar as condições, **E** para mudar o operador e **Enter** para
confirmar. Os mesmos controles estão disponíveis por clique. A fórmula vale
para a equipe inteira, acompanha a planta ativa e é preservada nos encontros
seguintes. Escolha o espaço do golpe com **1/2** ou clique em um dos golpes da
planta ativa. O mesmo espaço recebe o bônus quando qualquer planta da equipe
entrar em campo.

As condições disponíveis são ter uma reserva viva, o rival ter uma reserva
viva, a planta ativa estar viva com até metade do HP e a planta ativa rival
estar viva com até metade do HP. Uma reserva é uma planta viva fora da posição ativa.

| Operador | Regra | Bônus de poder |
| --- | --- | --- |
| **E** | As duas condições são verdadeiras. | +60 |
| **OU** | Pelo menos uma condição é verdadeira. | +45 |
| **XOR** | Exatamente uma condição é verdadeira. | +55 |

O OU oferece o menor bônus por ativar em mais combinações da tabela-verdade.
O E oferece o maior bônus por exigir ambas as condições. O XOR fica entre os
dois. O golpe mantém o próprio nome e precisão; a fórmula só aumenta seu poder.

Por exemplo, com **Sua ativa tem até metade do HP E O adversário tem reserva
viva**, o golpe começa bloqueado com `F E V = F`. Se sua planta perder
metade do HP ou mais e o rival ainda tiver reserva, a arena passa a mostrar
`V E V = V` e fortalece o golpe selecionado. O motor reavalia a expressão ao
receber o comando e aplica o bônus somente enquanto ela for verdadeira.

## Equipes e turnos

O jogador escolhe de duas a quatro plantas entre Cacto,
Girassol, Babosa, Rosa, Hera, Bambu, Samambaia, Lavanda, Orquídea, Lótus,
Hortelã e Trevo. O guardião usa espécies predefinidas e monta um grupo do mesmo
tamanho. Cada espécie usa diretamente os atributos base do catálogo e tem dois golpes próprios.

Selecione uma vaga com **Q/W/E/R** ou clique nela. Depois selecione a espécie
com **setas** e **Espaço** ou clique no cartão. A primeira posição começa
ativa. Se escolher uma espécie já presente para uma vaga ocupada, as duas
trocam de lugar. Para adicionar em uma vaga livre, escolha uma espécie que
ainda esteja fora do grupo. **Backspace** remove uma reserva, mantendo
pelo menos duas plantas no grupo.

O jogador faz a primeira ação. Ao retornar à exploração, sua equipe é
restaurada e sua fórmula é preservada para os encontros seguintes. Cada equipe comporta até quatro plantas.

1. Em um turno normal, a equipe pode atacar ou colocar em campo uma reserva viva.
   Qualquer uma dessas ações passa o turno ao adversário. Tentar trocar pela
   planta já ativa, por uma derrotada ou por uma posição inexistente mantém
   o estado e o turno.
2. Quando um ataque derruba a planta ativa e ainda há reserva viva, o combate
   passa ao estado de substituição obrigatória. O lado que sofreu o nocaute
   só pode escolher uma reserva. Depois da escolha, esse mesmo lado faz seu
   turno normal.
3. Quando uma equipe perde a última planta, o combate registra o vencedor
   imediatamente. A tela continua exibindo a última ação e oferece retorno
   à estufa. O motor rejeita ataques e trocas após o resultado.

## Dano e precisão

A precisão vem do catálogo da espécie. Um golpe com 100% de precisão acerta
sempre. Os demais usam um sorteio de 0 a 99 e acertam quando o número sorteado
é menor que a precisão. Um erro de precisão é uma ação válida: gasta o turno
e deixa o HP do alvo intacto.

O dano usa a fórmula sem nível `((poder × ataque ÷ defesa) ÷ 2 + 2) × multiplicadores`.
Os multiplicadores ficam neutros por enquanto; não há bônus de tipo. O HP fica
limitado a zero e a mensagem informa quanto HP o alvo perdeu. O poder usado
inclui o bônus de fórmula quando as condições são verdadeiras.

## Computador e interface

Cada guardião usa uma fórmula fixa de **E**, combinando uma condição de HP
com a existência de reservas na própria equipe. Quando sua fórmula é verdadeira,
o bônus se aplica ao espaço de golpe definido para a equipe. A CPU escolhe o golpe
com maior produto de poder efetivo por precisão. Esse critério
é uma aproximação simples e ignora o HP do alvo. Para repor uma planta
derrotada, escolhe a primeira reserva válida na ordem da equipe. A CPU só
troca quando a substituição é obrigatória.

A tela acumula o tempo dos quadros antes de cada ação da CPU. Isso dá tempo
para ler as mensagens enquanto a janela continua desenhando e aceitando ESC.
A interface e a CPU chamam as mesmas funções de ataque e troca. O motor
valida a ação e modifica o estado. O desenho apenas consulta esse estado,
e `game.c` coordena entrada e saída dos encontros.

## Como validar

Execute os comandos na raiz do projeto.

```powershell
cmake --build build --target plants_x_plants battle_check name_entry_check menu_screen_check
.\build\battle_check.exe
.\build\name_entry_check.exe
.\build\menu_screen_check.exe
```

`battle_check` verifica atributos das 12 espécies, tabelas-verdade, ações inválidas,
trocas, derrota, substituição pela IA, vitória e restauração. `name_entry_check`
percorre a campanha com guardiões e progresso. `menu_screen_check` confere a
entrada direta na seleção, teclas mantidas, retorno à tela inicial e encerramento.
O modo de batalha independente e seus recursos exclusivos foram removidos.
